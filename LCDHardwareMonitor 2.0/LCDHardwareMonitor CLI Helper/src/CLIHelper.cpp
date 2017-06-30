#pragma managed

#include <wtypes.h>
#include <msclr/gcroot.h>
using namespace msclr;
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

//TODO: Forget the whole AppDomain thing. It's not that important to optimize for unloading and loading plugins. Use shadow copy and stop calling updates on plugins that have been deleted. This also means we can call into native and just eat the interop cost. Tell people to behave.
//TODO: .NET Native supposedly has ~400% improvement to reverse P/Invoke.
//TODO: Consider .NET Native or NGEN to compile managed plugins to native code.
//TODO: MIght be able to call pluginAppDomain->DoCallback from native code to call directly into a plugin domain. Parameter would be the callback originally returned from the domain.
//TODO: Is it better to call into managed or unmanged code?
//TODO: Do I need a DllMain?

struct CLIHelperState
{
	gcroot<AppDomain^> appDomain = nullptr;
};
CLIHelperState cliHelperState;

ref struct LoadAssemblyArgs : MarshalByRefObject
{
	//'Parameters'
	String^ assemblyName = nullptr;

	//'Return'
	HMODULE hModule;

	void
	LoadAssembly()
	{
		//TODO: Try LoadFile
		//Assembly^ assembly = cliHelperState.appDomain->Load(assemblyName);
		//Assembly^ assembly = Assembly::Load(assemblyName);
		Assembly^ assembly = Assembly::LoadFrom(assemblyName);
		//Module^ module = assembly->GetModule(assemblyName);

		//TODO: What's the best way to get the module handle?
		//return (HMODULE) (void) module->ModuleHandle;
		//hModule = (HMODULE) (void*) Marshal::GetHINSTANCE(module);
	}
};

extern "C" __declspec(dllexport)
HMODULE
Initialize(wchar_t* assemblyDirectory, wchar_t* assemblyName)
{
	//NOTE: Non-domain neutral
	//AppDomain::CurrentDomain::Load
	//AppDomain::CurrentDomain::Unload();

	//TODO: PERF: Disable probing. We know what we want to load dammit.
	//TODO: Probably use shadow copy settings on AppDomainSetup to allow hotloading
	//  https://msdn.microsoft.com/en-us/library/ms404279(v=vs.110).aspx
	//TODO: wprintf?
	//TODO: Which AppDomain does LoadLibrary use? How is this determined?

	String^ assemblyDiirectoryString = gcnew String(assemblyDirectory);
	String^ assemblyNameString       = gcnew String(assemblyName);
	String^ domainName               = assemblyNameString + " AppDomain";

	/* NOTE: Let the new domain inherit/determine ApplicationBase. It ends up using
	 * the same one as this domain, which allows it to find this helper DLL which
	 * is needs because of the callback.
	 * 
	 * Set the PrivateBinPath to the actual plugin folder (relative to ApplicationBase)
	 * so the new domain is able to find the plugin files.
	 */

	AppDomainSetup^ appDomainSetup = gcnew AppDomainSetup();
	appDomainSetup->ApplicationName    = assemblyNameString;
	//appDomainSetup->ApplicationBase    = gcnew String(assemblyDirectory);
	appDomainSetup->PrivateBinPath     = gcnew String(assemblyDirectory);
	appDomainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;
	//appDomainSetup->ShadowCopyFiles    = true;
	//appDomainSetup->ShadowCopyDirectories???

	//TODO: Double check this nullptr
	cliHelperState.appDomain = AppDomain::CreateDomain(domainName, nullptr, appDomainSetup);

	//TODO: Is it worth handling the hosting through COM in C++? More code,
	//but more straightforward (and maybe performant?) code.

	/* NOTE: We need to load the assembly from the new AppDomain, not this one.
	 * Otherwise the assembly ends up getting loading in both AppDomains because
	 * it gets returned from the method and the Assembly type is not marshallable.
	 * This can result in a totally different assembly being returned than was
	 * loaded. Fun. Also we just don't want to waste resources loading it twice.
	 * Which would also prevent it from being unloaded if we unload the new
	 * AppDomain later on.
	 * 
	 * fuslogvw is a wonderful tool.
	 */

	//TODO: Try ExecuteAssembly (load context) or ExecuteAssemblyByName (load-from context)
	//TOOD: Try creating LoadAssemblyArgs in other domain (CreateInstance(AndUnwrap))
	//TODO: Pass module path through initialization args
	//TODO: Args passed to new domain may need to be [Serializable]
	auto loadAssemblyArgs = gcnew LoadAssemblyArgs();
	loadAssemblyArgs->assemblyName = gcnew String(assemblyName);
	auto callback = gcnew CrossAppDomainDelegate(loadAssemblyArgs, &LoadAssemblyArgs::LoadAssembly);
	cliHelperState.appDomain->DoCallBack(callback);

	return loadAssemblyArgs->hModule;
}

extern "C" __declspec(dllexport)
void
Teardown()
{
	AppDomain::Unload(cliHelperState.appDomain);
	cliHelperState.appDomain = nullptr;
}
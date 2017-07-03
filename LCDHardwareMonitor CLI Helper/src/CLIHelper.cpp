#pragma unmanaged
#define EXPORTING 1
#include "CLIHelper.h"
//#include "LHMDataSource.h"

#pragma managed
/* NOTE: This is in a separate project because it's going to get loaded into
 * the AppDomain for every plugin. This happens because we create a new domain
 * then run code defined in this file to lead the actual plugin assembly.
 */

#include <wtypes.h>
#include <msclr/gcroot.h>

using namespace msclr;
using namespace System;
using namespace System::Reflection;

struct CLIHelperState
{
	gcroot<AppDomain^> appDomain = nullptr;
	Plugin plugin;
};
CLIHelperState cliHelperState;

ref struct DomainProxy : MarshalByRefObject
{
	Plugin
	LoadAssembly(c16* assemblyDirectory, c16* assemblyName)
	{
		Plugin plugin = {};

		SetDllDirectoryW(assemblyDirectory);
		HMODULE module = LoadLibraryW(assemblyName);

		plugin.initialize = (InitializePtr) GetProcAddress(module, "Initialize");
		plugin.update     = (UpdatePtr)     GetProcAddress(module, "Update");
		plugin.teardown   = (TeardownPtr)   GetProcAddress(module, "Teardown");

		return plugin;
	}
};

//@TODO: Probably get rid of this
Plugin
_cdecl
ManagedPlugin_Load(c16* assemblyDirectory, c16* assemblyName)
{
	//@NOTE: fuslogvw is _amaaaaaazing_....

	String^ assemblyNameString = gcnew String(assemblyName);

	AppDomainSetup^ appDomainSetup = gcnew AppDomainSetup();
	appDomainSetup->ApplicationName    = assemblyNameString;
	appDomainSetup->PrivateBinPath     = gcnew String(assemblyDirectory);
	appDomainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;

	auto appDomain = AppDomain::CreateDomain("Plugin AppDomain", nullptr, appDomainSetup);

	auto proxy = (DomainProxy^) appDomain->CreateInstanceAndUnwrap("LCDHardwareMonitor CLI Helper", nameof(DomainProxy));
	Plugin plugin = proxy->LoadAssembly(assemblyDirectory, assemblyName);

	cliHelperState.appDomain = appDomain;
	cliHelperState.plugin    = plugin;
	return plugin;
}


void
_cdecl
ManagedPlugin_UpdateAllPlugins()
{
	cliHelperState.plugin.initialize();
	//cliHelperState.plugin.update();
	cliHelperState.plugin.teardown();
}

void
_cdecl
ManagedPlugin_Unload(Plugin plugin)
{
	if (plugin.initialize == cliHelperState.plugin.initialize)
	{
		AppDomain::Unload(cliHelperState.appDomain);
		cliHelperState = {};
	}
}
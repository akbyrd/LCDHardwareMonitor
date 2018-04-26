#include "LHMAPI.h"

#pragma managed
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

[assembly:ComVisible(false)];

[ComVisible(true)]
public interface class
ILHMAppDomainManager
{
	i32 LoadPlugin(c16* name, c16* directory, IntPtr* updateFn);
};

public ref class
LHMAppDomainManager : AppDomainManager, ILHMAppDomainManager
{
public:
	void
	InitializeNewDomain(AppDomainSetup^ appDomainInfo) override
	{
		InitializationFlags = AppDomainManagerInitializationOptions::RegisterWithHost;
	}

	virtual i32
	LoadPlugin(c16* _name, c16* _directory, IntPtr* updateFn)
	{
		auto name = gcnew String(_name);
		auto directory = gcnew String(_directory);

		/* NOTE: LHMAppDomainManager is going to get loaded into each new
		 * AppDomain so we need let ApplicationBase get inherited from the default
		 * domain in order for it to be found. Instead, we set PrivateBinPath
		 * the actual plugin can be found when we load it. */
		auto domainSetup = gcnew AppDomainSetup();
		domainSetup->PrivateBinPath     = directory;
		domainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;

		//TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		AppDomain^ domain = CreateDomain(name, nullptr, domainSetup);
		auto pluginManager = (LHMAppDomainManager^) domain->DomainManager;
		return pluginManager->InitializePlugin(name, directory, updateFn);
	}

private:
	IDataSourcePlugin^ plugin;
	delegate void PluginFn(IntPtr);
	PluginFn^ initialize;
	PluginFn^ update;
	PluginFn^ teardown;

	i32
	InitializePlugin(String^ name, String^ directory, IntPtr* updateFn)
	{
		auto path = String::Format("{0}\\{1}.dll", directory, name);
		auto assembly = Assembly::LoadFrom(path);

		//auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(IDataSourcePlugin::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!plugin)
				{
					plugin = (IDataSourcePlugin^) Activator::CreateInstance(type);
				}
				else
				{
					//TODO: Warning: multiple plugins in same file
				}
			}
		}
		if (!plugin)
		{
			//TODO: Error: load failed
		}

		//NOTE: Function calls average 200ns with this pattern
		initialize = gcnew PluginFn(plugin, &IDataSourcePlugin::Initialize);
		update     = gcnew PluginFn(plugin, &IDataSourcePlugin::Update);
		teardown   = gcnew PluginFn(plugin, &IDataSourcePlugin::Teardown);
		//*initializeFn = Marshal::GetFunctionPointerForDelegate(initialize);
		*updateFn = Marshal::GetFunctionPointerForDelegate(update);
		//*teardownFn = Marshal::GetFunctionPointerForDelegate(teardown);

		return AppDomain::CurrentDomain->Id;
	}
};

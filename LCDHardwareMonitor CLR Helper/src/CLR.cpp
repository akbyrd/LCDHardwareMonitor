#include "LHMAPI.h"

#pragma managed
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

[assembly:ComVisible(false)];

//TODO: Move to a header
struct Plugin
{
	typedef void (__stdcall *PluginFn)(void*);
	u32        appDomainID;
	PluginFn   initialize;
	PluginFn   update;
	PluginFn   teardown;

	b32        isLoaded;
	i32        kind;
	c16*       directory;
	c16*       name;
};

[ComVisible(true)]
public interface class
ILHMAppDomainManager
{
	//TODO: Want to pass Plugin without exposing it to COM
	b32 LoadPlugin(void* plugin);
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

	virtual b32
	LoadPlugin(void* _plugin)
	{
		Plugin* plugin = (Plugin*) _plugin;
		auto name = gcnew String(plugin->name);
		auto directory = gcnew String(plugin->directory);

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
		return pluginManager->InitializePlugin(plugin, name);
	}

private:
	delegate void PluginFn(IntPtr);
	IDataSourcePlugin^ plugin;
	PluginFn^ initialize;
	PluginFn^ update;
	PluginFn^ teardown;

	b32
	InitializePlugin(Plugin* _plugin, String^ name)
	{
		auto assembly = Assembly::Load(name);
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
		_plugin->initialize = (Plugin::PluginFn) (void*) Marshal::GetFunctionPointerForDelegate(initialize);
		_plugin->update     = (Plugin::PluginFn) (void*) Marshal::GetFunctionPointerForDelegate(update);
		_plugin->teardown   = (Plugin::PluginFn) (void*) Marshal::GetFunctionPointerForDelegate(teardown);
		_plugin->appDomainID = AppDomain::CurrentDomain->Id;

		return true;
	}
};

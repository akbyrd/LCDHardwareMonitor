#include "LHMAPI.h"

#pragma managed
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

template<typename T>
using SList = System::Collections::Generic::List<T>;

[assembly:ComVisible(false)];
[ComVisible(true)]
public interface class
ILHMPluginLoader
{
	b32 LoadDataSource   (void* dataSource);
	b32 UnloadDataSource (void* dataSource);
};

ref class LHMPluginLoader;
//TODO: Might just shove this into DataSourceManaged
public ref struct
PluginInfoManaged
{
	//TODO: This pointer will break when the list resizes
	PluginInfo*      pluginInfo;
	AppDomain^       appDomain;
	LHMPluginLoader^ pluginLoader;
};

delegate void DataSourceInitializeDel();
delegate void DataSourceUpdateDel();
delegate void DataSourceTeardownDel();

private ref struct
DataSourceManaged
{
	IDataSourcePlugin^       iDataSource;
	DataSourceInitializeDel^ initialize;
	DataSourceUpdateDel^     update;
	DataSourceTeardownDel^   teardown;
};

public ref class
LHMPluginLoader : AppDomainManager, ILHMPluginLoader
{
	//NOTE: These functions run in the default AppDomain

public:
	void
	InitializeNewDomain(AppDomainSetup^ appDomainInfo) override
	{
		InitializationFlags = AppDomainManagerInitializationOptions::RegisterWithHost;
	}

	virtual b32
	LoadDataSource(void* _dataSource)
	{
		DataSource* dataSource = (DataSource*) _dataSource;
		PluginInfo* pluginInfo = dataSource->pluginInfo;

		b32 success;
		success = LoadPlugin(pluginInfo);
		if (!success) return false;

		PluginInfoManaged^ pluginInfoManaged = GetPluginInfoManaged(pluginInfo);
		success = pluginInfoManaged->pluginLoader->InitializeDataSource(dataSource);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadDataSource(void* _dataSource)
	{
		DataSource* dataSource = (DataSource*) _dataSource;
		PluginInfo* pluginInfo = dataSource->pluginInfo;

		b32 success;
		PluginInfoManaged^ pluginInfoManaged = GetPluginInfoManaged(pluginInfo);
		success = pluginInfoManaged->pluginLoader->TeardownDataSource(dataSource);
		if (!success) return false;

		success = UnloadPlugin(pluginInfo);
		if (!success) return false;

		return true;
	}

private:
	//TODO: Would it be better to store gcroot<...> pointers in the native PluginInfo?
	SList<PluginInfoManaged^>^ pluginInfosManaged = gcnew SList<PluginInfoManaged^>();
	DataSourceManaged dataSourceManaged;

	b32
	LoadPlugin(PluginInfo* pluginInfo)
	{
		auto name      = gcnew String(pluginInfo->name);
		auto directory = gcnew String(pluginInfo->directory);

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

		AppDomain^ appDomain = CreateDomain(name, nullptr, domainSetup);

		PluginInfoManaged^ pluginInfoManaged = gcnew PluginInfoManaged();
		pluginInfoManaged->pluginInfo   = pluginInfo;
		pluginInfoManaged->appDomain    = appDomain;
		pluginInfoManaged->pluginLoader = (LHMPluginLoader^) appDomain->DomainManager;
		pluginInfosManaged->Add(pluginInfoManaged);
		return true;
	}

	b32
	UnloadPlugin(PluginInfo* pluginInfo)
	{
		PluginInfoManaged^ pluginInfoManaged = GetPluginInfoManaged(pluginInfo);
		pluginInfosManaged->Remove(pluginInfoManaged);

		AppDomain::Unload(pluginInfoManaged->appDomain);
		return true;
	}

	PluginInfoManaged^
	GetPluginInfoManaged(PluginInfo* pluginInfo)
	{
		for (i32 i = 0; i < pluginInfosManaged->Count; i++)
			if (pluginInfosManaged[i]->pluginInfo == pluginInfo)
				return pluginInfosManaged[i];

		//Unreachable
		Assert(false);
		return nullptr;
	}



	//NOTE: These functions run in the plugin AppDomain

	b32
	InitializeDataSource(DataSource* dataSource)
	{
		PluginInfo* pluginInfo = dataSource->pluginInfo;
		auto name = gcnew String(pluginInfo->name);

		auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(IDataSourcePlugin::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!dataSourceManaged.iDataSource)
				{
					dataSourceManaged.iDataSource = (IDataSourcePlugin^) Activator::CreateInstance(type);
				}
				else
				{
					//TODO: Warning: multiple plugins in same file
				}
			}
		}
		//TODO: Logging
		//LOG_IF(!dataSourceManaged.iDataSource, L"Failed to find a data source class", Severity::Warning, return false);

		//NOTE: Function calls average 200ns with this pattern
		//Try playing with security settings if optimizing this
		dataSourceManaged.initialize = gcnew DataSourceInitializeDel(dataSourceManaged.iDataSource, &IDataSourcePlugin::Initialize);
		dataSourceManaged.update     = gcnew DataSourceUpdateDel    (dataSourceManaged.iDataSource, &IDataSourcePlugin::Update);
		dataSourceManaged.teardown   = gcnew DataSourceTeardownDel  (dataSourceManaged.iDataSource, &IDataSourcePlugin::Teardown);

		dataSource->initialize = (DataSourceInitializeFn) (void*) Marshal::GetFunctionPointerForDelegate(dataSourceManaged.initialize);
		dataSource->update     = (DataSourceUpdateFn)     (void*) Marshal::GetFunctionPointerForDelegate(dataSourceManaged.update);
		dataSource->teardown   = (DataSourceTeardownFn)   (void*) Marshal::GetFunctionPointerForDelegate(dataSourceManaged.teardown);

		return true;
	}

	b32
	TeardownDataSource(DataSource* dataSource)
	{
		//Nothing to do currently
		return true;
	}
};

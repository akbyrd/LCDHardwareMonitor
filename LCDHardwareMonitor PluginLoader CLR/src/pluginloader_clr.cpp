#pragma unmanaged
#include "LHMAPI.h"
#include "LHMPluginHeader.h"

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
	b32 LoadDataSource   (void* pluginHeader, void* dataSource);
	b32 UnloadDataSource (void* pluginHeader, void* dataSource);
};

[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
private delegate void DataSourceInitializeDel(DS_INITIALIZE_ARGS);

[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
private delegate void DataSourceUpdateDel(DS_UPDATE_ARGS);

[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
private delegate void DataSourceTeardownDel(DS_TEARDOWN_ARGS);

private ref struct
DataSource_clr
{
	//TODO: Replace these with a list of GCHandles or gcroots?
	IDataSourcePlugin^       iDataSource;
	DataSourceInitializeDel^ initialize;
	DataSourceUpdateDel^     update;
	DataSourceTeardownDel^   teardown;
};

public ref struct
LHMPluginLoader : AppDomainManager, ILHMPluginLoader
{
	//NOTE: These functions run in the default AppDomain

	void
	InitializeNewDomain(AppDomainSetup^ appDomainInfo) override
	{
		InitializationFlags = AppDomainManagerInitializationOptions::RegisterWithHost;
	}

	virtual b32
	LoadDataSource(void* _pluginHeader, void* _dataSource)
	{
		DataSource*   dataSource   = (DataSource*) _dataSource;
		PluginHeader* pluginHeader = (PluginHeader*) _pluginHeader;

		b32 success;
		success = LoadPlugin(pluginHeader);
		if (!success) return false;

		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->pluginLoader).Target;
		success = pluginLoader->InitializeDataSource(pluginHeader, dataSource);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadDataSource(void* _pluginHeader, void* _dataSource)
	{
		DataSource*      dataSource   = (DataSource*) _dataSource;
		PluginHeader*    pluginHeader = (PluginHeader*) _pluginHeader;
		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->pluginLoader).Target;

		b32 success;
		success = pluginLoader->TeardownDataSource(dataSource);
		if (!success) return false;

		success = UnloadPlugin(pluginHeader);
		if (!success) return false;

		return true;
	}

private:
	DataSource_clr dataSource_clr;

	b32
	LoadPlugin(PluginHeader* pluginHeader)
	{
		auto name      = gcnew String(pluginHeader->name);
		auto directory = gcnew String(pluginHeader->directory);

		/* NOTE: LHMAppDomainManager is going to get loaded into each new
		 * AppDomain so we need to let ApplicationBase get inherited from the
		 * default domain in order for it to be found. Instead, we set
		 * PrivateBinPath the actual plugin can be found when we load it. */
		auto domainSetup = gcnew AppDomainSetup();
		domainSetup->PrivateBinPath     = directory;
		domainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;

		//TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		AppDomain^ appDomain = CreateDomain(name, nullptr, domainSetup);

		/* TODO : Configure these handles to pin the objects. */
		/* TODO : I'm 'leaking' these GCHandles under the assumption that I
		 * re-create them later using the IntPtr in order to free them in
		 * UnloadPlugin. I'm reasonably confident this works as expected, but I
		 * need to confirm this is ok. It's not a huge concern, though, since the
		 * objects will definitely go away when then AppDomain is unloaded and we
		 * need them for the entire life of the domain.
		 */
		pluginHeader->appDomain    = (void*) (IntPtr) GCHandle::Alloc(appDomain);
		pluginHeader->pluginLoader = (void*) (IntPtr) GCHandle::Alloc(appDomain->DomainManager);
		return true;
	}

	b32
	UnloadPlugin(PluginHeader* pluginHeader)
	{
		AppDomain^ appDomain = (AppDomain^) ((GCHandle) (IntPtr) pluginHeader->appDomain).Target;
		AppDomain::Unload(appDomain);

		((GCHandle) (IntPtr) pluginHeader->appDomain).Free();
		pluginHeader->appDomain = nullptr;

		((GCHandle) (IntPtr) pluginHeader->pluginLoader).Free();
		pluginHeader->pluginLoader = nullptr;
		return true;
	}



	//NOTE: These functions run in the plugin AppDomain

	b32
	InitializeDataSource(PluginHeader* pluginHeader, DataSource* dataSource)
	{
		auto name = gcnew String(pluginHeader->name);

		auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(IDataSourcePlugin::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!dataSource_clr.iDataSource)
				{
					dataSource_clr.iDataSource = (IDataSourcePlugin^) Activator::CreateInstance(type);
				}
				else
				{
					//TODO: Warning: multiple plugins in same file
				}
			}
		}
		//TODO: Logging
		//LOG_IF(!dataSource_clr.iDataSource, L"Failed to find a data source class", Severity::Warning, return false);

		//NOTE: Function calls average 200ns with this pattern
		//Try playing with security settings if optimizing this
		dataSource_clr.initialize = gcnew DataSourceInitializeDel(dataSource_clr.iDataSource, &IDataSourcePlugin::Initialize);
		dataSource_clr.update     = gcnew DataSourceUpdateDel    (dataSource_clr.iDataSource, &IDataSourcePlugin::Update);
		dataSource_clr.teardown   = gcnew DataSourceTeardownDel  (dataSource_clr.iDataSource, &IDataSourcePlugin::Teardown);

		dataSource->initialize = (DataSourceInitializeFn) (void*) Marshal::GetFunctionPointerForDelegate(dataSource_clr.initialize);
		dataSource->update     = (DataSourceUpdateFn)     (void*) Marshal::GetFunctionPointerForDelegate(dataSource_clr.update);
		dataSource->teardown   = (DataSourceTeardownFn)   (void*) Marshal::GetFunctionPointerForDelegate(dataSource_clr.teardown);

		return true;
	}

	b32
	TeardownDataSource(DataSource* dataSource)
	{
		//Nothing to do currently
		return true;
	}
};

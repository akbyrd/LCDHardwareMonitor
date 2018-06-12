#pragma unmanaged
#include "LHMAPI.h"
#include "LHMPluginHeader.h"

#pragma managed
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
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
		success = pluginLoader->TeardownDataSource(pluginHeader, dataSource);
		if (!success) return false;

		success = UnloadPlugin(pluginHeader);
		if (!success) return false;

		return true;
	}

private:
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

		/* TODO : I'm 'leaking' these GCHandles under the assumption that I
		 * re-create them later using the IntPtr in order to free them in
		 * UnloadPlugin. I'm reasonably confident this works as expected, but I
		 * need to confirm this is ok. It's not a huge concern, though, since the
		 * objects will definitely go away when then AppDomain is unloaded and we
		 * need them for the entire life of the domain.
		 */
		pluginHeader->appDomain    = (void*) (IntPtr) GCHandle::Alloc(appDomain               , GCHandleType::Pinned);
		pluginHeader->pluginLoader = (void*) (IntPtr) GCHandle::Alloc(appDomain->DomainManager, GCHandleType::Pinned);
		return true;
	}

	b32
	UnloadPlugin(PluginHeader* pluginHeader)
	{
		GCHandle appDomainHandle    = ((GCHandle) (IntPtr) pluginHeader->appDomain);
		GCHandle pluginLoaderHandle = ((GCHandle) (IntPtr) pluginHeader->pluginLoader);

		AppDomain::Unload((AppDomain^) appDomainHandle.Target);
		appDomainHandle.Free();
		pluginHeader->appDomain = nullptr;

		pluginLoaderHandle.Free();
		pluginHeader->pluginLoader = nullptr;
		return true;
	}



	//NOTE: These functions run in the plugin AppDomain

	b32
	InitializeDataSource(PluginHeader* pluginHeader, DataSource* dataSource)
	{
		auto name = gcnew String(pluginHeader->name);

		IDataSourcePlugin^ pluginInstance;
		auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(IDataSourcePlugin::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!pluginInstance)
				{
					pluginInstance = (IDataSourcePlugin^) Activator::CreateInstance(type);
				}
				else
				{
					//TODO: Warning: multiple plugins in same file
				}
			}
		}
		//TODO: Logging
		//LOG_IF(!pluginInstance, L"Failed to find a data source class", Severity::Warning, return false);
		dataSource->pluginInstance = (void*) (IntPtr) GCHandle::Alloc(pluginInstance, GCHandleType::Pinned);

		//NOTE: Cross domain function calls average 200ns with this pattern
		//Try playing with security settings if optimizing this
		DataSourceInitializeDel initializeDel = gcnew DataSourceInitializeDel(pluginInstance, &IDataSourcePlugin::Initialize);
		DataSourceUpdateDel     updateDel     = gcnew DataSourceUpdateDel    (pluginInstance, &IDataSourcePlugin::Update);
		DataSourceTeardownDel   teardownDel   = gcnew DataSourceTeardownDel  (pluginInstance, &IDataSourcePlugin::Teardown);

		dataSource->initializeDel = (void*) (IntPtr) GCHandle::Alloc(initializeDel, GCHandleType::Pinned);
		dataSource->updateDel     = (void*) (IntPtr) GCHandle::Alloc(updateDel,     GCHandleType::Pinned);
		dataSource->teardownDel   = (void*) (IntPtr) GCHandle::Alloc(teardownDel,   GCHandleType::Pinned);

		dataSource->initialize = (DataSourceInitializeFn) (void*) Marshal::GetFunctionPointerForDelegate(initializeDel);
		dataSource->update     = (DataSourceUpdateFn)     (void*) Marshal::GetFunctionPointerForDelegate(updateDel);
		dataSource->teardown   = (DataSourceTeardownFn)   (void*) Marshal::GetFunctionPointerForDelegate(teardownDel);

		return true;
	}

	b32
	TeardownDataSource(PluginHeader* pluginHeader, DataSource* dataSource)
	{
		dataSource->pluginInstance = 0;

		((GCHandle) dataSource->initializeDel).Free();
		((GCHandle) dataSource->updateDel    ).Free();
		((GCHandle) dataSource->teardownDel  ).Free();

		dataSource->initializeDelegate = 0;
		dataSource->updateDelegate     = 0;
		dataSource->teardownDelegate   = 0;

		return true;
	}
};

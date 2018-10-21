#pragma unmanaged
#include "LHMAPI.h"

#pragma managed
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;
[assembly:ComVisible(false)];

[ComVisible(true)]
public interface class
ILHMPluginLoader
{
	b32 LoadSensorPlugin   (void* pluginHeader, void* sensorPlugin);
	b32 UnloadSensorPlugin (void* pluginHeader, void* sensorPlugin);
	b32 LoadWidgetPlugin   (void* pluginHeader, void* widgetPlugin);
	b32 UnloadWidgetPlugin (void* pluginHeader, void* widgetPlugin);
};

// TODO: Maybe move these to LCDHardwareMonitor CLR API
#define Attributes UnmanagedFunctionPointer(CallingConvention::Cdecl)
[Attributes] private delegate void SensorPluginInitializeDel(SP_INITIALIZE_ARGS);
[Attributes] private delegate void SensorPluginUpdateDel    (SP_UPDATE_ARGS);
[Attributes] private delegate void SensorPluginTeardownDel  (SP_TEARDOWN_ARGS);
[Attributes] private delegate void WidgetPluginInitializeDel(WP_INITIALIZE_ARGS);
[Attributes] private delegate void WidgetPluginUpdateDel    (WP_UPDATE_ARGS);
[Attributes] private delegate void WidgetPluginTeardownDel  (WP_TEARDOWN_ARGS);

public ref struct
LHMPluginLoader : AppDomainManager, ILHMPluginLoader
{
	// NOTE: These functions run in the default AppDomain

	void
	InitializeNewDomain(AppDomainSetup^ appDomainInfo) override
	{
		UNUSED(appDomainInfo);
		InitializationFlags = AppDomainManagerInitializationOptions::RegisterWithHost;
	}

	virtual b32
	LoadSensorPlugin(void* _pluginHeader, void* _sensorPlugin)
	{
		PluginHeader* pluginHeader = (PluginHeader*) _pluginHeader;
		SensorPlugin* sensorPlugin = (SensorPlugin*) _sensorPlugin;

		b32 success;
		success = LoadPlugin(pluginHeader);
		if (!success) return false;

		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->managed.pluginLoader).Target;
		success = pluginLoader->InitializeSensorPlugin(pluginHeader, sensorPlugin);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadSensorPlugin(void* _pluginHeader, void* _sensorPlugin)
	{
		PluginHeader*    pluginHeader = (PluginHeader*) _pluginHeader;
		SensorPlugin*    sensorPlugin = (SensorPlugin*) _sensorPlugin;
		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->managed.pluginLoader).Target;

		b32 success;
		success = pluginLoader->TeardownSensorPlugin(pluginHeader, sensorPlugin);
		if (!success) return false;

		success = UnloadPlugin(pluginHeader);
		if (!success) return false;

		return true;
	}

	virtual b32
	LoadWidgetPlugin(void* _pluginHeader, void* _widgetPlugin)
	{
		PluginHeader* pluginHeader = (PluginHeader*) _pluginHeader;
		WidgetPlugin* widgetPlugin = (WidgetPlugin*) _widgetPlugin;

		b32 success;
		success = LoadPlugin(pluginHeader);
		if (!success) return false;

		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->managed.pluginLoader).Target;
		success = pluginLoader->InitializeWidgetPlugin(pluginHeader, widgetPlugin);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadWidgetPlugin(void* _pluginHeader, void* _widgetPlugin)
	{
		PluginHeader*    pluginHeader = (PluginHeader*) _pluginHeader;
		WidgetPlugin*    widgetPlugin = (WidgetPlugin*) _widgetPlugin;
		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->managed.pluginLoader).Target;

		b32 success;
		success = pluginLoader->TeardownWidgetPlugin(pluginHeader, widgetPlugin);
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

		// NOTE: LHMAppDomainManager is going to get loaded into each new
		// AppDomain so we need to let ApplicationBase get inherited from the
		// default domain in order for it to be found. Instead, we set
		// PrivateBinPath the actual plugin can be found when we load it.
		auto domainSetup = gcnew AppDomainSetup();
		domainSetup->PrivateBinPath     = directory;
		domainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;

		// TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		AppDomain^ appDomain = CreateDomain(name, nullptr, domainSetup);

		// TODO: I'm 'leaking' these GCHandles under the assumption that I
		// re-create them later using the IntPtr in order to free them in
		// UnloadPlugin. I'm reasonably confident this works as expected, but I
		// need to confirm this is ok. It's not a huge concern, though, since the
		// objects will definitely go away when then AppDomain is unloaded and we
		// need them for the entire life of the domain.
		// NOTE: We can't pin the AppDomain pointer. Yay.
		pluginHeader->managed.appDomain    = (void*) (IntPtr) GCHandle::Alloc(appDomain);
		pluginHeader->managed.pluginLoader = (void*) (IntPtr) GCHandle::Alloc(appDomain->DomainManager);
		return true;
	}

	b32
	UnloadPlugin(PluginHeader* pluginHeader)
	{
		GCHandle appDomainHandle    = ((GCHandle) (IntPtr) pluginHeader->managed.appDomain);
		GCHandle pluginLoaderHandle = ((GCHandle) (IntPtr) pluginHeader->managed.pluginLoader);

		AppDomain::Unload((AppDomain^) appDomainHandle.Target);
		appDomainHandle.Free();
		pluginHeader->managed.appDomain = nullptr;

		pluginLoaderHandle.Free();
		pluginHeader->managed.pluginLoader = nullptr;
		return true;
	}



	// NOTE: These functions run in the plugin AppDomain

#if false
	generic <typename T>
	T^
	InstantiatePluginType(PluginHeader* pluginHeader)
	{
		auto name = gcnew String(pluginHeader->name);

		ISensorPlugin^ pluginInstance = nullptr;
		auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(T::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!pluginInstance)
				{
					//pluginInstance = (T^) Activator::CreateInstance(T::typeid);
					pluginInstance = gcnew T();
				}
				else
				{
					// TODO: Warning: multiple plugins in same file
				}
			}
		}
		// TODO: Logging
		//LOG_IF(!pluginInstance, "Failed to find a managed sensor plugin class", Severity::Warning, return false);
		return pluginInstance;
	}
#endif

	b32
	InitializeSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		auto name = gcnew String(pluginHeader->name);

		ISensorPlugin^ pluginInstance = nullptr;
		auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(ISensorPlugin::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!pluginInstance)
				{
					pluginInstance = (ISensorPlugin^) Activator::CreateInstance(type);
				}
				else
				{
					// TODO: Warning: multiple plugins in same file
				}
			}
		}
		// TODO: Logging
		//LOG_IF(!pluginInstance, "Failed to find a managed sensor plugin class", Severity::Warning, return false);
		sensorPlugin->pluginInstance = (void*) (IntPtr) GCHandle::Alloc(pluginInstance);

		SensorPluginInitializeDel^ initializeDel = gcnew SensorPluginInitializeDel(pluginInstance, &ISensorPlugin::Initialize);
		SensorPluginUpdateDel^     updateDel     = gcnew SensorPluginUpdateDel    (pluginInstance, &ISensorPlugin::Update);
		SensorPluginTeardownDel^   teardownDel   = gcnew SensorPluginTeardownDel  (pluginInstance, &ISensorPlugin::Teardown);

		sensorPlugin->initializeDelegate = (void*) (IntPtr) GCHandle::Alloc(initializeDel);
		sensorPlugin->updateDelegate     = (void*) (IntPtr) GCHandle::Alloc(updateDel);
		sensorPlugin->teardownDelegate   = (void*) (IntPtr) GCHandle::Alloc(teardownDel);

		sensorPlugin->initialize = (SensorPluginInitializeFn) (void*) Marshal::GetFunctionPointerForDelegate(initializeDel);
		sensorPlugin->update     = (SensorPluginUpdateFn)     (void*) Marshal::GetFunctionPointerForDelegate(updateDel);
		sensorPlugin->teardown   = (SensorPluginTeardownFn)   (void*) Marshal::GetFunctionPointerForDelegate(teardownDel);

		return true;
	}

	b32
	TeardownSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		UNUSED(pluginHeader);

		sensorPlugin->pluginInstance = 0;

		((GCHandle) (IntPtr) sensorPlugin->initializeDelegate).Free();
		((GCHandle) (IntPtr) sensorPlugin->updateDelegate    ).Free();
		((GCHandle) (IntPtr) sensorPlugin->teardownDelegate  ).Free();

		sensorPlugin->initializeDelegate = 0;
		sensorPlugin->updateDelegate     = 0;
		sensorPlugin->teardownDelegate   = 0;

		return true;
	}

	b32
	InitializeWidgetPlugin(PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
	{
		auto name = gcnew String(pluginHeader->name);

		IWidgetPlugin^ pluginInstance = nullptr;
		auto assembly = Assembly::Load(name);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(IWidgetPlugin::typeid->FullName) != nullptr;
			if (isPlugin)
			{
				if (!pluginInstance)
				{
					pluginInstance = (IWidgetPlugin^) Activator::CreateInstance(type);
				}
				else
				{
					// TODO: Warning: multiple plugins in same file
				}
			}
		}
		// TODO: Logging
		//LOG_IF(!pluginInstance, "Failed to find a managed sensor plugin class", Severity::Warning, return false);
		widgetPlugin->pluginInstance = (void*) (IntPtr) GCHandle::Alloc(pluginInstance);

		WidgetPluginInitializeDel^ initializeDel = gcnew WidgetPluginInitializeDel(pluginInstance, &IWidgetPlugin::Initialize);
		WidgetPluginUpdateDel^     updateDel     = gcnew WidgetPluginUpdateDel    (pluginInstance, &IWidgetPlugin::Update);
		WidgetPluginTeardownDel^   teardownDel   = gcnew WidgetPluginTeardownDel  (pluginInstance, &IWidgetPlugin::Teardown);

		widgetPlugin->initializeDelegate = (void*) (IntPtr) GCHandle::Alloc(initializeDel);
		widgetPlugin->updateDelegate     = (void*) (IntPtr) GCHandle::Alloc(updateDel);
		widgetPlugin->teardownDelegate   = (void*) (IntPtr) GCHandle::Alloc(teardownDel);

		widgetPlugin->initialize = (WidgetPluginInitializeFn) (void*) Marshal::GetFunctionPointerForDelegate(initializeDel);
		widgetPlugin->update     = (WidgetPluginUpdateFn)     (void*) Marshal::GetFunctionPointerForDelegate(updateDel);
		widgetPlugin->teardown   = (WidgetPluginTeardownFn)   (void*) Marshal::GetFunctionPointerForDelegate(teardownDel);

		return true;
	}

	b32
	TeardownWidgetPlugin(PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
	{
		UNUSED(pluginHeader);

		widgetPlugin->pluginInstance = 0;

		((GCHandle) (IntPtr) widgetPlugin->initializeDelegate).Free();
		((GCHandle) (IntPtr) widgetPlugin->updateDelegate    ).Free();
		((GCHandle) (IntPtr) widgetPlugin->teardownDelegate  ).Free();

		widgetPlugin->initializeDelegate = 0;
		widgetPlugin->updateDelegate     = 0;
		widgetPlugin->teardownDelegate   = 0;

		return true;
	}
};

// NOTE: Cross domain function calls average 200ns with the delegate pattern.
// Try playing with security settings if optimizing this

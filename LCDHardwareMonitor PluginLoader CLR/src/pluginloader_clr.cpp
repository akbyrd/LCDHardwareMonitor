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
	b32 LoadSensorPlugin   (void* pluginHeader, void* sensorPlugin);
	b32 UnloadSensorPlugin (void* pluginHeader, void* sensorPlugin);
};

[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
private delegate void SensorPluginInitializeDel(SP_INITIALIZE_ARGS);

[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
private delegate void SensorPluginUpdateDel(SP_UPDATE_ARGS);

[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
private delegate void SensorPluginTeardownDel(SP_TEARDOWN_ARGS);

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
	LoadSensorPlugin(void* _pluginHeader, void* _sensorPlugin)
	{
		PluginHeader* pluginHeader = (PluginHeader*) _pluginHeader;
		SensorPlugin* sensorPlugin = (SensorPlugin*) _sensorPlugin;

		b32 success;
		success = LoadPlugin(pluginHeader);
		if (!success) return false;

		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->pluginLoader).Target;
		success = pluginLoader->InitializeSensorPlugin(pluginHeader, sensorPlugin);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadSensorPlugin(void* _pluginHeader, void* _sensorPlugin)
	{
		PluginHeader*    pluginHeader = (PluginHeader*) _pluginHeader;
		SensorPlugin*    sensorPlugin = (SensorPlugin*) _sensorPlugin;
		LHMPluginLoader^ pluginLoader = (LHMPluginLoader^) ((GCHandle) (IntPtr) pluginHeader->pluginLoader).Target;

		b32 success;
		success = pluginLoader->TeardownSensorPlugin(pluginHeader, sensorPlugin);
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
		//NOTE: We can't pin the AppDomain pointer. Yay.
		pluginHeader->appDomain    = (void*) (IntPtr) GCHandle::Alloc(appDomain);
		pluginHeader->pluginLoader = (void*) (IntPtr) GCHandle::Alloc(appDomain->DomainManager);
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
	InitializeSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		auto name = gcnew String(pluginHeader->name);

		ISensorPlugin^ pluginInstance;
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
					//TODO: Warning: multiple plugins in same file
				}
			}
		}
		//TODO: Logging
		//LOG_IF(!pluginInstance, L"Failed to find a managed sensor plugin class", Severity::Warning, return false);
		sensorPlugin->pluginInstance = (void*) (IntPtr) GCHandle::Alloc(pluginInstance);

		//NOTE: Cross domain function calls average 200ns with this pattern
		//Try playing with security settings if optimizing this
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
		sensorPlugin->pluginInstance = 0;

		((GCHandle) (IntPtr) sensorPlugin->initializeDelegate).Free();
		((GCHandle) (IntPtr) sensorPlugin->updateDelegate    ).Free();
		((GCHandle) (IntPtr) sensorPlugin->teardownDelegate  ).Free();

		sensorPlugin->initializeDelegate = 0;
		sensorPlugin->updateDelegate     = 0;
		sensorPlugin->teardownDelegate   = 0;

		return true;
	}
};

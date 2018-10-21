#pragma unmanaged
#include "LHMAPI.h"
#include "LHMPluginHeader.h"
#include "LHMSensorPlugin.h"
#include "LHMWidgetPlugin.h"

#pragma managed
using namespace System;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

// NOTE: Cross domain function calls average 200ns with the delegate pattern.
// Try playing with security settings if optimizing this

// TODO: *Really* want to default to non-visible so TLBs aren't bloated to
// hell, but I can't find a way to allow native types in as parameters to
// ILHMPluginLoader functions without setting the entire assembly to visible.
// And currently I prefer the bloat to passing void* parameters and casting.
//[assembly:ComVisible(false)];

[ComVisible(true)]
public interface class
ILHMPluginLoader
{
	b32 LoadSensorPlugin   (PluginHeader* pluginHeader, SensorPlugin* sensorPlugin);
	b32 UnloadSensorPlugin (PluginHeader* pluginHeader, SensorPlugin* sensorPlugin);
	b32 LoadWidgetPlugin   (PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin);
	b32 UnloadWidgetPlugin (PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin);
};

[ComVisible(false)]
public value struct
SensorPlugin_CLR
{
	#define Attributes UnmanagedFunctionPointer(CallingConvention::Cdecl)
	[Attributes] delegate void InitializeDelegate(SP_INITIALIZE_ARGS);
	[Attributes] delegate void UpdateDelegate    (SP_UPDATE_ARGS);
	[Attributes] delegate void TeardownDelegate  (SP_TEARDOWN_ARGS);
	#undef Attributes

	ISensorPlugin^      pluginInstance;
	InitializeDelegate^ initializeDelegate;
	UpdateDelegate^     updateDelegate;
	TeardownDelegate^   teardownDelegate;
};

[ComVisible(false)]
public value struct
WidgetPlugin_CLR
{
	#define Attributes UnmanagedFunctionPointer(CallingConvention::Cdecl)
	[Attributes] delegate void InitializeDelegate(WP_INITIALIZE_ARGS);
	[Attributes] delegate void UpdateDelegate    (WP_UPDATE_ARGS);
	[Attributes] delegate void TeardownDelegate  (WP_TEARDOWN_ARGS);
	#undef Attributes

	IWidgetPlugin^      pluginInstance;
	InitializeDelegate^ initializeDelegate;
	UpdateDelegate^     updateDelegate;
	TeardownDelegate^   teardownDelegate;
};

[ComVisible(false)]
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
	LoadSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		b32 success;

		success = LoadPlugin(pluginHeader);
		if (!success) return false;

		LHMPluginLoader^ pluginLoader = GetDomainResidentLoader(pluginHeader);
		success = pluginLoader->InitializeSensorPlugin(pluginHeader, sensorPlugin);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		b32 success;

		LHMPluginLoader^ pluginLoader = GetDomainResidentLoader(pluginHeader);
		success = pluginLoader->TeardownSensorPlugin(pluginHeader, sensorPlugin);
		if (!success) return false;

		success = UnloadPlugin(pluginHeader);
		if (!success) return false;

		return true;
	}

	virtual b32
	LoadWidgetPlugin(PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
	{
		b32 success;

		success = LoadPlugin(pluginHeader);
		if (!success) return false;

		LHMPluginLoader^ pluginLoader = GetDomainResidentLoader(pluginHeader);
		success = pluginLoader->InitializeWidgetPlugin(pluginHeader, widgetPlugin);
		if (!success) return false;

		return true;
	}

	virtual b32
	UnloadWidgetPlugin(PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
	{
		b32 success;

		LHMPluginLoader^ pluginLoader = GetDomainResidentLoader(pluginHeader);
		success = pluginLoader->TeardownWidgetPlugin(pluginHeader, widgetPlugin);
		if (!success) return false;

		success = UnloadPlugin(pluginHeader);
		if (!success) return false;

		return true;
	}

	LHMPluginLoader^
	GetDomainResidentLoader (PluginHeader* pluginHeader)
	{
		GCHandle         appDomainHandle = (GCHandle) (IntPtr) pluginHeader->userData;
		AppDomain^       appDomain       = (AppDomain^) appDomainHandle.Target;
		LHMPluginLoader^ pluginLoader    = (LHMPluginLoader^) appDomain->DomainManager;
		return pluginLoader;
	}

	b32
	LoadPlugin(PluginHeader* pluginHeader)
	{
		auto name      = gcnew String(pluginHeader->name);
		auto directory = gcnew String(pluginHeader->directory);

		// NOTE: LHMAppDomainManager is going to get loaded into each new
		// AppDomain so we need to let ApplicationBase get inherited from the
		// default domain in order for it to be found. We set PrivateBinPath so
		// the actual plugin can be found when we load it.
		auto domainSetup = gcnew AppDomainSetup();
		domainSetup->PrivateBinPath     = directory;
		domainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;

		// TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		AppDomain^ appDomain = CreateDomain(name, nullptr, domainSetup);
		pluginHeader->userData = (void*) (IntPtr) GCHandle::Alloc(appDomain);
		return true;
	}

	b32
	UnloadPlugin(PluginHeader* pluginHeader)
	{
		GCHandle appDomainHandle = (GCHandle) (IntPtr) pluginHeader->userData;

		AppDomain::Unload((AppDomain^) appDomainHandle.Target);
		appDomainHandle.Free();
		pluginHeader->userData = nullptr;
		return true;
	}



	// NOTE: These functions run in the plugin AppDomain

	SensorPlugin_CLR sensorPluginCLR;
	WidgetPlugin_CLR widgetPluginCLR;

	generic <typename T>
	static b32
	LoadAssemblyAndInstantiateType (c8* _fileName, T% instance)
	{
		// NOTE: T^ and T^% map back to plain T
		String^ typeName = T::typeid->FullName;
		String^ fileName = gcnew String(_fileName);

		auto assembly = Assembly::Load(fileName);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			bool isPlugin = type->GetInterface(typeName) != nullptr;
			if (isPlugin)
			{
				if (!instance)
				{
					instance = (T) Activator::CreateInstance(type);
				}
				else
				{
					// TODO: Warning: multiple plugins in same file
				}
			}
		}
		// TODO: Logging
		//LOG_IF(!instance, "Failed to find a managed sensor plugin class", Severity::Warning, return false);

		return true;
	}

	b32
	InitializeSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		b32 success = LoadAssemblyAndInstantiateType(pluginHeader->name, sensorPluginCLR.pluginInstance);
		if (!success) return false;

		auto iInitialize = dynamic_cast<ISensorInitialize^>(sensorPluginCLR.pluginInstance);
		if (iInitialize)
		{
			sensorPluginCLR.initializeDelegate = gcnew SensorPlugin_CLR::InitializeDelegate(iInitialize, &ISensorInitialize::Initialize);
			sensorPlugin->initialize = (SensorPluginInitializeFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.initializeDelegate);
		}

		auto iUpdate = dynamic_cast<ISensorUpdate^>(sensorPluginCLR.pluginInstance);
		if (iUpdate)
		{
			sensorPluginCLR.updateDelegate = gcnew SensorPlugin_CLR::UpdateDelegate(iUpdate, &ISensorUpdate::Update);
			sensorPlugin->update = (SensorPluginUpdateFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.updateDelegate);
		}

		auto iTeardown = dynamic_cast<ISensorTeardown^>(sensorPluginCLR.pluginInstance);
		if (iTeardown)
		{
			sensorPluginCLR.teardownDelegate = gcnew SensorPlugin_CLR::TeardownDelegate(iTeardown, &ISensorTeardown::Teardown);
			sensorPlugin->teardown = (SensorPluginTeardownFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.teardownDelegate);
		}

		return true;
	}

	b32
	TeardownSensorPlugin(PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
	{
		UNUSED(pluginHeader);

		sensorPlugin->initialize = 0;
		sensorPlugin->update     = 0;
		sensorPlugin->teardown   = 0;
		sensorPluginCLR = SensorPlugin_CLR();

		return true;
	}

	b32
	InitializeWidgetPlugin(PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
	{
		b32 success = LoadAssemblyAndInstantiateType(pluginHeader->name, widgetPluginCLR.pluginInstance);
		if (!success) return false;

		auto iInitialize = dynamic_cast<IWidgetInitialize^>(widgetPluginCLR.pluginInstance);
		if (iInitialize)
		{
			widgetPluginCLR.initializeDelegate = gcnew WidgetPlugin_CLR::InitializeDelegate(iInitialize, &IWidgetInitialize::Initialize);
			widgetPlugin->initialize = (WidgetPluginInitializeFn*) (void*) Marshal::GetFunctionPointerForDelegate(widgetPluginCLR.initializeDelegate);
		}

		auto iUpdate = dynamic_cast<IWidgetUpdate^>(widgetPluginCLR.pluginInstance);
		if (iUpdate)
		{
			widgetPluginCLR.updateDelegate = gcnew WidgetPlugin_CLR::UpdateDelegate(iUpdate, &IWidgetUpdate::Update);
			widgetPlugin->update = (WidgetPluginUpdateFn*) (void*) Marshal::GetFunctionPointerForDelegate(widgetPluginCLR.updateDelegate);
		}

		auto iTeardown = dynamic_cast<IWidgetTeardown^>(widgetPluginCLR.pluginInstance);
		if (iTeardown)
		{
			widgetPluginCLR.teardownDelegate = gcnew WidgetPlugin_CLR::TeardownDelegate(iTeardown, &IWidgetTeardown::Teardown);
			widgetPlugin->teardown = (WidgetPluginTeardownFn*) (void*) Marshal::GetFunctionPointerForDelegate(widgetPluginCLR.teardownDelegate);
		}

		return true;
	}

	b32
	TeardownWidgetPlugin(PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
	{
		UNUSED(pluginHeader);

		widgetPlugin->initialize = 0;
		widgetPlugin->update     = 0;
		widgetPlugin->teardown   = 0;
		widgetPluginCLR = WidgetPlugin_CLR();

		return true;
	}
};

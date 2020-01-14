#pragma unmanaged
#include "LHMAPICLR.h"

#pragma unmanaged
#include "plugin_shared.h"

#pragma managed
#pragma make_public(SensorPlugin)
#pragma make_public(WidgetPlugin)

using namespace System;
using namespace System::IO;
using namespace System::Reflection;
using namespace System::Runtime::InteropServices;

// NOTE: Cross domain function calls average 200ns with the delegate pattern.
// Try playing with security settings if optimizing this

// NOTE: Can't pass references through COM interfaces. tlbgen replaces them with pointers.

// TODO: *Really* want to default to non-visible so TLBs aren't bloated to
// hell, but I can't find a way to allow native types as parameters to
// ILHMPluginLoader functions without setting the entire assembly to visible.
// And currently I prefer the bloat to passing void* parameters and casting.
//[assembly:ComVisible(false)];

[ComVisible(true)]
public interface class
ILHMPluginLoader
{
	b8 LoadSensorPlugin   (SensorPlugin& sensorPlugin);
	b8 UnloadSensorPlugin (SensorPlugin& sensorPlugin);
	b8 LoadWidgetPlugin   (WidgetPlugin& widgetPlugin);
	b8 UnloadWidgetPlugin (WidgetPlugin& widgetPlugin);
};

[ComVisible(false)]
public value struct
SensorPlugin_CLR
{
	#define Attributes UnmanagedFunctionPointer(CallingConvention::Cdecl)
	[Attributes] delegate b8   InitializeDelegate (PluginContext& context, SensorPluginAPI::Initialize api);
	[Attributes] delegate void UpdateDelegate     (PluginContext& context, SensorPluginAPI::Update     api);
	[Attributes] delegate void TeardownDelegate   (PluginContext& context, SensorPluginAPI::Teardown   api);
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
	[Attributes] delegate b8   InitializeDelegate (PluginContext& context, WidgetPluginAPI::Initialize api);
	[Attributes] delegate void UpdateDelegate     (PluginContext& context, WidgetPluginAPI::Update     api);
	[Attributes] delegate void TeardownDelegate   (PluginContext& context, WidgetPluginAPI::Teardown   api);
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
	// ----------------------------------------------------------------------------------------------
	// Executed in Default AppDomain

	void
	InitializeNewDomain(AppDomainSetup^ appDomainInfo) override
	{
		UNUSED(appDomainInfo);
		InitializationFlags = AppDomainManagerInitializationOptions::RegisterWithHost;
	}

	virtual b8
	LoadSensorPlugin(SensorPlugin& sensorPlugin)
	{
		b8 success;

		success = LoadPlugin(sensorPlugin.header);
		if (!success) return false;

		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(sensorPlugin.header);
		success = pluginLoader.InitializeSensorPlugin(sensorPlugin);
		if (!success) return false;

		return true;
	}

	virtual b8
	UnloadSensorPlugin(SensorPlugin& sensorPlugin)
	{
		b8 success;

		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(sensorPlugin.header);
		success = pluginLoader.TeardownSensorPlugin(sensorPlugin);
		if (!success) return false;

		success = UnloadPlugin(sensorPlugin.header);
		if (!success) return false;

		return true;
	}

	virtual b8
	LoadWidgetPlugin(WidgetPlugin& widgetPlugin)
	{
		b8 success;

		success = LoadPlugin(widgetPlugin.header);
		if (!success) return false;

		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(widgetPlugin.header);
		success = pluginLoader.InitializeWidgetPlugin(widgetPlugin);
		if (!success) return false;

		return true;
	}

	virtual b8
	UnloadWidgetPlugin(WidgetPlugin& widgetPlugin)
	{
		b8 success;

		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(widgetPlugin.header);
		success = pluginLoader.TeardownWidgetPlugin(widgetPlugin);
		if (!success) return false;

		success = UnloadPlugin(widgetPlugin.header);
		if (!success) return false;

		return true;
	}

	LHMPluginLoader%
	GetDomainResidentLoader(PluginHeader pluginHeader)
	{
		GCHandle         appDomainHandle = (GCHandle) (IntPtr) pluginHeader.userData;
		AppDomain%       appDomain       = *(AppDomain^) appDomainHandle.Target;
		LHMPluginLoader% pluginLoader    = *(LHMPluginLoader^) appDomain.DomainManager;
		return pluginLoader;
	}

	b8
	LoadPlugin(PluginHeader& pluginHeader)
	{
		auto name      = gcnew CLRString(pluginHeader.fileName.data);
		auto directory = gcnew CLRString(pluginHeader.directory.data);

		// NOTE: LHMAppDomainManager is going to get loaded into each new
		// AppDomain so we need to let ApplicationBase get inherited from the
		// default domain in order for it to be found. We set PrivateBinPath so
		// the actual plugin can be found when we load it.
		AppDomainSetup% domainSetup = *gcnew AppDomainSetup();
		domainSetup.PrivateBinPath     = directory;
		domainSetup.LoaderOptimization = LoaderOptimization::MultiDomainHost;

		// TODO: Shadowcopy and file watch
		//domainSetup.CachePath             = "Cache"
		//domainSetup.ShadowCopyDirectories = true
		//domainSetup.ShadowCopyFiles       = true

		AppDomain^ appDomain = CreateDomain(name, nullptr, %domainSetup);
		pluginHeader.userData = (void*) (IntPtr) GCHandle::Alloc(appDomain);
		return true;
	}

	b8
	UnloadPlugin(PluginHeader& pluginHeader)
	{
		GCHandle appDomainHandle = (GCHandle) (IntPtr) pluginHeader.userData;

		AppDomain::Unload((AppDomain^) appDomainHandle.Target);
		appDomainHandle.Free();
		pluginHeader.userData = nullptr;
		return true;
	}

	// ----------------------------------------------------------------------------------------------
	// Executed in Plugin AppDomain

	SensorPlugin_CLR sensorPluginCLR;
	WidgetPlugin_CLR widgetPluginCLR;

	generic <typename T>
	static b8
	LoadAssemblyAndInstantiateType(LHMString _fileName, T% instance)
	{
		// NOTE: T^ and T^% map back to plain T
		CLRString^ typeName = T::typeid->FullName;
		CLRString^ fileName = Path::GetFileNameWithoutExtension(gcnew CLRString(_fileName.data));

		auto assembly = Assembly::Load(fileName);
		for each (Type^ type in assembly->GetExportedTypes())
		{
			b8 isPlugin = type->GetInterface(typeName) != nullptr;
			if (isPlugin)
			{
				if (!instance)
				{
					instance = (T) Activator::CreateInstance(type);
				}
				else
				{
					// TODO: Logging
					//LOG_IF(!instance, return false,
					//	Severity::Error, "Found multiple managed plugin types '%', _fileName.data");
				}
			}
		}
		// TODO: Logging
		//LOG_IF(!instance, return false,
		//	Severity::Error, "Failed to find a managed plugin type '%', _fileName.data");

		return true;
	}

	b8
	InitializeSensorPlugin(SensorPlugin& sensorPlugin)
	{
		b8 success = LoadAssemblyAndInstantiateType(sensorPlugin.header.fileName, sensorPluginCLR.pluginInstance);
		if (!success) return false;

		sensorPluginCLR.pluginInstance->GetPluginInfo(sensorPlugin.info);

		// DEBUG: Remove me (just for fast loading)
		#if true
		#pragma message("warning: Sensor plugin init temporarily disabled for faster loading")
		#else
		auto iInitialize = dynamic_cast<ISensorInitialize^>(sensorPluginCLR.pluginInstance);
		if (iInitialize)
		{
			sensorPluginCLR.initializeDelegate = gcnew SensorPlugin_CLR::InitializeDelegate(iInitialize, &ISensorInitialize::Initialize);
			sensorPlugin.functions.initialize = (SensorPluginFunctions::InitializeFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.initializeDelegate);
		}

		auto iUpdate = dynamic_cast<ISensorUpdate^>(sensorPluginCLR.pluginInstance);
		if (iUpdate)
		{
			sensorPluginCLR.updateDelegate = gcnew SensorPlugin_CLR::UpdateDelegate(iUpdate, &ISensorUpdate::Update);
			sensorPlugin.functions.update = (SensorPluginFunctions::UpdateFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.updateDelegate);
		}

		auto iTeardown = dynamic_cast<ISensorTeardown^>(sensorPluginCLR.pluginInstance);
		if (iTeardown)
		{
			sensorPluginCLR.teardownDelegate = gcnew SensorPlugin_CLR::TeardownDelegate(iTeardown, &ISensorTeardown::Teardown);
			sensorPlugin.functions.teardown = (SensorPluginFunctions::TeardownFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.teardownDelegate);
		}
		#endif

		return true;
	}

	b8
	TeardownSensorPlugin(SensorPlugin& sensorPlugin)
	{
		sensorPlugin.functions = {};
		sensorPluginCLR = SensorPlugin_CLR();

		return true;
	}

	b8
	InitializeWidgetPlugin(WidgetPlugin& widgetPlugin)
	{
		b8 success = LoadAssemblyAndInstantiateType(widgetPlugin.header.fileName, widgetPluginCLR.pluginInstance);
		if (!success) return false;

		widgetPluginCLR.pluginInstance->GetPluginInfo(widgetPlugin.info);

		auto iInitialize = dynamic_cast<IWidgetPluginInitialize^>(widgetPluginCLR.pluginInstance);
		if (iInitialize)
		{
			widgetPluginCLR.initializeDelegate = gcnew WidgetPlugin_CLR::InitializeDelegate(iInitialize, &IWidgetPluginInitialize::Initialize);
			widgetPlugin.functions.Initialize = (WidgetPluginFunctions::InitializeFn*) (void*) Marshal::GetFunctionPointerForDelegate(widgetPluginCLR.initializeDelegate);
		}

		auto iUpdate = dynamic_cast<IWidgetPluginUpdate^>(widgetPluginCLR.pluginInstance);
		if (iUpdate)
		{
			widgetPluginCLR.updateDelegate = gcnew WidgetPlugin_CLR::UpdateDelegate(iUpdate, &IWidgetPluginUpdate::Update);
			widgetPlugin.functions.Update = (WidgetPluginFunctions::UpdateFn*) (void*) Marshal::GetFunctionPointerForDelegate(widgetPluginCLR.updateDelegate);
		}

		auto iTeardown = dynamic_cast<IWidgetPluginTeardown^>(widgetPluginCLR.pluginInstance);
		if (iTeardown)
		{
			widgetPluginCLR.teardownDelegate = gcnew WidgetPlugin_CLR::TeardownDelegate(iTeardown, &IWidgetPluginTeardown::Teardown);
			widgetPlugin.functions.Teardown = (WidgetPluginFunctions::TeardownFn*) (void*) Marshal::GetFunctionPointerForDelegate(widgetPluginCLR.teardownDelegate);
		}

		return true;
	}

	b8
	TeardownWidgetPlugin(WidgetPlugin& widgetPlugin)
	{
		widgetPlugin.functions = {};
		widgetPluginCLR = WidgetPlugin_CLR();

		return true;
	}
};

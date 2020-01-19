#pragma unmanaged
#include "LHMAPICLR.h"

#pragma unmanaged
#include "plugin_shared.h"

#pragma managed
#pragma make_public(Plugin)
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
	b8 LoadSensorPlugin   (Plugin& plugin, SensorPlugin& sensorPlugin);
	b8 UnloadSensorPlugin (Plugin& plugin, SensorPlugin& sensorPlugin);
	b8 LoadWidgetPlugin   (Plugin& plugin, WidgetPlugin& widgetPlugin);
	b8 UnloadWidgetPlugin (Plugin& plugin, WidgetPlugin& widgetPlugin);
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
	LoadSensorPlugin(Plugin& plugin, SensorPlugin& sensorPlugin)
	{
		b8 success = LoadPlugin(plugin);
		if (!success) return false;

		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(plugin);
		success = pluginLoader.InitializeSensorPlugin(plugin, sensorPlugin);
		if (!success) return false;

		return true;
	}

	virtual b8
	UnloadSensorPlugin(Plugin& plugin, SensorPlugin& sensorPlugin)
	{
		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(plugin);
		b8 success = pluginLoader.TeardownSensorPlugin(sensorPlugin);
		if (!success) return false;

		success = UnloadPlugin(plugin);
		if (!success) return false;

		return true;
	}

	virtual b8
	LoadWidgetPlugin(Plugin& plugin, WidgetPlugin& widgetPlugin)
	{
		b8 success = LoadPlugin(plugin);
		if (!success) return false;

		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(plugin);
		success = pluginLoader.InitializeWidgetPlugin(plugin, widgetPlugin);
		if (!success) return false;

		return true;
	}

	virtual b8
	UnloadWidgetPlugin(Plugin& plugin, WidgetPlugin& widgetPlugin)
	{
		LHMPluginLoader% pluginLoader = GetDomainResidentLoader(plugin);
		b8 success = pluginLoader.TeardownWidgetPlugin(widgetPlugin);
		if (!success) return false;

		success = UnloadPlugin(plugin);
		if (!success) return false;

		return true;
	}

	LHMPluginLoader%
	GetDomainResidentLoader(Plugin plugin)
	{
		GCHandle         appDomainHandle = (GCHandle) (IntPtr) plugin.loaderData;
		AppDomain%       appDomain       = *(AppDomain^) appDomainHandle.Target;
		LHMPluginLoader% pluginLoader    = *(LHMPluginLoader^) appDomain.DomainManager;
		return pluginLoader;
	}

	b8
	LoadPlugin(Plugin& plugin)
	{
		auto name      = gcnew CLRString(plugin.fileName.data);
		auto directory = gcnew CLRString(plugin.directory.data);

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
		plugin.loaderData = (void*) (IntPtr) GCHandle::Alloc(appDomain);
		return true;
	}

	b8
	UnloadPlugin(Plugin& plugin)
	{
		GCHandle appDomainHandle = (GCHandle) (IntPtr) plugin.loaderData;

		AppDomain::Unload((AppDomain^) appDomainHandle.Target);
		appDomainHandle.Free();
		plugin.loaderData = nullptr;
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
	InitializeSensorPlugin(Plugin& plugin, SensorPlugin& sensorPlugin)
	{
		b8 success = LoadAssemblyAndInstantiateType(plugin.fileName, sensorPluginCLR.pluginInstance);
		if (!success) return false;

		String_Free(plugin.info.name);
		String_Free(plugin.info.author);

		sensorPluginCLR.pluginInstance->GetPluginInfo(plugin.info);
		sensorPlugin.functions.GetPluginInfo = nullptr;

		// DEBUG: Remove me (just for fast loading)
		#if true
		#pragma message("warning: Sensor plugin init temporarily disabled for faster loading")
		UNUSED(sensorPlugin);
		#else
		auto iInitialize = dynamic_cast<ISensorPluginInitialize^>(sensorPluginCLR.pluginInstance);
		if (iInitialize)
		{
			sensorPluginCLR.initializeDelegate = gcnew SensorPlugin_CLR::InitializeDelegate(iInitialize, &ISensorPluginInitialize::Initialize);
			sensorPlugin.functions.Initialize = (SensorPluginFunctions::InitializeFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.initializeDelegate);
		}

		auto iUpdate = dynamic_cast<ISensorPluginUpdate^>(sensorPluginCLR.pluginInstance);
		if (iUpdate)
		{
			sensorPluginCLR.updateDelegate = gcnew SensorPlugin_CLR::UpdateDelegate(iUpdate, &ISensorPluginUpdate::Update);
			sensorPlugin.functions.Update = (SensorPluginFunctions::UpdateFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.updateDelegate);
		}

		auto iTeardown = dynamic_cast<ISensorPluginTeardown^>(sensorPluginCLR.pluginInstance);
		if (iTeardown)
		{
			sensorPluginCLR.teardownDelegate = gcnew SensorPlugin_CLR::TeardownDelegate(iTeardown, &ISensorPluginTeardown::Teardown);
			sensorPlugin.functions.Teardown = (SensorPluginFunctions::TeardownFn*) (void*) Marshal::GetFunctionPointerForDelegate(sensorPluginCLR.teardownDelegate);
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
	InitializeWidgetPlugin(Plugin& plugin, WidgetPlugin& widgetPlugin)
	{
		b8 success = LoadAssemblyAndInstantiateType(plugin.fileName, widgetPluginCLR.pluginInstance);
		if (!success) return false;

		String_Free(plugin.info.name);
		String_Free(plugin.info.author);

		widgetPluginCLR.pluginInstance->GetPluginInfo(plugin.info);
		widgetPlugin.functions.GetPluginInfo = nullptr;

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

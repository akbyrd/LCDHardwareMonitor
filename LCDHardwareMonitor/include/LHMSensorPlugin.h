#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

struct SensorPlugin;
using SensorPluginRef = List<SensorPlugin>::RefT;

struct Sensor;
using SensorRef = List<Sensor>::RefT;

struct Sensor
{
	SensorRef ref;
	String    name;
	String    identifier;
	String    format;
	r32       value;

	// TODO: Might want an integer Type field with a plugin provided to-string function.
};

struct SensorPluginAPI
{
	struct Initialize
	{
		using RegisterSensorsFn = void(PluginContext&, Slice<Sensor>);

		RegisterSensorsFn* RegisterSensors;
	};

	struct Update
	{
		// NOTE: Plugins should not free Sensor members until *after* calling UnregisterSensors!

		using RegisterSensorsFn   = void(PluginContext&, Slice<Sensor>);
		using UnregisterSensorsFn = void(PluginContext&, Slice<SensorRef>);

		RegisterSensorsFn*   RegisterSensors;
		UnregisterSensorsFn* UnregisterSensors;

		Slice<Sensor> sensors;
	};

	struct Teardown
	{
		Slice<Sensor> sensors;
	};
};

struct SensorPluginFunctions
{
	using GetPluginInfoFn = void(PluginInfo& info, SensorPluginFunctions& functions);
	using InitializeFn    = b8  (PluginContext& context, SensorPluginAPI::Initialize api);
	using UpdateFn        = void(PluginContext& context, SensorPluginAPI::Update     api);
	using TeardownFn      = void(PluginContext& context, SensorPluginAPI::Teardown   api);

	GetPluginInfoFn* GetPluginInfo;
	InitializeFn*    Initialize;
	UpdateFn*        Update;
	TeardownFn*      Teardown;
};

#endif

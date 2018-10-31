#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

struct PluginContext;

struct Sensor
{
	c8* name;
	c8* identifier;
	c8* string;
	r32 value;
	// TODO: Range struct?
	r32 minValue;
	r32 maxValue;
};

struct SensorPluginAPI
{
	struct Initialize
	{
		using AddSensorFn = void(PluginContext*, Sensor);

		AddSensorFn* AddSensor;
	};

	struct Update
	{
		Slice<Sensor> sensors;
	};

	struct Teardown
	{
		Slice<Sensor> sensors;
	};
};

struct SensorPluginFunctions
{
	using GetPluginInfoFn = void(PluginInfo* info, SensorPluginFunctions* functions);
	using InitializeFn    = b32 (PluginContext* context, SensorPluginAPI::Initialize api);
	using UpdateFn        = void(PluginContext* context, SensorPluginAPI::Update     api);
	using TeardownFn      = void(PluginContext* context, SensorPluginAPI::Teardown   api);

	GetPluginInfoFn* getPluginInfo;
	InitializeFn*    initialize;
	UpdateFn*        update;
	TeardownFn*      teardown;
};

#endif

#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

struct Sensor;
using SensorRefT = List<Sensor>::RefT;

struct SensorPlugin;
using SensorPluginRefT = List<SensorPlugin>::RefT;

struct SensorRef
{
	SensorPluginRefT plugin;
	SensorRefT       sensor;

	static const SensorRef Null;
	operator b32() { return plugin && sensor; }
};

const SensorRef SensorRef::Null = {};
inline b32 operator== (SensorRef lhs, SensorRef rhs) { return lhs.plugin == rhs.plugin && lhs == rhs.sensor; }
inline b32 operator!= (SensorRef lhs, SensorRef rhs) { return lhs.plugin != rhs.plugin && lhs != rhs.sensor; }

struct Sensor
{
	c8* name;
	c8* identifier;
	c8* string;
	r32 value;
	// TODO: Range struct?
	r32 minValue;
	r32 maxValue;

	// TODO: Might want an integer Type field with a plugin provided to-string
	// function.
};

struct SensorPluginAPI
{
	struct Initialize
	{
		using AddSensorsFn = void(PluginContext*, Slice<Sensor>);

		AddSensorsFn* AddSensors;
	};

	struct Update
	{
		// NOTE: Plugins should not free Sensor members until *after* calling RemoveSensors!

		using AddSensorsFn    = void(PluginContext*, Slice<Sensor>);
		using RemoveSensorsFn = void(PluginContext*, Slice<SensorRef>);

		AddSensorsFn*    AddSensors;
		RemoveSensorsFn* RemoveSensors;

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

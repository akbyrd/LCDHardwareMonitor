#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

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

// TODO: Remove these once the API stabilizes
#define SP_INITIALIZE_ARGS struct SensorPlugin* s
#define SP_UPDATE_ARGS     struct SensorPlugin* s
#define SP_TEARDOWN_ARGS   struct SensorPlugin* s

COM_VISIBLE struct SensorPlugin
{
	using InitializeFn = void(SP_INITIALIZE_ARGS);
	using UpdateFn     = void(SP_UPDATE_ARGS);
	using TeardownFn   = void(SP_TEARDOWN_ARGS);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;

	List<Sensor>              sensors;
	//List<SensorRef>           activeSensors;
};

#endif

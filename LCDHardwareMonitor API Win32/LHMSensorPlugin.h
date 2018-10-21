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

using SensorPluginInitializeFn = void(SP_INITIALIZE_ARGS);
using SensorPluginUpdateFn     = void(SP_UPDATE_ARGS);
using SensorPluginTeardownFn   = void(SP_TEARDOWN_ARGS);

COM_VISIBLE struct SensorPlugin
{
	PluginHeaderRef           pluginHeaderRef;
	SensorPluginInitializeFn* initialize;
	SensorPluginUpdateFn*     update;
	SensorPluginTeardownFn*   teardown;

	List<Sensor>              sensors;
	//List<SensorRef>           activeSensors;
};

#endif

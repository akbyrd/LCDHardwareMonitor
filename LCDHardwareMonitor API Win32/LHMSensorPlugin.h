#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

struct Sensor
{
	c16* name;
	c16* identifier;
	c16* string;
	r32  value;
	// TODO: Range struct?
	r32  minValue;
	r32  maxValue;
};

// TODO: Remove these once the API stabilizes
#define SP_INITIALIZE_ARGS struct SensorPlugin* s
#define SP_UPDATE_ARGS     struct SensorPlugin* s
#define SP_TEARDOWN_ARGS   struct SensorPlugin* s

using SensorPluginInitializeFn = void (*)(SP_INITIALIZE_ARGS);
using SensorPluginUpdateFn     = void (*)(SP_UPDATE_ARGS);
using SensorPluginTeardownFn   = void (*)(SP_TEARDOWN_ARGS);

// TODO: Passing this directly to plugins is dangerous.
PUBLIC struct SensorPlugin
{
	PluginHeaderRef          pluginHeaderRef;
	SensorPluginInitializeFn initialize;
	SensorPluginUpdateFn     update;
	SensorPluginTeardownFn   teardown;

	List<Sensor>             sensors;
	//List<SensorRef>          activeSensors;

	void* pluginInstance;
	void* initializeDelegate;
	void* updateDelegate;
	void* teardownDelegate;
};

#endif

#ifndef LHMSENSORPLUGIN
#define LHMSENSORPLUGIN

struct SensorRef { i32 index; };
struct Sensor
{
	c16* name;
	c16* identifier;
	c16* string;
	r32  value;
	//TODO: Range struct?
	r32  minValue;
	r32  maxValue;
};

//TODO: Remove these once the API stabilizes
#define SP_INITIALIZE_ARGS struct SensorPlugin* s
#define SP_UPDATE_ARGS     struct SensorPlugin* s
#define SP_TEARDOWN_ARGS   struct SensorPlugin* s

typedef void (*SensorPluginInitializeFn)(SP_INITIALIZE_ARGS);
typedef void (*SensorPluginUpdateFn)    (SP_UPDATE_ARGS);
typedef void (*SensorPluginTeardownFn)  (SP_TEARDOWN_ARGS);

//TODO: Passing this directly to plugins is dangerous.
struct SensorPluginRef { i32 index; };
PUBLIC struct SensorPlugin
{
	PluginHeaderRef          pluginHeaderRef;
	SensorPluginInitializeFn initialize;
	SensorPluginUpdateFn     update;
	SensorPluginTeardownFn   teardown;

	List<Sensor>             sensors;
	List<SensorRef>          activeSensors;

	void* pluginInstance;
	void* initializeDelegate;
	void* updateDelegate;
	void* teardownDelegate;
};

#endif

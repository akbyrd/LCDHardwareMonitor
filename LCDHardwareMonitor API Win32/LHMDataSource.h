#ifndef LHMDATASOURCE
#define LHMDATASOURCE

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
#define DS_INITIALIZE_ARGS struct DataSource* s
#define DS_UPDATE_ARGS     struct DataSource* s
#define DS_TEARDOWN_ARGS   struct DataSource* s

typedef void (*DataSourceInitializeFn)(DS_INITIALIZE_ARGS);
typedef void (*DataSourceUpdateFn)    (DS_UPDATE_ARGS);
typedef void (*DataSourceTeardownFn)  (DS_TEARDOWN_ARGS);

struct DataSourceRef { i32 index; };
PUBLIC struct DataSource
{
	PluginHeaderRef        pluginHeaderRef;
	DataSourceInitializeFn initialize;
	DataSourceUpdateFn     update;
	DataSourceTeardownFn   teardown;

	List<Sensor>           sensors;
	List<SensorRef>        activeSensors;

	void* pluginInstance;
	void* initializeDelegate;
	void* updateDelegate;
	void* teardownDelegate;
};

#endif

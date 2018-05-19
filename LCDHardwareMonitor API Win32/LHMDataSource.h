#pragma once

PUBLIC struct Sensor
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
#define DS_INITIALIZE_ARGS
#define DS_UPDATE_ARGS
#define DS_TEARDOWN_ARGS

typedef void (*DataSourceInitializeFn)(DS_INITIALIZE_ARGS);
typedef void (*DataSourceUpdateFn)    (DS_UPDATE_ARGS);
typedef void (*DataSourceTeardownFn)  (DS_TEARDOWN_ARGS);

struct DataSource
{
	//TODO: This pointer will break when the list resizes
	PluginInfo*            pluginInfo;
	DataSourceInitializeFn initialize;
	DataSourceUpdateFn     update;
	DataSourceTeardownFn   teardown;
	List<Sensor>           sensors;
};

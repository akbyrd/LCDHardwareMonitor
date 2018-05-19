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

typedef void (*DataSourceInitializeFn)();
typedef void (*DataSourceUpdateFn)    ();
typedef void (*DataSourceTeardownFn)  ();

struct DataSource
{
	//TODO: This pointer will break when the list resizes
	PluginInfo*            pluginInfo;
	DataSourceInitializeFn initialize;
	DataSourceUpdateFn     update;
	DataSourceTeardownFn   teardown;
	List<Sensor>           sensors;
};

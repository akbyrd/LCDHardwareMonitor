#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

struct PluginContext;

struct SensorPlugin
{
	struct InitializeAPI {};
	struct UpdateAPI     {};
	struct TeardownAPI   {};

	using InitializeFn = void(PluginContext* context, InitializeAPI* api);
	using UpdateFn     = void(PluginContext* context, UpdateAPI*     api);
	using TeardownFn   = void(PluginContext* context, TeardownAPI*   api);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;

	List<Sensor>    sensors;
	//List<SensorRef> activeSensors;
};

#endif

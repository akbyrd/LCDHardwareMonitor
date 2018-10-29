#ifndef LHM_SENSORPLUGIN
#define LHM_SENSORPLUGIN

struct PluginContext;

struct SensorPluginAPI
{
	struct Initialize {};
	struct Update     {};
	struct Teardown   {};

	using InitializeFn = b32 (PluginContext* context, Initialize api);
	using UpdateFn     = void(PluginContext* context, Update     api);
	using TeardownFn   = void(PluginContext* context, Teardown   api);
};

#endif

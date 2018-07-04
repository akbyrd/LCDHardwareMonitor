#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct Widget
{
};

// TODO: Remove these once the API stabilizes
#define WP_INITIALIZE_ARGS struct WidgetPlugin* s
#define WP_UPDATE_ARGS     struct WidgetPlugin* s
#define WP_TEARDOWN_ARGS   struct WidgetPlugin* s

using WidgetPluginInitializeFn = void (*)(WP_INITIALIZE_ARGS);
using WidgetPluginUpdateFn     = void (*)(WP_UPDATE_ARGS);
using WidgetPluginTeardownFn   = void (*)(WP_TEARDOWN_ARGS);

PUBLIC struct WidgetPlugin
{
	PluginHeaderRef          pluginHeaderRef;
	WidgetPluginInitializeFn initialize;
	WidgetPluginUpdateFn     update;
	WidgetPluginTeardownFn   teardown;

	void* pluginInstance;
	void* initializeDelegate;
	void* updateDelegate;
	void* teardownDelegate;
};

#endif

#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct Widget
{
};

//TODO: Remove these once the API stabilizes
#define WP_INITIALIZE_ARGS struct WidgetPlugin* s
#define WP_UPDATE_ARGS     struct WidgetPlugin* s
#define WP_TEARDOWN_ARGS   struct WidgetPlugin* s

typedef void (*WidgetPluginInitializeFn)(WP_INITIALIZE_ARGS);
typedef void (*WidgetPluginUpdateFn)    (WP_UPDATE_ARGS);
typedef void (*WidgetPluginTeardownFn)  (WP_TEARDOWN_ARGS);

PUBLIC struct WidgetPlugin
{
	PluginHeaderRef          pluginHeaderRef;
	WidgetPluginInitializeFn initialize;
	WidgetPluginUpdateFn     update;
	WidgetPluginTeardownFn   teardown;
};

#endif

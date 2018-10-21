#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct WidgetDefinition
{
	c8* name;
	c8* author;
	u32 version;
};

// TODO: Move this somewhere common to all plugin types
struct PluginContext;
#ifndef BUILDING_LHM
	COM_VISIBLE struct PluginContext {};
#endif

// TODO: COM_VISIBLE is serving two different purposes currently. 1) allowing a
// COM exposed function to take a native type. 2) Allowing a non-COM CLR
// function to take a native type.
// TODO: Maybe remove COM_VISIBLE and use make_public() instead
COM_VISIBLE struct WidgetPluginInitializeAPI
{
	using AddWidgetDefinitionFn = b32(PluginContext*, WidgetDefinition*);

	AddWidgetDefinitionFn* AddWidgetDefinition;
};

COM_VISIBLE struct WidgetPluginUpdateAPI   { };
COM_VISIBLE struct WidgetPluginTeardownAPI { };

// TODO: Remove these once the API stabilizes
#define WP_INITIALIZE_ARGS PluginContext* context, WidgetPluginInitializeAPI* api
#define WP_UPDATE_ARGS     PluginContext* context, WidgetPluginUpdateAPI*     api
#define WP_TEARDOWN_ARGS   PluginContext* context, WidgetPluginTeardownAPI*   api

COM_VISIBLE struct WidgetPlugin
{
	using InitializeFn = void(WP_INITIALIZE_ARGS);
	using UpdateFn     = void(WP_UPDATE_ARGS);
	using TeardownFn   = void(WP_TEARDOWN_ARGS);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;
};

#endif

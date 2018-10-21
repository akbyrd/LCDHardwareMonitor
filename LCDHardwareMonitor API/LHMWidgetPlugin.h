#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct PluginContext;

struct WidgetDefinition
{
	c8* name;
	c8* author;
	u32 version;
};

struct WidgetPlugin
{
	struct InitializeAPI
	{
		using AddWidgetDefinitionFn = b32(PluginContext*, WidgetDefinition*);

		AddWidgetDefinitionFn* AddWidgetDefinition;
	};

	struct UpdateAPI   {};
	struct TeardownAPI {};

	using InitializeFn = void(PluginContext* context, InitializeAPI* api);
	using UpdateFn     = void(PluginContext* context, UpdateAPI*     api);
	using TeardownFn   = void(PluginContext* context, TeardownAPI*   api);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;
};

#endif

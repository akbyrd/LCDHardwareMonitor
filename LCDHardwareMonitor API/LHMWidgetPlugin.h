#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct PluginContext;
struct PluginHeader;
using PluginHeaderRef = List<PluginHeader>::RefT;

// TODO: Some of this is per-plugin and some of this is per-widget.
struct WidgetDefinition
{
	PluginHeaderRef ref;
	c8*             name;
	c8*             author;
	u32             version;
};

// TODO: The API structs need to be part of the public API, but the data does not.
struct WidgetPlugin
{
	struct InitializeAPI
	{
		using AddWidgetDefinitionFn = b32(PluginContext*, WidgetDefinition*);

		AddWidgetDefinitionFn* AddWidgetDefinition;
	};

	struct UpdateAPI   {};
	struct TeardownAPI {};

	// TODO: Should probably pass api by value
	using InitializeFn = b32 (PluginContext* context, InitializeAPI* api);
	using UpdateFn     = void(PluginContext* context, UpdateAPI*     api);
	using TeardownFn   = void(PluginContext* context, TeardownAPI*   api);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;
};

#endif
#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct PluginContext;

struct PluginInfo;
using PluginInfoRef = List<PluginInfo>::RefT;

struct WidgetDefinition
{
	using InitializeFn = void(Widget*);

	// Filled by plugin
	c8*           name;
	c8*           author;
	u32           version;
	u32           size;
	InitializeFn* initialize;

	// TODO: Can this be removed so plugins don't see it?
	// Filled by application
	PluginInfoRef ref;
	Bytes         instances;
};

template <typename T>
inline static void
SetWidgetDataType(WidgetDefinition* widgetDef)
{
	widgetDef->size = sizeof(T);
}

template <typename T>
inline static T*
GetWidgetData(Widget* widget)
{
	T* widgetData = (T*) (widget + 1);
	return widgetData;
}

struct WidgetPluginAPI
{
	struct Initialize
	{
		// TODO: Standardize by-value or by-ref
		using AddWidgetDefinitionFn = void       (PluginContext*, WidgetDefinition*);
		using LoadPixelShaderFn     = PixelShader(PluginContext*, c8* path, Slice<ConstantBufferDesc>);

		AddWidgetDefinitionFn* AddWidgetDefinition;
		LoadPixelShaderFn*     LoadPixelShader;
	};

	struct Update
	{
		using PushDrawCallFn  = void   (PluginContext*, DrawCall);
		using GetWVPPointerFn = Matrix*(PluginContext*);

		r32 t;
		WidgetDefinition* widgetDefinition;
		PushDrawCallFn*   PushDrawCall;
		GetWVPPointerFn*  GetWVPPointer;
	};

	struct Teardown {};
};

#endif

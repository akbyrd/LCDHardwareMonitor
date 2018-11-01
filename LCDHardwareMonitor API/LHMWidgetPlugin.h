#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct PluginContext;

struct Widget
{
	// TODO: Should plugins or the renderer be responsible for creating a matrix from this information?
	v2        position;
	//v2        scale;
	v2        pivot;
	r32       depth;
	SensorRef sensor; // TODO: Rename
};

struct WidgetDefinition
{
	using InitializeFn = void(Widget*);

	c8*           name;
	u32           size;
	InitializeFn* initialize;
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
		using AddWidgetDefinitionFn = void       (PluginContext*, WidgetDefinition);
		using LoadPixelShaderFn     = PixelShader(PluginContext*, c8* path, Slice<ConstantBufferDesc>);

		AddWidgetDefinitionFn* AddWidgetDefinition;
		LoadPixelShaderFn*     LoadPixelShader;
	};

	struct Update
	{
		using PushDrawCallFn  = void   (PluginContext*, DrawCall);
		using GetWVPPointerFn = Matrix*(PluginContext*);

		r32 t;
		// TODO: Bytes is a lame data structure here. Strongly typed Slice with a
		// stride?! Also, I think we need to pass the definitions along with the
		// instances or else the plugin won't know what it's updating/drawing.
		// This likely informs the resolution to the teardown issue mentioned
		// below.
		Bytes             widgetInstances;
		Slice<Sensor>     sensors;
		PushDrawCallFn*   PushDrawCall;
		GetWVPPointerFn*  GetWVPPointer;
	};

	struct Teardown
	{
		// TODO: Need to do teardown for multiple WidgetTypes because the plugin
		// might need to release resources on every widget instance. We don't
		// want to call teardown multiple times though so this needs some
		//thought.
		//Bytes widgetInstances;
	};
};

struct WidgetPluginFunctions
{
	using GetPluginInfoFn = void(PluginInfo* info, WidgetPluginFunctions* functions);
	using InitializeFn    = b32 (PluginContext* context, WidgetPluginAPI::Initialize api);
	using UpdateFn        = void(PluginContext* context, WidgetPluginAPI::Update     api);
	using TeardownFn      = void(PluginContext* context, WidgetPluginAPI::Teardown   api);

	GetPluginInfoFn* getPluginInfo;
	InitializeFn*    initialize;
	UpdateFn*        update;
	TeardownFn*      teardown;
};

#endif

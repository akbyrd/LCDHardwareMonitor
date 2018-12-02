#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct Widget
{
	// TODO: Should plugins or the renderer be responsible for creating a matrix from this information?
	v2        position;
	//v2        scale;
	v2        pivot;
	r32       depth;
	SensorRef sensorRef;
	//u8        data[1];
};

struct WidgetInstanceAPI
{
	struct Initialize
	{
		Slice<Widget> widgets;
		Slice<u8>     widgetData;
	};

	struct Update
	{
		using PushDrawCallFn  = void   (PluginContext*, DrawCall);
		using GetWVPPointerFn = Matrix*(PluginContext*);

		r32              t;
		Slice<Widget>    widgets;
		Slice<u8>        widgetData;
		Slice<Sensor>    sensors;
		PushDrawCallFn*  PushDrawCall;
		GetWVPPointerFn* GetWVPPointer;
	};

	struct Teardown
	{
		Slice<Widget> widgets;
		Slice<u8>     widgetData;
	};
};

// TODO: Consider renaming this to WidgetDescription
struct WidgetDefinition
{
	using InitializeFn = b32 (PluginContext*, WidgetInstanceAPI::Initialize);
	using UpdateFn     = void(PluginContext*, WidgetInstanceAPI::Update);
	using TeardownFn   = void(PluginContext*, WidgetInstanceAPI::Teardown);

	c8*           name;
	u32           size;
	InitializeFn* initialize;
	UpdateFn*     update;
	TeardownFn*   teardown;
};

struct WidgetPluginAPI
{
	struct Initialize
	{
		using AddWidgetDefinitionsFn = void       (PluginContext*, Slice<WidgetDefinition>);
		using LoadPixelShaderFn      = PixelShader(PluginContext*, c8* relPath, Slice<ConstantBufferDesc>);

		AddWidgetDefinitionsFn* AddWidgetDefinitions;
		LoadPixelShaderFn*      LoadPixelShader;
	};

	struct Update   {};
	struct Teardown {};
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

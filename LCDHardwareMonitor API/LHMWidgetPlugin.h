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
	//u8        data[1];
};

struct WidgetInstancesAPI
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

struct WidgetDefinition
{
	using InitializeFn = b32 (PluginContext*, WidgetInstancesAPI::Initialize);
	using UpdateFn     = void(PluginContext*, WidgetInstancesAPI::Update);
	using TeardownFn   = void(PluginContext*, WidgetInstancesAPI::Teardown);

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
		using AddWidgetDefinitionFn = void       (PluginContext*, WidgetDefinition);
		using LoadPixelShaderFn     = PixelShader(PluginContext*, c8* path, Slice<ConstantBufferDesc>);

		AddWidgetDefinitionFn* AddWidgetDefinition;
		LoadPixelShaderFn*     LoadPixelShader;
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

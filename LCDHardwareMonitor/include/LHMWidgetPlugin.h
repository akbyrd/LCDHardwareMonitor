#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct WidgetPluginRef : Index {};
struct WidgetRef       : Index {};

struct Widget
{
	WidgetRef       ref;
	v2              position;
	v2              size;
	v2              pivot;
	r32             depth;
	SensorPluginRef sensorPluginRef;
	SensorRef       sensorRef;
};

struct WidgetAPI
{
	struct Initialize
	{
		Slice<Widget> widgets;
		ByteSlice     widgetsUserData;
	};

	struct Update
	{
		using GetViewMatrixFn           = Matrix(PluginContext&);
		using GetProjectionMatrixFn     = Matrix(PluginContext&);
		using GetViewProjectionMatrixFn = Matrix(PluginContext&);
		using UpdateVSConstantBufferFn  = void  (PluginContext&, VertexShader, u32 index, void* data);
		using UpdatePSConstantBufferFn  = void  (PluginContext&, PixelShader, u32 index, void* data);
		using DrawMeshFn                = void  (PluginContext&, Mesh);
		using PushVertexShaderFn        = void  (PluginContext&, VertexShader);
		using PushPixelShaderFn         = void  (PluginContext&, PixelShader);
		using PopVertexShaderFn         = void  (PluginContext&);
		using PopPixelShaderFn          = void  (PluginContext&);

		r32                        t;
		Slice<Widget>              widgets;
		ByteSlice                  widgetsUserData;
		Slice<Sensor>              sensors;
		GetViewMatrixFn*           GetViewMatrix;
		GetProjectionMatrixFn*     GetProjectionMatrix;
		GetViewProjectionMatrixFn* GetViewProjectionMatrix;
		UpdateVSConstantBufferFn*  UpdateVSConstantBuffer;
		UpdatePSConstantBufferFn*  UpdatePSConstantBuffer;
		DrawMeshFn*                DrawMesh;
		PushVertexShaderFn*        PushVertexShader;
		PushPixelShaderFn*         PushPixelShader;
		PopVertexShaderFn*         PopVertexShader;
		PopPixelShaderFn*          PopPixelShader;
	};

	struct Teardown
	{
		Slice<Widget> widgets;
		ByteSlice     widgetsUserData;
	};
};

struct WidgetDescRef : Index {};

struct WidgetDesc
{
	using InitializeFn = b8  (PluginContext&, WidgetAPI::Initialize);
	using UpdateFn     = void(PluginContext&, WidgetAPI::Update);
	using TeardownFn   = void(PluginContext&, WidgetAPI::Teardown);

	WidgetDescRef ref;
	String        name;
	u32           userDataSize;
	InitializeFn* Initialize;
	UpdateFn*     Update;
	TeardownFn*   Teardown;
};

struct WidgetPluginAPI
{
	struct Initialize
	{
		using RegisterWidgetsFn = void       (PluginContext&, Slice<WidgetDesc>);
		using LoadPixelShaderFn = PixelShader(PluginContext&, StringView relPath, Slice<u32> cBufSizes);

		RegisterWidgetsFn* RegisterWidgets;
		LoadPixelShaderFn* LoadPixelShader;
	};

	struct Update   {};
	struct Teardown {};
};

struct WidgetPluginFunctions
{
	using GetPluginInfoFn = void(PluginDesc& desc, WidgetPluginFunctions& functions);
	using InitializeFn    = b8  (PluginContext& context, WidgetPluginAPI::Initialize api);
	using UpdateFn        = void(PluginContext& context, WidgetPluginAPI::Update     api);
	using TeardownFn      = void(PluginContext& context, WidgetPluginAPI::Teardown   api);

	GetPluginInfoFn* GetPluginInfo;
	InitializeFn*    Initialize;
	UpdateFn*        Update;
	TeardownFn*      Teardown;
};

inline v2
WidgetPosition(Widget widget)
{
	// TODO: If feels like there are 2 different desired behaviors here.
	// 1) Cancel out the mesh pivot and use the widget pivot. Widget pivot is always relative to the same place.
	// 2) Don't cancel out the mesh pivot. Widget pivot is always relative to the mesh pivot.
	// TODO: Actually get this from the mesh
	// NOTE: This *cancels out* the mesh pivot.
	//v2 meshPivot = { 0.5f, 0.5f };
	v2 meshPivot = {};

	v2 result = widget.position + (meshPivot + widget.pivot) * widget.size;
	return result;
}

inline v4
WidgetRect(Widget widget)
{
	// TODO: Actually get this from the mesh
	v2 meshPivot = { 0.5f, 0.5f };

	v4 result = {};
	result.pos = widget.position - (meshPivot + widget.pivot) * widget.size;
	result.size = widget.size;
	return result;
}

#endif

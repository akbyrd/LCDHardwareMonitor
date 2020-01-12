#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN
namespace LCDHardwareMonitor {

struct WidgetPlugin;
using WidgetPluginRef = List<WidgetPlugin>::RefT;

struct Widget;
using WidgetRef = List<Widget>::RefT;

struct Widget
{
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
		using PushConstantBufferUpdateFn = void  (PluginContext&, Material, ShaderStage, u32 index, void* data);

		Slice<Widget>               widgets;
		ByteSlice                   widgetsUserData;
		PushConstantBufferUpdateFn* PushConstantBufferUpdate;
	};

	struct Update
	{
		using GetViewMatrixFn            = Matrix(PluginContext&);
		using GetProjectionMatrixFn      = Matrix(PluginContext&);
		using GetViewProjectionMatrixFn  = Matrix(PluginContext&);
		using PushConstantBufferUpdateFn = void  (PluginContext&, Material, ShaderStage, u32 index, void* data);
		using PushDrawCallFn             = void  (PluginContext&, Material);

		r32                         t;
		Slice<Widget>               widgets;
		ByteSlice                   widgetsUserData;
		Slice<Sensor>               sensors;
		GetViewMatrixFn*            GetViewMatrix;
		GetProjectionMatrixFn*      GetProjectionMatrix;
		GetViewProjectionMatrixFn*  GetViewProjectionMatrix;
		PushConstantBufferUpdateFn* PushConstantBufferUpdate;
		PushDrawCallFn*             PushDrawCall;
	};

	struct Teardown
	{
		Slice<Widget> widgets;
		ByteSlice     widgetsUserData;
	};
};

struct WidgetDesc;
using WidgetDescRef = List<WidgetDesc>::RefT;

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
		using CreateMaterialFn  = Material   (PluginContext&, Mesh, VertexShader, PixelShader);

		RegisterWidgetsFn* RegisterWidgets;
		LoadPixelShaderFn* LoadPixelShader;
		CreateMaterialFn*  CreateMaterial;
	};

	struct Update   {};
	struct Teardown {};
};

struct WidgetPluginFunctions
{
	using GetPluginInfoFn = void(PluginInfo& info, WidgetPluginFunctions& functions);
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
	// TODO: Actually get this from the mesh
	v2 meshPivot = { 0.5f, 0.5f };

	v2 result = widget.position + (meshPivot - widget.pivot) * widget.size;
	return result;
}

inline v4
WidgetRect(Widget widget)
{
	v4 result = {};
	result.pos = widget.position - widget.pivot * widget.size;
	result.size = widget.size;
	return result;
}

}
#endif

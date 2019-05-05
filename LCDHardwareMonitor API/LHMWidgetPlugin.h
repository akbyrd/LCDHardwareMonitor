#ifndef LHM_WIDGETPLUGIN
#define LHM_WIDGETPLUGIN

struct WidgetPlugin;
using WidgetPluginRef = List<WidgetPlugin>::RefT;

struct Widget
{
	v2              position;
	//v2              scale;
	v2              pivot;
	r32             depth;
	SensorPluginRef sensorPluginRef;
	SensorRef       sensorRef;
	//u8              data[1];
};

struct WidgetInstanceAPI
{
	struct Initialize
	{
		using PushConstantBufferUpdateFn = void  (PluginContext*, Material, ShaderStage, u32 index, void* data);

		Slice<Widget>               widgets;
		ByteSlice                   widgetsUserData;
		PushConstantBufferUpdateFn* PushConstantBufferUpdate;
	};

	struct Update
	{
		using GetViewMatrixFn            = Matrix(PluginContext*);
		using GetProjectionMatrixFn      = Matrix(PluginContext*);
		using GetViewProjectionMatrixFn  = Matrix(PluginContext*);
		using PushConstantBufferUpdateFn = void  (PluginContext*, Material, ShaderStage, u32 index, void* data);
		using PushDrawCallFn             = void  (PluginContext*, Material);

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
	using InitializeFn = b32 (PluginContext*, WidgetInstanceAPI::Initialize);
	using UpdateFn     = void(PluginContext*, WidgetInstanceAPI::Update);
	using TeardownFn   = void(PluginContext*, WidgetInstanceAPI::Teardown);

	WidgetDescRef ref;
	String        name;
	u32           userDataSize;
	InitializeFn* initialize;
	UpdateFn*     update;
	TeardownFn*   teardown;
};

struct WidgetPluginAPI
{
	struct Initialize
	{
		using RegisterWidgetsFn = void       (PluginContext*, Slice<WidgetDesc>);
		using LoadPixelShaderFn = PixelShader(PluginContext*, c8* relPath, Slice<u32> cBufSizes);
		using CreateMaterialFn  = Material   (PluginContext*, Mesh, VertexShader, PixelShader);

		RegisterWidgetsFn* RegisterWidgets;
		LoadPixelShaderFn* LoadPixelShader;
		CreateMaterialFn*  CreateMaterial;
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

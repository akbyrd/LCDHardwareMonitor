#include "LHMAPI.h"
#include "WVP.vs.h"
#include "Filled Bar.ps.h"

struct BarWidget
{
	v2u          size;
	r32          borderSize;
	r32          borderBlur;
	VSPerObject  vsPerObject;
	PSInitialize psInitialize;
	PSPerObject  psPerObject;
};

static Material filledBarMat = {};

static b32
InitializeBarWidgets(PluginContext& context, WidgetInstanceAPI::Initialize api)
{
	for (u32 i = 0; i < api.widgets.length; i++)
	{
		//Widget&    widget    = api.widgets[i];
		BarWidget& barWidget = (BarWidget&) api.widgetsUserData[i];

		barWidget.size       = { 240, 12 };
		barWidget.borderSize = 1.0f;
		barWidget.borderBlur = 0.0f;

		v2 pixelsPerUV = 1.0f / (v2) barWidget.size;
		barWidget.psInitialize.borderSizeUV    = barWidget.borderSize * pixelsPerUV;
		barWidget.psInitialize.borderBlurUV    = barWidget.borderBlur * pixelsPerUV;
		barWidget.psInitialize.borderColor     = Color32(47, 112, 22, 255);
		barWidget.psInitialize.fillColor       = Color32(47, 112, 22, 255);
		barWidget.psInitialize.backgroundColor = Color32( 0,   0,  0, 255);

		api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Pixel,  0, &barWidget.psInitialize);
	}
	return true;
}

static void
UpdateBarWidgets(PluginContext& context, WidgetInstanceAPI::Update api)
{
	for (u32 i = 0; i < api.widgets.length; i++)
	{
		Widget&    widget    = api.widgets[i];
		BarWidget& barWidget = (BarWidget&) api.widgetsUserData[i];

		// Update
		if (widget.sensorRef)
		{
			Sensor& sensor = api.sensors[widget.sensorRef];
			float value = sensor.value / 100.0f;
			barWidget.psPerObject.fillAmount = Lerp(barWidget.psPerObject.fillAmount, value, 0.10f);
		}
		else
		{
			r32 phase = (r32) i / (r32) (api.widgets.length + 1) * 0.5f * r32Pi;
			barWidget.psPerObject.fillAmount = sin(api.t + phase) * sin(api.t + phase);
		}

		// Draw
		{
			Matrix world = Identity();

			v2 size = (v2) barWidget.size;
			v2 position = widget.position;
			// TODO: Add a SetPosition that takes a pivot and size
			position += (v2{ 0.5f, 0.5f } - widget.pivot) * size;
			SetPosition(world, position, -widget.depth);
			SetScale   (world, size, 1.0f);
			barWidget.vsPerObject.wvp = world * api.GetViewProjectionMatrix(context);

			api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Vertex, 0, &barWidget.vsPerObject);
			api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Pixel,  1, &barWidget.psPerObject);
			api.PushDrawCall(context, filledBarMat);
		}
	}

	// DEBUG: Background Quad
	{
		v3 pos   = { 160.0f, 120.0f, -10.0f };
		v3 scale = { 244.0f,  76.0f,   1.0f };
		Matrix world = GetST(scale, pos);

		static Matrix wvp;
		wvp = world * api.GetViewProjectionMatrix(context);

		Material material = {};
		material.mesh = StandardMesh::Quad;
		material.vs   = StandardVertexShader::WVP;
		material.ps   = StandardPixelShader::VertexColored;

		api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Vertex, 0, &wvp);
		api.PushDrawCall(context, material);
	}
}

static b32
Initialize(PluginContext& context, WidgetPluginAPI::Initialize api)
{
	// TODO: Shouldn't have to do this on the user side
	String name = {};
	b32 success = String_Format(name, "Filled Bar");
	if (!success) return false;

	WidgetDesc widgetDesc = {};
	widgetDesc.name         = name;
	widgetDesc.userDataSize = sizeof(BarWidget);
	widgetDesc.Initialize   = InitializeBarWidgets;
	widgetDesc.Update       = UpdateBarWidgets;
	api.RegisterWidgets(context, widgetDesc);

	u32 cBufSizes[] = {
		{ sizeof(PSInitialize) },
		{ sizeof(PSPerObject)  }
	};
	PixelShader ps = api.LoadPixelShader(context, "Filled Bar.ps.cso", cBufSizes);

	filledBarMat = api.CreateMaterial(context, StandardMesh::Quad, StandardVertexShader::WVP, ps);
	return true;
}

EXPORT void
GetWidgetPluginInfo(PluginInfo& info, WidgetPluginFunctions& functions)
{
	info.name    = "Filled Bar";
	info.author  = "akbyrd";
	info.version = 1;

	functions.Initialize = Initialize;
}

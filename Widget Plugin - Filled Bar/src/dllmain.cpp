#include "LHMAPI.h"
#include "Vertex Shader - WVP.h"
#include "Pixel Shader - Filled Bar.h"

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
InitializeBarWidgets(PluginContext* context, WidgetInstanceAPI::Initialize api)
{
	for (u32 i = 0; i < api.widgets.length; i++)
	{
		//Widget*    widget    = &api.widgets[i];
		BarWidget* barWidget = (BarWidget*) &api.widgetsUserData[i];

		barWidget->size       = { 240, 12 };
		barWidget->borderSize = 1.0f;
		barWidget->borderBlur = 0.0f;

		v2 pixelsPerUV = 1.0f / (v2) barWidget->size;
		barWidget->psInitialize.borderSizeUV    = barWidget->borderSize * pixelsPerUV;
		barWidget->psInitialize.borderBlurUV    = barWidget->borderBlur * pixelsPerUV;
		barWidget->psInitialize.borderColor     = Color32(47, 112, 22, 255);
		barWidget->psInitialize.fillColor       = Color32(47, 112, 22, 255);
		barWidget->psInitialize.backgroundColor = Color32( 0,   0,  0, 255);

		api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Pixel,  0, &barWidget->psInitialize);
	}
	return true;
}

static void
UpdateBarWidgets(PluginContext* context, WidgetInstanceAPI::Update api)
{
	for (u32 i = 0; i < api.widgets.length; i++)
	{
		Widget*    widget    = &api.widgets[i];
		BarWidget* barWidget = (BarWidget*) &api.widgetsUserData[i];

		// Update
		if (widget->sensorRef)
		{
			Sensor* sensor = &api.sensors[widget->sensorRef.sensor];
			float value = sensor->value / 100.0f;
			barWidget->psPerObject.fillAmount = Lerp(barWidget->psPerObject.fillAmount, value, 0.10f);
		}
		else
		{
			r32 phase = (r32) i / (r32) (api.widgets.length + 1) * 0.5f * r32Pi;
			barWidget->psPerObject.fillAmount = sin(api.t + phase) * sin(api.t + phase);
		}

		// Draw
		{
			Matrix world = Identity();

			v2 size = (v2) barWidget->size;
			v2 position = widget->position;
			// TODO: Add a SetPosition that takes a pivot and size
			position += (v2{ 0.5f, 0.5f } - widget->pivot) * size;
			SetPosition(world, position, -widget->depth);
			SetScale   (world, size, 1.0f);
			barWidget->vsPerObject.wvp = world * api.GetViewProjectionMatrix(context);

			api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Vertex, 0, &barWidget->vsPerObject);
			api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Pixel,  1, &barWidget->psPerObject);
			api.PushDrawCall(context, filledBarMat);
		}
	}
}

static b32
Initialize(PluginContext* context, WidgetPluginAPI::Initialize api)
{
	WidgetDesc widgetDesc = {};
	widgetDesc.name         = "Filled Bar";
	widgetDesc.userDataSize = sizeof(BarWidget);
	widgetDesc.initialize   = &InitializeBarWidgets;
	widgetDesc.update       = &UpdateBarWidgets;
	api.RegisterWidgets(context, widgetDesc);

	u32 cBufSizes[] = {
		{ sizeof(PSInitialize) },
		{ sizeof(PSPerObject)  }
	};
	PixelShader ps = api.LoadPixelShader(context, "Pixel Shader - Filled Bar.cso", cBufSizes);

	filledBarMat = api.CreateMaterial(context, StandardMesh::Quad, StandardVertexShader::WVP, ps);
	return true;
}

EXPORT void
GetWidgetPluginInfo(PluginInfo* info, WidgetPluginFunctions* functions)
{
	info->name    = "Filled Bar";
	info->author  = "akbyrd";
	info->version = 1;

	functions->initialize = Initialize;
}

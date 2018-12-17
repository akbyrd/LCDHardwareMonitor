#include "LHMAPI.h"
#include "Filled Bar Constant Buffer.h"

// TODO: Move to a shared header
struct VSConstants
{
	Matrix wvp;
};

struct BarWidget
{
	v2u         size;
	r32         borderSize;
	r32         borderBlur;
	VSConstants vsConstants;
	PSConstants psConstants;
};

static Material filledBarMat = {};

static b32
InitializeBarWidgets(PluginContext* context, WidgetInstanceAPI::Initialize api)
{
	UNUSED(context);

	for (u32 i = 0; i < api.widgets.length; i++)
	{
		//Widget*    widget    = &api.widgets[i];
		BarWidget* barWidget = (BarWidget*) &api.widgetsUserData[i];

		barWidget->size       = { 240, 12 };
		barWidget->borderSize = 1.0f;
		barWidget->borderBlur = 0.0f;

		v2 pixelsPerUV = 1.0f / (v2) barWidget->size;
		barWidget->psConstants.borderSizeUV    = barWidget->borderSize * pixelsPerUV;
		barWidget->psConstants.borderBlurUV    = barWidget->borderBlur * pixelsPerUV;
		barWidget->psConstants.borderColor     = Color32(47, 112, 22, 255);
		barWidget->psConstants.fillColor       = Color32(47, 112, 22, 255);
		barWidget->psConstants.backgroundColor = Color32( 0,   0,  0, 255);
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
			barWidget->psConstants.fillAmount = Lerp(barWidget->psConstants.fillAmount, value, 0.10f);
		}
		else
		{
			r32 phase = (r32) i / (r32) (api.widgets.length + 1) * 0.5f * r32Pi;
			barWidget->psConstants.fillAmount = sin(api.t + phase) * sin(api.t + phase);
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
			barWidget->vsConstants.wvp = world * api.GetViewProjectionMatrix(context);

			api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Vertex, 0, &barWidget->vsConstants);
			api.PushConstantBufferUpdate(context, filledBarMat, ShaderStage::Pixel,  0, &barWidget->psConstants);
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

	ConstantBufferDesc cBufDesc = {};
	cBufDesc.size = sizeof(PSConstants);
	PixelShader ps = api.LoadPixelShader(context, "Filled Bar Pixel Shader.cso", cBufDesc);

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

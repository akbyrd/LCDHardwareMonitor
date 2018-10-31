#include "LHMAPI.h"
#include "Filled Bar Constant Buffer.h"

struct BarWidget
{
	v2u         size;
	r32         borderSize;
	r32         borderBlur;
	PSConstants constants;
};

// TODO: Standardize plugin functions
static void
InitializeBarWidget(Widget* widget)
{
	BarWidget* barWidget = GetWidgetData<BarWidget>(widget);

	barWidget->size       = { 240, 12 };
	barWidget->borderSize = 1.0f;
	barWidget->borderBlur = 0.0f;

	v2 pixelsPerUV = 1.0f / (v2) barWidget->size;
	barWidget->constants.borderSizeUV    = barWidget->borderSize * pixelsPerUV;
	barWidget->constants.borderBlurUV    = barWidget->borderBlur * pixelsPerUV;
	barWidget->constants.borderColor     = Color32(47, 112, 22, 255);
	barWidget->constants.fillColor       = Color32(47, 112, 22, 255);
	barWidget->constants.backgroundColor = Color32( 0,   0,  0, 255);
}

// TODO: Allocate in an application supplied arena.
static PixelShader filledBarPS = {};

static void
DrawBarWidget(PluginContext* context, WidgetPluginAPI::Update api, Widget* widget, BarWidget* barWidget)
{
	DrawCall dc = {};
	dc.mesh = StandardMesh::Quad;

	v2 size = (v2) barWidget->size;
	v2 position = widget->position;

	position += (v2{ 0.5f, 0.5f } - widget->pivot) * size;
	v3 position3 = { position.x, position.y, -widget->depth };

	SetPosition(dc.world, position3);
	SetScale   (dc.world, size);

	dc.vs               = StandardVertexShader::Debug;
	dc.cBufPerObjDataVS = api.GetWVPPointer(context);

	dc.ps               = filledBarPS;
	dc.cBufPerObjDataPS = &barWidget->constants;

	api.PushDrawCall(context, dc);
}

b32
Initialize(PluginContext* context, WidgetPluginAPI::Initialize api)
{
	WidgetDefinition widgetDef = {};
	widgetDef.name       = "Filled Bar";
	widgetDef.size       = sizeof(BarWidget);
	widgetDef.initialize = &InitializeBarWidget;
	api.AddWidgetDefinition(context, widgetDef);

	// TODO: This feels janky. Would like to unify the handling of cbufs in some way
	ConstantBufferDesc cBufDesc = {};
	cBufDesc.size      = sizeof(PSConstants);
	cBufDesc.frequency = ConstantBufferFrequency::PerObject;

	filledBarPS = api.LoadPixelShader(context, "Filled Bar Pixel Shader.cso", cBufDesc);
	return true;
}

void
Update(PluginContext* context, WidgetPluginAPI::Update api)
{
	// HACK: Nasty, hard-coded fuckery
	u32 elemSize = sizeof(Widget) + sizeof(BarWidget);
	u32 instanceCount = api.widgetInstances.length / elemSize;
	for (u32 i = 0; i < instanceCount; i++)
	{
		Widget* widget = (Widget*) &api.widgetInstances[i * elemSize];
		BarWidget* barWidget = (BarWidget*) ((u8*) widget + sizeof(Widget));

		if (widget->sensor)
		{
			Sensor* sensor = &api.sensors[widget->sensor];
			float value = sensor->value / 100.0f;
			barWidget->constants.fillAmount = Lerp(barWidget->constants.fillAmount, value, 0.10f);
		}
		else
		{
			r32 phase = (r32) i / (r32) (instanceCount + 1) * 0.5f * r32Pi;
			barWidget->constants.fillAmount = sin(api.t + phase) * sin(api.t + phase);
		}

		DrawBarWidget(context, api, widget, barWidget);
	}
}

void
Teardown(PluginContext* context, WidgetPluginAPI::Teardown api)
{
	UNUSED(context); UNUSED(api);
}

EXPORT void
GetWidgetPluginInfo(PluginInfo* info, WidgetPluginFunctions* functions)
{
	info->name    = "Filled Bar";
	info->author  = "akbyrd";
	info->version = 1;

	functions->initialize = Initialize;
	functions->update     = Update;
	functions->teardown   = Teardown;
}

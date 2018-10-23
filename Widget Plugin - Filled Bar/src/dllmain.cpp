#include "LHMAPI.h"
#include "Filled Bar Constant Buffer.h"

struct BarWidget
{
	v2u size;
};

// TODO: Standardize plugin functions
static void
InitializeBarWidget(Widget* widget)
{
	BarWidget* barWidget = GetWidgetData<BarWidget>(widget);
	barWidget->size = { 240, 12 };
}

// TODO: Allocate in an application supplied arena.
static PixelShader filledBarPS = {};
static PSConstants psConstants = {};

static void
DrawBarWidget(PluginContext* context, WidgetPlugin::UpdateAPI api, Widget* widget, BarWidget* barWidget)
{
	DrawCall dc = {};
	dc.mesh       = StandardMesh::Quad;
	dc.vs         = StandardVertexShader::Debug;
	dc.ps         = filledBarPS;
	dc.world.sx  = r32(barWidget->size.x);
	dc.world.sy  = r32(barWidget->size.y);
	dc.world.sz  = 1.0f;
	dc.world.m33 = 1.0f;
	dc.world.tx  = r32(widget->position.x);
	dc.world.ty  = r32(widget->position.y);
	api.PushDrawCall(context, dc);
}

EXPORT b32
Initialize(PluginContext* context, WidgetPlugin::InitializeAPI api)
{
	WidgetDefinition widgetDef = {};
	widgetDef.name       = "Filled Bar";
	widgetDef.author     = "akbyrd";
	widgetDef.version    = 1;
	widgetDef.size       = sizeof(BarWidget);
	widgetDef.initialize = &InitializeBarWidget;
	api.AddWidgetDefinition(context, &widgetDef);

	ConstantBufferDesc cBufDesc = {};
	cBufDesc.size      = sizeof(PSConstants);
	cBufDesc.data      = &psConstants;
	cBufDesc.frequency = ConstantBufferFrequency::PerObject;

	filledBarPS = api.LoadPixelShader(context, "Filled Bar Pixel Shader.cso", cBufDesc);
	return true;
}

// TODO: I don't think drawing belongs in update
EXPORT void
Update(PluginContext* context, WidgetPlugin::UpdateAPI api)
{
	psConstants.fillAmount = sin(api.t) * sin(api.t);

	// HACK: Nasty, hard-coded fuckery
	u32 elemSize = sizeof(Widget) + sizeof(BarWidget);
	u32 instanceCount = api.widgetDefinition->instances.length / elemSize;
	for (u32 i = 0; i < instanceCount; i++)
	{
		Widget* widget = (Widget*) &api.widgetDefinition->instances[i * elemSize];
		BarWidget* barWidget = (BarWidget*) ((u8*) widget + sizeof(Widget));
		DrawBarWidget(context, api, widget, barWidget);
	}
}

EXPORT void
Teardown(PluginContext* context, WidgetPlugin::TeardownAPI api)
{
	UNUSED(context); UNUSED(api);
}

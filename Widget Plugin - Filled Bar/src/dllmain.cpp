#include "LHMAPI.h"

#if false
struct Widget
{
	__declspec(align(16))
	struct VSConstants
	{
	};

	__declspec(align(16))
	struct PSConstants
	{
		// TODO: How do we get this? (Do the UV calculation in CPU, I think)
		v2i   res;

		v4    borderColor = { 1.0f, 0.0f, 0.0f, 1.0f };
		float borderSize  = 20.0f;
		float borderBlur  = 20.0f;

		v4    fillColor  = { 1.0f, 1.0f, 1.0f, 0.5f };
		float fillAmount = 0.5f;
		float fillBlur   = 10.0f;

		v4    backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
	};

	//SensorRef sensorRef;
	//Mesh      mesh;
	v2i       position;
	v2i       size;

	VSConstants vsConstants;
	PSConstants psConstants;

	// TODO: Maybe string and range overrides?
	// TODO: drawFn? Widget type to function map?
};

void
DrawWidget_FilledBar(Widget& w /*, RendererState* renderer */)
{
	UNUSED(w);
	// Set Input Layout
	// Set Vertex Shader
	// Set VS constant buffer
	// Set Pixel Shader
	// Set PS constant buffer
}

//using ConstantBufferType = FilledBarConstants;
#endif

struct BarWidget
{
	// TODO: Unsigned vector type
	v2i size;
};

// TODO: Standardize plugin functions
static void
InitializeBarWidget(Widget* widget)
{
	BarWidget* barWidget = GetWidgetData<BarWidget>(widget);
	barWidget->size = { 240, 12 };
}

// TODO: Assert alignment in API
// TODO: Assert size in API
__declspec(align(16))
#pragma pack(push)
#pragma pack(4)
struct PSConstants
{
	// TODO: How do we get this? (Do the UV calculation in CPU, I think)
	v2  res = { 240, 12 };
	r32 padding1[2];

	v4  borderColor = Color32(47, 112, 22, 255);
	r32 borderSize  = 1.0f;
	r32 borderBlur  = 0.0f;
	r32 padding2[2];

	v4  fillColor  = Color32(47, 112, 22, 255);;
	r32 fillAmount = 0.5f;
	r32 fillBlur   = 0.0f;
	r32 padding3[2];

	v4 backgroundColor = { 0.0f, 0.0f, 0.0f, 1.0f };
};
#pragma pack(pop)

// TODO: Allocate in an application supplied arena.
static PixelShader filledBarPS = {};
static PSConstants psConstants = {};

static void
DrawBarWidget(PluginContext* context, WidgetPlugin::UpdateAPI api, Widget* widget, BarWidget* barWidget)
{
	DrawCall dc = {};
	dc.mesh         = StandardMesh::Quad;
	dc.vs           = StandardVertexShader::Debug;
	dc.ps           = filledBarPS;
	dc.worldM[0][0] = r32(barWidget->size.x);
	dc.worldM[1][1] = r32(barWidget->size.y);
	dc.worldM[2][2] = 1.0f;
	dc.worldM[3][3] = 1.0f;
	dc.worldM[3][0] = r32(widget->position.x);
	dc.worldM[3][1] = r32(widget->position.y);
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

	ConstantBufferDesc cBufferDesc = {};
	cBufferDesc.size = sizeof(PSConstants);
	cBufferDesc.data = &psConstants;
	filledBarPS = api.LoadPixelShader(context, "Filled Bar Pixel Shader.cso", cBufferDesc);
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

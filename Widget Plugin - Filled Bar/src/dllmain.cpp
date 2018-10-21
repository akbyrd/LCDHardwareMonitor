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

EXPORT
b32
Initialize(PluginContext* context, WidgetPlugin::InitializeAPI* api)
{
	WidgetDefinition widgetDef = {};
	widgetDef.name    = "Filled Bar";
	widgetDef.author  = "akbyrd";
	widgetDef.version = 1;
	api->AddWidgetDefinition(context, &widgetDef);
	return true;
}

EXPORT void Update   (PluginContext* context, WidgetPlugin::UpdateAPI*   api) { UNUSED(context); UNUSED(api); }
EXPORT void Teardown (PluginContext* context, WidgetPlugin::TeardownAPI* api) { UNUSED(context); UNUSED(api); }

#include "LHMAPI.h"
#include "WVP.vs.h"
#include "Filled Bar.ps.h"

struct BarWidget
{
	r32         borderSize;
	r32         borderBlur;
	VSPerObject vsPerObject;
	PSPerObject psPerObject;
};

static PixelShader filledBarPS = {};

static b8
InitializeBarWidgets(PluginContext& context, WidgetAPI::Initialize api)
{
	Unused(context);

	for (u32 i = 0; i < api.widgets.length; i++)
	{
		Widget&    widget    = api.widgets[i];
		BarWidget& barWidget = (BarWidget&) api.widgetsUserData[i];

		widget.size = { 240, 12 };

		barWidget.borderSize = 1.0f;
		barWidget.borderBlur = 0.0f;

		v2 pixelsPerUV = 1.0f / widget.size;
		barWidget.psPerObject.borderSizeUV    = barWidget.borderSize * pixelsPerUV;
		barWidget.psPerObject.borderBlurUV    = barWidget.borderBlur * pixelsPerUV;
		barWidget.psPerObject.borderColor     = Color128(47, 112, 22, 255);
		barWidget.psPerObject.fillColor       = Color128(47, 112, 22, 255);
		barWidget.psPerObject.backgroundColor = Color128( 0,   0,  0, 255);
	}
	return true;
}

static void
UpdateBarWidgets(PluginContext& context, WidgetAPI::Update api)
{
	if (api.widgets.length == 0) return;

	api.PushVertexShader(context, StandardVertexShader::WVP);
	api.PushPixelShader(context, filledBarPS);

	for (u32 i = 0; i < api.widgets.length; i++)
	{
		Widget&    widget    = api.widgets[i];
		BarWidget& barWidget = (BarWidget&) api.widgetsUserData[i];

		// Update
		{
			// TODO: Lerping here means rendering has side effects. This interferes with rendering more
			// than once to highlight widgets.
			// Option 1 - Store lastValue
			// Option 2 - Split Update and Render for widgets
			// Option 3 - Don't render multiple times - push shader overrides before updating
			// Option 4 - Don't render multiple times - remember rendering calls and replay them
			Sensor& sensor = *api.GetSensor(context, widget.sensorHandle);
			barWidget.psPerObject.fillAmount = Lerp(barWidget.psPerObject.fillAmount, sensor.value, 0.10f);
		}

		// Draw
		{
			v2 position = WidgetPosition(widget);
			v2 size = widget.size;

			Matrix world = Identity();
			SetPosition(world, position, -widget.depth);
			SetScale   (world, size, 1.0f);
			barWidget.vsPerObject.wvp = world * api.GetViewProjectionMatrix(context);

			// TODO: Can we use instancing to accelerate this?
			api.UpdateVSConstantBuffer(context, StandardVertexShader::WVP, 0, &barWidget.vsPerObject);
			api.UpdatePSConstantBuffer(context, filledBarPS, 0, &barWidget.psPerObject);
			api.DrawMesh(context, StandardMesh::Quad);
		}
	}

	api.PopVertexShader(context);
	api.PopPixelShader(context);
}

static b8
Initialize(PluginContext& context, WidgetPluginAPI::Initialize api)
{
	WidgetDesc widgetDesc = {};
	widgetDesc.name         = String_FromView("Filled Bar");
	widgetDesc.userDataSize = sizeof(BarWidget);
	widgetDesc.Initialize   = InitializeBarWidgets;
	widgetDesc.Update       = UpdateBarWidgets;
	api.RegisterWidgets(context, widgetDesc);

	u32 cBufSizes[] = {
		{ sizeof(PSPerObject)  }
	};
	filledBarPS = api.LoadPixelShader(context, "Shaders/Filled Bar.ps.cso", cBufSizes);
	// TODO: Assert shader

	return true;
}

EXPORT void
GetWidgetPluginInfo(PluginDesc& desc, WidgetPluginFunctions& functions)
{
	desc.name       = "Filled Bar";
	desc.author     = "akbyrd";
	desc.version    = 1;
	desc.lhmVersion = LHMVersion;

	functions.Initialize = Initialize;
}

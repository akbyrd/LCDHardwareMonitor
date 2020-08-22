#include "LHMAPI.h"
#include "WVP.vs.h"
#include "Filled Bar.ps.h"

struct BarWidget
{
	r32          borderSize;
	r32          borderBlur;
	VSPerObject  vsPerObject;
	PSInitialize psInitialize;
	PSPerObject  psPerObject;
};

static PixelShader filledBarPS = {};

static b8
InitializeBarWidgets(PluginContext& context, WidgetAPI::Initialize api)
{
	UNUSED(context);

	for (u32 i = 0; i < api.widgets.length; i++)
	{
		Widget&    widget    = api.widgets[i];
		BarWidget& barWidget = (BarWidget&) api.widgetsUserData[i];

		widget.size = { 240, 12 };

		barWidget.borderSize = 1.0f;
		barWidget.borderBlur = 0.0f;

		v2 pixelsPerUV = 1.0f / widget.size;
		barWidget.psInitialize.borderSizeUV    = barWidget.borderSize * pixelsPerUV;
		barWidget.psInitialize.borderBlurUV    = barWidget.borderBlur * pixelsPerUV;
		barWidget.psInitialize.borderColor     = Color128(47, 112, 22, 255);
		barWidget.psInitialize.fillColor       = Color128(47, 112, 22, 255);
		barWidget.psInitialize.backgroundColor = Color128( 0,   0,  0, 255);
	}
	return true;
}

static void
UpdateBarWidgets(PluginContext& context, WidgetAPI::Update api)
{
	if (api.widgets.length == 0) return;

	// TODO:This shouldn't be stored on every widget!
	BarWidget& barWidgetShared = (BarWidget&) api.widgetsUserData[0];
	api.UpdatePSConstantBuffer(context, filledBarPS, 0, &barWidgetShared.psInitialize);

	api.PushVertexShader(context, StandardVertexShader::WVP);
	api.PushPixelShader(context, filledBarPS);

	for (u32 i = 0; i < api.widgets.length; i++)
	{
		Widget&    widget    = api.widgets[i];
		BarWidget& barWidget = (BarWidget&) api.widgetsUserData[i];

		// TODO: Not the biggest fan of this...
		if (!widget.ref) continue;

		// Update
		if (widget.sensorRef)
		{
			Sensor& sensor = api.sensors[widget.sensorRef];
			r32 value = sensor.value / 100.0f;
			barWidget.psPerObject.fillAmount = Lerp(barWidget.psPerObject.fillAmount, value, 0.10f);
		}
		else
		{
			// DEBUG: Remove this (add a default dummy sensor)
			r32 phase = (r32) i / (r32) (api.widgets.length + 1) * 0.5f * r32Pi;
			barWidget.psPerObject.fillAmount = sinf(api.t + phase) * sinf(api.t + phase);
		}

		// Draw
		{
			v2 position = WidgetPosition(widget);
			v2 size = widget.size;

			Matrix world = Identity();
			SetPosition(world, position, -widget.depth);
			SetScale   (world, size, 1.0f);
			barWidget.vsPerObject.wvp = world * api.GetViewProjectionMatrix(context);

			api.UpdateVSConstantBuffer(context, StandardVertexShader::WVP, 0, &barWidget.vsPerObject);
			api.UpdatePSConstantBuffer(context, filledBarPS, 1, &barWidget.psPerObject);
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
		{ sizeof(PSInitialize) },
		{ sizeof(PSPerObject)  }
	};
	filledBarPS = api.LoadPixelShader(context, "Filled Bar.ps.cso", cBufSizes);
	// TODO: Assert shader

	return true;
}

EXPORT void
GetWidgetPluginInfo(PluginInfo& info, WidgetPluginFunctions& functions)
{
	info.name       = String_FromView( "Filled Bar");
	info.author     = String_FromView( "akbyrd");
	info.version    = 1;
	info.lhmVersion = LHMVersion;

	functions.Initialize = Initialize;
}

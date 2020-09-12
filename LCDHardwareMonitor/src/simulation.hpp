#include "Solid Colored.ps.h"
#include "Outline.ps.h"

enum struct GUIInteraction
{
	Null,
	MouseLook,
	DragAndDrop,
	DragSelection,
};

struct OutlineAnimation
{
	b8                  isShowing;
	i64                 startTicks;
	i64                 duration;
	r32                 startValue;
	r32                 endValue;
	List<FullWidgetRef> widgets;
	Outline::PSPerPass  psPerPass;
};

struct SimulationState
{
	PluginLoaderState*     pluginLoader;
	RendererState*         renderer;
	FT232HState*           ft232h;
	ILI9341State*          ili9341;

	List<Plugin>           plugins;
	List<SensorPlugin>     sensorPlugins;
	List<WidgetPlugin>     widgetPlugins;

	v2u                    renderSize;
	CPUTexture             renderTargetCPUCopy;
	RenderTarget           renderTargetGUICopy;

	v3                     cameraPos;
	Matrix                 view;
	Matrix                 proj;
	Matrix                 vp; // TODO: Remove this when simulation talks to the renderer directly
	Matrix                 iview;
	i64                    startTime;
	r32                    currentTime;

	b8                     ft232hInitialized;
	u32                    ft232hRetryCount;

	b8                     previewWindow;
	ConnectionState        guiConnection;
	GUIInteraction         guiInteraction;
	v4i                    interactionRect;
	v2i                    interactionRelPosStart;
	v2i                    mousePos;
	v2i                    mousePosStart;
	v2                     cameraRot;
	v2                     cameraRotStart;

	List<FullWidgetRef>    selected;
	List<FullWidgetRef>    hovered;
	RenderTarget           tempRenderTargets[2];
	DepthBuffer            tempDepthBuffers[3];
	List<OutlineAnimation> hoverAnimations;
	PixelShader            outlineShader;
	PixelShader            outlineCompositeShader;
	PixelShader            depthToAlphaShader;
	Outline::PSPerPass     outlinePSPerPass[2];
	Outline::PSPerPass     outlinePSPerPassSelected;
	Outline::PSPerPass     outlinePSPerPassHovered;
};

struct PluginContext
{
	SimulationState* s;
	SensorPlugin*    sensorPlugin;
	WidgetPlugin*    widgetPlugin;
	b8               success;
};

// -------------------------------------------------------------------------------------------------
// C++ is Stupid.

static String GetNameFromPath(StringView);
static void RemoveSensorRefs(SimulationState&, SensorPluginRef, Slice<SensorRef>);
static void RemoveWidgetRefs(SimulationState&, WidgetPluginRef, WidgetData&);
static void SelectWidgets(SimulationState&, Slice<FullWidgetRef>);

// -------------------------------------------------------------------------------------------------
// Sensor API

static void
RegisterSensors(PluginContext& context, Slice<Sensor> sensors)
{
	if (!context.success) return;

	SensorPlugin& sensorPlugin = *context.sensorPlugin;
	for (u32 i = 0; i < sensors.length; i++)
	{
		Sensor& sensor = sensors[i];
		sensor.ref = { sensorPlugin.sensors.length + i };
	}

	// TODO: Re-use empty slots in the list (from removes)
	List_AppendRange(sensorPlugin.sensors, sensors);
}

static void
UnregisterSensors(PluginContext& context, Slice<SensorRef> sensorRefs)
{
	if (!context.success) return;
	context.success = false;

	SensorPlugin& sensorPlugin = *context.sensorPlugin;

	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];

		b8 valid = true;
		valid = valid && List_IsRefValid(sensorPlugin.sensors, sensorRef);
		valid = valid && sensorPlugin.sensors[sensorRef].ref == sensorRef;
		LOG_IF(!valid, return,
			Severity::Error, "Sensor plugin gave a bad SensorRef '%'",
			context.s->plugins[sensorPlugin.pluginRef].info.name);
	}

	RemoveSensorRefs(*context.s, sensorPlugin.ref, sensorRefs);

	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];
		sensorPlugin.sensors[sensorRef] = {};
	}

	context.success = true;
}

// -------------------------------------------------------------------------------------------------
// Widget API

static void
RegisterWidgets(PluginContext& context, Slice<WidgetDesc> widgetDescs)
{
	if (!context.success) return;

	// TODO: Handle invalid widgetDef

	WidgetPlugin& widgetPlugin = *context.widgetPlugin;

	for (u32 i = 0; i < widgetDescs.length; i++)
	{
		WidgetDesc& widgetDesc = widgetDescs[i];

		WidgetData& widgetData = List_Append(widgetPlugin.widgetDatas);
		widgetData.ref = List_GetLastRef(widgetPlugin.widgetDatas);
		widgetData.desc = widgetDesc;
		widgetData.desc.ref = { widgetData.ref.value };

		List_Reserve(widgetData.widgets, 8);
		List_Reserve(widgetData.widgetsUserData, 8 * widgetDesc.userDataSize);
	}
}

static PixelShader
LoadPixelShader(PluginContext& context, StringView relPath, Slice<u32> cBufSizes)
{
	if (!context.success) return PixelShader::Null;
	context.success = false;

	Plugin& plugin = context.s->plugins[context.widgetPlugin->pluginRef];
	RendererState& rendererState = *context.s->renderer;

	String path = String_Format("%/%", plugin.directory, relPath);
	defer { String_Free(path); };

	String psName = GetNameFromPath(path);
	defer { String_Free(psName); };

	PixelShader ps = Renderer_LoadPixelShader(rendererState, psName, path, cBufSizes);
	LOG_IF(!ps, return PixelShader::Null,
		Severity::Error, "Failed to load pixel shader '%'", path);

	context.success = true;
	return ps;
}

static Matrix
GetViewMatrix(PluginContext& context)
{
	return context.s->view;
}

static Matrix
GetProjectionMatrix(PluginContext& context)
{
	return context.s->proj;
}

static Matrix
GetViewProjectionMatrix(PluginContext& context)
{
	return context.s->vp;
}

static void
UpdateVSConstantBuffer(PluginContext& context, VertexShader vs, u32 index, void* data)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;
	WidgetPlugin&  widgetPlugin  = *context.widgetPlugin;

	VSConstantBufferUpdate cBufUpdate = {};
	cBufUpdate.vs    = vs;
	cBufUpdate.index = index;
	cBufUpdate.data  = data;

	b8 success = Renderer_ValidateVSConstantBufferUpdate(rendererState, cBufUpdate);
	LOG_IF(!success, return,
		Severity::Warning, "Updating invalid VS constant buffer from plugin '%'", widgetPlugin.name);

	Renderer_UpdateVSConstantBuffer(rendererState, cBufUpdate);

	context.success = true;
}

static void
UpdatePSConstantBuffer(PluginContext& context, PixelShader ps, u32 index, void* data)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;
	WidgetPlugin&  widgetPlugin  = *context.widgetPlugin;

	PSConstantBufferUpdate cBufUpdate = {};
	cBufUpdate.ps    = ps;
	cBufUpdate.index = index;
	cBufUpdate.data  = data;

	b8 success = Renderer_ValidatePSConstantBufferUpdate(rendererState, cBufUpdate);
	LOG_IF(!success, return,
		Severity::Warning, "Updating invalid PS constant buffer from plugin '%'", widgetPlugin.name);

	Renderer_UpdatePSConstantBuffer(rendererState, cBufUpdate);

	context.success = true;
}

static void
DrawMesh(PluginContext& context, Mesh mesh)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;
	WidgetPlugin&  widgetPlugin  = *context.widgetPlugin;

	b8 success = Renderer_ValidateMesh(rendererState, mesh);
	LOG_IF(!success, return,
		Severity::Warning, "Drawing invalid mesh from plugin '%'", widgetPlugin.name);

	Renderer_DrawMesh(rendererState, mesh);

	context.success = true;
}

// TODO: Validation
static void
PushVertexShader(PluginContext& context, VertexShader vs)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;
	WidgetPlugin&  widgetPlugin  = *context.widgetPlugin;

	b8 success = Renderer_ValidateVertexShader(rendererState, vs);
	LOG_IF(!success, return,
		Severity::Warning, "Pushing invalid VS from plugin '%'", widgetPlugin.name);

	Renderer_PushVertexShader(rendererState, vs);

	context.success = true;
}

static void
PopVertexShader(PluginContext& context)
{
	if (!context.success) return;

	RendererState& rendererState = *context.s->renderer;
	Renderer_PopVertexShader(rendererState);
}

static void
PushPixelShader(PluginContext& context, PixelShader ps)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;
	WidgetPlugin&  widgetPlugin  = *context.widgetPlugin;

	b8 success = Renderer_ValidatePixelShader(rendererState, ps);
	LOG_IF(!success, return,
		Severity::Warning, "Pushing invalid PS from plugin '%'", widgetPlugin.name);

	Renderer_PushPixelShader(rendererState, ps);

	context.success = true;
}

static void
PopPixelShader(PluginContext& context)
{
	if (!context.success) return;

	RendererState& rendererState = *context.s->renderer;
	Renderer_PopPixelShader(rendererState);
}

// -------------------------------------------------------------------------------------------------
// Messages To GUI

// NOTE: None of these functions return error codes because the messaging system handles errors
// internally. It will disconnect and stop attempting to send messages when a failure occurs.

static void
ToGUI_Connect(SimulationState& s)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::Connect connect = {};
	connect.version       = LHMVersion;
	connect.renderSurface = Renderer_GetSharedRenderTargetHandle(*s.renderer, s.renderTargetGUICopy);
	connect.renderSize.x  = s.renderSize.x;
	connect.renderSize.y  = s.renderSize.y;
	SerializeAndQueueMessage(s.guiConnection, connect);
}

// TODO: Can this implicitly be PluginStatesChanged also?
static void
ToGUI_PluginsAdded(SimulationState& s, Slice<Plugin> plugins)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::PluginsAdded pluginsAdded = {};
	pluginsAdded.refs  = Slice_MemberSlice(plugins, &Plugin::ref);
	pluginsAdded.kinds = Slice_MemberSlice(plugins, &Plugin::kind);
	pluginsAdded.infos = Slice_MemberSlice(plugins, &Plugin::info);
	SerializeAndQueueMessage(s.guiConnection, pluginsAdded);
}

static void
ToGUI_PluginStatesChanged(SimulationState& s, Slice<Plugin> plugins)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::PluginStatesChanged statesChanged = {};
	statesChanged.refs       = Slice_MemberSlice(plugins, &Plugin::ref);
	statesChanged.kinds      = Slice_MemberSlice(plugins, &Plugin::kind);
	statesChanged.loadStates = Slice_MemberSlice(plugins, &Plugin::loadState);
	SerializeAndQueueMessage(s.guiConnection, statesChanged);
}

static void
ToGUI_SensorsAdded(SimulationState& s, Slice<SensorPluginRef> sensorPluginRefs, Slice<List<Sensor>> sensors)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::SensorsAdded sensorsAdded = {};
	sensorsAdded.sensorPluginRefs = sensorPluginRefs;
	sensorsAdded.sensors          = sensors;
	SerializeAndQueueMessage(s.guiConnection, sensorsAdded);
}

static void
ToGUI_WidgetDescsAdded(SimulationState& s, WidgetPluginRef widgetPluginRef, Slice<WidgetDesc> widgetDescs)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::WidgetDescsAdded widgetDescsAdded = {};
	widgetDescsAdded.widgetPluginRefs = widgetPluginRef;
	widgetDescsAdded.descs            = widgetDescs;
	SerializeAndQueueMessage(s.guiConnection, widgetDescsAdded);
}

static void
ToGUI_WidgetsAdded(SimulationState& s, WidgetPlugin& widgetPlugin, WidgetData widgetData, Slice<WidgetRef> widgetRefs)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::WidgetsAdded widgetsAdded = {};
	widgetsAdded.pluginRef  = widgetPlugin.ref;
	widgetsAdded.dataRef    = widgetData.ref;
	widgetsAdded.widgetRefs = widgetRefs;
	SerializeAndQueueMessage(s.guiConnection, widgetsAdded);
}

static void
ToGUI_WidgetSelectionChanged(SimulationState& s, Slice<FullWidgetRef> refs)
{
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	ToGUI::WidgetSelectionChanged widgetSelection = {};
	widgetSelection.refs = refs;
	SerializeAndQueueMessage(s.guiConnection, widgetSelection);
}

// -------------------------------------------------------------------------------------------------
// GUI State Changes

static void
OnConnect(SimulationState& s)
{
	ToGUI_Connect(s);
	ToGUI_PluginsAdded(s, s.plugins);
	ToGUI_PluginStatesChanged(s, s.plugins);

	{
		Slice<SensorPluginRef> sensorPluginRefs = List_MemberSlice(s.sensorPlugins, &SensorPlugin::ref);
		Slice<List<Sensor>>    sensors          = List_MemberSlice(s.sensorPlugins, &SensorPlugin::sensors);
		ToGUI_SensorsAdded(s, sensorPluginRefs, sensors);
	}

	for (u32 i = 0; i < s.widgetPlugins.length; i++)
	{
		WidgetPlugin& widgetPlugin = s.widgetPlugins[i];

		// NOTE: It's surprisingly tricky to send a Slice<Slice<T>> when it's backed by
		// a set of List<T>. The inner slices are temporaries and need to be stored.
		Slice<WidgetDesc> descs = List_MemberSlice(widgetPlugin.widgetDatas, &WidgetData::desc);
		ToGUI_WidgetDescsAdded(s, widgetPlugin.ref, descs);

		for (u32 j = 0; j < widgetPlugin.widgetDatas.length; j++)
		{
			WidgetData& widgetData = widgetPlugin.widgetDatas[j];

			Slice<WidgetRef> refs = List_MemberSlice(widgetData.widgets, &Widget::ref);
			ToGUI_WidgetsAdded(s, widgetPlugin, widgetData, refs);
		}
	}
}

static void
OnDisconnect(SimulationState& s)
{
	List_Free(s.hovered);
	List_Free(s.selected);

	s.guiConnection.sendIndex = 0;
	s.guiConnection.recvIndex = 0;
}

static void
OnTeardown(ConnectionState& con)
{
	PipeResult result;

	ToGUI::Disconnect disconnect = {};
	disconnect.header.id    = IdOf<ToGUI::Disconnect>;
	disconnect.header.index = con.sendIndex - (con.queue.length - con.queueIndex);
	disconnect.header.size  = sizeof(ToGUI::Disconnect);

	Bytes bytes = {};
	bytes.length   = sizeof(ToGUI::Disconnect);
	bytes.capacity = bytes.length;
	bytes.data     = (u8*) &disconnect;

	result = Platform_WritePipe(con.pipe, bytes);
	// TODO: WritePipe should probably update the connection state to make this checking more correct
	LOG_IF(result == PipeResult::TransientFailure, return,
		Severity::Info, "Unable to send GUI disconnect signal");
	LOG_IF(result == PipeResult::UnexpectedFailure, return,
		Severity::Error, "Failed to send GUI disconnect signal");

	result = Platform_FlushPipe(con.pipe);
	LOG_IF(result == PipeResult::UnexpectedFailure, return,
		Severity::Error, "Failed to flush GUI communication pipe");
}

// -------------------------------------------------------------------------------------------------
// Plugin Loading

static Plugin&
RegisterPlugin(SimulationState& s, StringView directory, StringView fileName)
{
	// Re-use empty slots from unregistered plugins
	Plugin* plugin = nullptr;
	for (u32 i = 0; i < s.plugins.length; i++)
	{
		Plugin& slot = s.plugins[i];
		if (!slot.ref)
		{
			plugin = &slot;
			break;
		}
	}

	if (!plugin)
		plugin = &List_Append(s.plugins);

	defer { ToGUI_PluginStatesChanged(s, *plugin); };
	auto pluginGuard = guard { plugin->loadState = PluginLoadState::Broken; };

	plugin->ref       = List_GetLastRef(s.plugins);
	plugin->fileName  = String_FromView(fileName);
	plugin->directory = String_FromView(directory);

	pluginGuard.dismiss = true;
	return *plugin;
}

static void
UnregisterPlugin(SimulationState& s, Plugin& plugin)
{
	Assert(plugin.loadState != PluginLoadState::Loaded);
	defer { ToGUI_PluginStatesChanged(s, plugin); };

	String_Free(plugin.fileName);
	String_Free(plugin.directory);
	String_Free(plugin.info.name);
	String_Free(plugin.info.author);
	plugin = {};
}

static void
TeardownSensor(Sensor& sensor)
{
	String_Free(sensor.name);
	String_Free(sensor.identifier);
	String_Free(sensor.format);
}

static void
TeardownSensorPlugin(SensorPlugin& sensorPlugin)
{
	for (u32 i = 0; i < sensorPlugin.sensors.length; i++)
	{
		Sensor& sensor = sensorPlugin.sensors[i];
		TeardownSensor(sensor);
	}
	List_Free(sensorPlugin.sensors);
	sensorPlugin = {};
}

static b8
LoadSensorPlugin(SimulationState& s, Plugin& plugin)
{
	Assert(plugin.loadState != PluginLoadState::Loaded);

	SensorPlugin* sensorPlugin = nullptr;
	for (u32 i = 0; i < s.sensorPlugins.length; i++)
	{
		SensorPlugin& slot = s.sensorPlugins[i];
		if (!slot.ref)
		{
			sensorPlugin = &slot;
			break;
		}
	}

	if (!sensorPlugin)
		sensorPlugin = &List_Append(s.sensorPlugins);

	sensorPlugin->ref = List_GetLastRef(s.sensorPlugins);
	sensorPlugin->pluginRef = plugin.ref;

	plugin.rawRefToKind = sensorPlugin->ref.value;
	plugin.kind = PluginKind::Sensor;

	defer { ToGUI_PluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	b8 success = PluginLoader_LoadSensorPlugin(*s.pluginLoader, plugin, *sensorPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to load Sensor plugin '%'", plugin.fileName);

	sensorPlugin->name = plugin.info.name;
	// TODO: Leak on failure?
	List_Reserve(sensorPlugin->sensors, 32);

	// TODO: try/catch?
	if (sensorPlugin->functions.Initialize)
	{
		PluginContext context = {};
		context.s            = &s;
		context.sensorPlugin = sensorPlugin;
		context.success      = true;

		SensorPluginAPI::Initialize api = {};
		api.RegisterSensors = RegisterSensors;

		success = sensorPlugin->functions.Initialize(context, api);
		success &= context.success;
		if (!success)
		{
			LOG(Severity::Error, "Failed to initialize Sensor plugin '%'", plugin.info.name);
			// Remove all sensors so they don't get used
			TeardownSensorPlugin(*sensorPlugin);
			return false;
		}
	}

	pluginGuard.dismiss = true;
	return true;
}

static b8
UnloadSensorPlugin(SimulationState& s, SensorPlugin& sensorPlugin)
{
	Plugin& plugin = s.plugins[sensorPlugin.pluginRef];
	Assert(plugin.loadState != PluginLoadState::Unloaded);

	defer { ToGUI_PluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	// TODO: try/catch?
	if (sensorPlugin.functions.Teardown)
	{
		PluginContext context = {};
		context.s            = &s;
		context.sensorPlugin = &sensorPlugin;
		context.success      = true;

		SensorPluginAPI::Teardown api = {};
		api.sensors = sensorPlugin.sensors;

		sensorPlugin.functions.Teardown(context, api);
	}

	RemoveSensorRefs(s, sensorPlugin.ref, List_MemberSlice(sensorPlugin.sensors, &Sensor::ref));
	TeardownSensorPlugin(sensorPlugin);

	b8 success = PluginLoader_UnloadSensorPlugin(*s.pluginLoader, plugin, sensorPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Sensor plugin '%'", plugin.info.name);

	plugin.rawRefToKind = 0;

	pluginGuard.dismiss = true;
	return true;
}

static void
TeardownWidgetData(WidgetData& widgetData)
{
	List_Free(widgetData.widgets);
	List_Free(widgetData.widgetsUserData);
}

static void
TeardownWidgetPlugin(WidgetPlugin& widgetPlugin)
{
	for (u32 i = 0; i < widgetPlugin.widgetDatas.length; i++)
	{
		WidgetData& widgetData = widgetPlugin.widgetDatas[i];
		TeardownWidgetData(widgetData);
	}
	List_Free(widgetPlugin.widgetDatas);
	widgetPlugin = {};
}

static b8
LoadWidgetPlugin(SimulationState& s, Plugin& plugin)
{
	Assert(plugin.loadState != PluginLoadState::Loaded);

	WidgetPlugin* widgetPlugin = nullptr;
	for (u32 i = 0; i < s.widgetPlugins.length; i++)
	{
		WidgetPlugin& slot = s.widgetPlugins[i];
		if (!slot.ref)
		{
			widgetPlugin = &slot;
			break;
		}
	}

	if (!widgetPlugin)
		widgetPlugin = &List_Append(s.widgetPlugins);

	widgetPlugin->ref = List_GetLastRef(s.widgetPlugins);
	widgetPlugin->pluginRef = plugin.ref;

	plugin.rawRefToKind = widgetPlugin->ref.value;
	plugin.kind = PluginKind::Widget;

	defer { ToGUI_PluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	b8 success = PluginLoader_LoadWidgetPlugin(*s.pluginLoader, plugin, *widgetPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to load Widget plugin '%'", plugin.fileName);

	widgetPlugin->name = plugin.info.name;

	// TODO: try/catch?
	if (widgetPlugin->functions.Initialize)
	{
		PluginContext context = {};
		context.s            = &s;
		context.widgetPlugin = widgetPlugin;
		context.success      = true;

		WidgetPluginAPI::Initialize api = {};
		api.RegisterWidgets = RegisterWidgets;
		api.LoadPixelShader = LoadPixelShader;

		success = widgetPlugin->functions.Initialize(context, api);
		success &= context.success;
		if (!success)
		{
			LOG(Severity::Error, "Failed to initialize Widget plugin '%'", plugin.info.name);
			// Remove all widgets so they don't get used
			TeardownWidgetPlugin(*widgetPlugin);
			return false;
		}
	}

	pluginGuard.dismiss = true;
	return true;
}

static b8
UnloadWidgetPlugin(SimulationState& s, WidgetPlugin& widgetPlugin)
{
	Plugin& plugin = s.plugins[widgetPlugin.pluginRef];
	Assert(plugin.loadState != PluginLoadState::Unloaded);

	defer { ToGUI_PluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	PluginContext context = {};
	context.s            = &s;
	context.widgetPlugin = &widgetPlugin;

	WidgetAPI::Teardown widgetAPI = {};
	for (u32 i = 0; i < widgetPlugin.widgetDatas.length; i++)
	{
		WidgetData& widgetData = widgetPlugin.widgetDatas[i];
		if (widgetData.widgets.length == 0) continue;

		context.success = true;

		widgetAPI.widgets                = widgetData.widgets;
		widgetAPI.widgetsUserData        = widgetData.widgetsUserData;
		widgetAPI.widgetsUserData.stride = widgetData.desc.userDataSize;

		// TODO: try/catch?
		if (widgetData.desc.Teardown)
			widgetData.desc.Teardown(context, widgetAPI);
	}

	// TODO: try/catch?
	if (widgetPlugin.functions.Teardown)
	{
		WidgetPluginAPI::Teardown pluginAPI = {};
		widgetPlugin.functions.Teardown(context, pluginAPI);
	}

	for (u32 i = 0; i < widgetPlugin.widgetDatas.length; i++)
	{
		WidgetData& widgetData = widgetPlugin.widgetDatas[i];
		RemoveWidgetRefs(s, widgetPlugin.ref, widgetData);
	}
	TeardownWidgetPlugin(widgetPlugin);

	b8 success = PluginLoader_UnloadWidgetPlugin(*s.pluginLoader, plugin, widgetPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Widget plugin '%'", plugin.info.name);

	plugin.rawRefToKind = 0;

	pluginGuard.dismiss = true;
	return true;
}

// -------------------------------------------------------------------------------------------------

static String
GetNameFromPath(StringView path)
{
	if (path.length == 0) return {};

	StrPos first = String_FindLast(path, '/');
	if (first == StrPos::Null)
		first = String_GetFirstPos(path);
	Assert(first != String_GetLastPos(path));
	first = first + 1;

	StrPos last = String_FindLast(path, '.');
	if (last == StrPos::Null)
		last = String_GetLastPos(path);
	Assert(last != String_GetFirstPos(path));
	last = last - 1;

	if (first == last) return {};

	StringSlice nameSlice = StringSlice_Create(path, first, last);
	return String_FromSlice(nameSlice);
}

static void
AddWidgets(SimulationState& s, WidgetPlugin& widgetPlugin, WidgetData& widgetData, Slice<v2> positions)
{
	u32 prevWidgetLen = widgetData.widgets.length;

	auto createGuard = guard {
		widgetData.widgets.length = prevWidgetLen;
		widgetData.widgetsUserData.length = widgetData.desc.userDataSize * prevWidgetLen;
	};

	List_AppendRange(widgetData.widgets, positions.length);
	List_AppendRange(widgetData.widgetsUserData, widgetData.desc.userDataSize * positions.length);

	PluginContext context = {};
	context.s            = &s;
	context.widgetPlugin = &widgetPlugin;
	context.success      = true;

	WidgetAPI::Initialize api = {};
	api.widgets                = List_Slice(widgetData.widgets, prevWidgetLen);
	api.widgetsUserData        = List_Slice(widgetData.widgetsUserData, widgetData.desc.userDataSize * prevWidgetLen);
	api.widgetsUserData.stride = widgetData.desc.userDataSize;

	widgetData.desc.Initialize(context, api);
	if (!context.success) return;

	for (u32 i = 0; i < positions.length; i++)
	{
		v2 position = positions[i];
		Widget& widget = widgetData.widgets[prevWidgetLen + i];
		widget.ref      = List_GetRef(widgetData.widgets, prevWidgetLen + i);
		widget.position = position;
	}

	Slice<Widget> newWidgets = List_Slice(widgetData.widgets, prevWidgetLen);
	ToGUI_WidgetsAdded(s, widgetPlugin, widgetData, Slice_MemberSlice(newWidgets, &Widget::ref));

	createGuard.dismiss = true;
}

static void
AddWidgets(SimulationState& s, FullWidgetDataRef ref, Slice<v2> positions)
{
	WidgetPlugin& widgetPlugin = s.widgetPlugins[ref.pluginRef];
	WidgetData& widgetData = widgetPlugin.widgetDatas[ref.dataRef];
	AddWidgets(s, widgetPlugin, widgetData, positions);
}

static void
RemoveWidgetRefs(SimulationState& s, Slice<FullWidgetRef> refs)
{
	for (u32 i = 0; i < refs.length; i++)
	{
		FullWidgetRef ref = refs[i];

		// NOTE: Do slow removes to maintain selection order
		for (u32 j = 0; j < s.hovered.length; j++)
		{
			if (s.hovered[j] == ref)
			{
				List_Remove(s.hovered, j);
				break;
			}
		}

		for (u32 j = 0; j < s.selected.length; j++)
		{
			if (s.selected[j] == ref)
			{
				List_Remove(s.selected, j);
				break;
			}
		}
	}
}

static void
RemoveWidgetRefs(SimulationState& s, WidgetPluginRef pluginRef, WidgetData& widgetData)
{
	FullWidgetRef ref = {};
	ref.pluginRef = pluginRef;
	ref.dataRef = widgetData.ref;

	// TODO: Should widgets have their own refs?
	for (u32 i = 0; i < widgetData.widgets.length; i++)
	{
		ref.widgetRef = ToRef<Widget>(i);
		RemoveWidgetRefs(s, ref);
	}
}

static void
RemoveWidgets(SimulationState& s, Slice<FullWidgetRef> refs)
{
	RemoveWidgetRefs(s, refs);

	for (u32 i = 0; i < refs.length; i++)
	{
		FullWidgetRef ref = refs[i];

		WidgetPlugin& plugin = s.widgetPlugins[ref.pluginRef];
		WidgetData& data = plugin.widgetDatas[ref.dataRef];

		// NOTE: Can't 'remove fast' because it invalidates refs
		data.widgets[ref.widgetRef] = {};
		List_ZeroRange(data.widgetsUserData, data.desc.userDataSize * ToIndex(ref.widgetRef), data.desc.userDataSize);
	}
}

static void
RemoveSensorRefs(SimulationState& s, SensorPluginRef sensorPluginRef, Slice<SensorRef> sensorRefs)
{
	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];
		// TODO: Widget iterator!
		for (u32 j = 0; j < s.widgetPlugins.length; j++)
		{
			WidgetPlugin& widgetPlugin = s.widgetPlugins[j];
			for (u32 k = 0; k < widgetPlugin.widgetDatas.length; k++)
			{
				WidgetData& widgetData = widgetPlugin.widgetDatas[k];
				for (u32 l = 0; l < widgetData.widgets.length; l++)
				{
					Widget& widget = widgetData.widgets[l];
					// TODO: Add FullSensorRef
					if (widget.sensorPluginRef == sensorPluginRef && widget.sensorRef == sensorRef)
					{
						widget.sensorPluginRef = SensorPluginRef::Null;
						widget.sensorRef       = SensorRef::Null;
					}
				}
			}
		}
	}
}

static void
RemoveSelectedWidgets(SimulationState& s)
{
	RemoveWidgets(s, s.selected);
}

static Widget&
GetWidget(SimulationState& s, FullWidgetRef ref)
{
	WidgetPlugin& plugin = s.widgetPlugins[ref.pluginRef];
	WidgetData& data = plugin.widgetDatas[ref.dataRef];
	Widget& widget = data.widgets[ref.widgetRef];
	return widget;
}

static void
SelectWidgets(SimulationState& s, Slice<FullWidgetRef> widgets)
{
	// TODO: Validate
	List_Clear(s.selected);
	List_AppendRange(s.selected, widgets);

	ToGUI_WidgetSelectionChanged(s, s.selected);
}

static b8
IsWidgetSelected(SimulationState& s, FullWidgetRef widget)
{
	for (u32 i = 0; i < s.selected.length; i++)
		if (s.selected[i] == widget)
			return true;
	return false;
}

static void
MouseLook(SimulationState& s)
{
	v2i deltaPos = s.mousePos - s.mousePosStart;
	s.cameraRot = 0.0005f * (v2) deltaPos * (2*r32Pi);

	v2 rot = s.cameraRotStart + s.cameraRot;
	rot.pitch = Clamp(rot.pitch, -0.49f*r32Pi, 0.49f*r32Pi);

	// TODO: Get render size out of here
	v2 offset = (v2) s.renderSize / 2.0f;
	v3 target = { offset.x, offset.y, 0 };
	v3 pos    = GetOrbitPos(target, rot, 500);

	s.cameraPos = pos;
	s.view      = LookAt(pos, target);
	s.vp        = s.view * s.proj;

	s.iview = InvertRT(s.view);
}

static void
ResetCamera(SimulationState& s)
{
	v2 offset = (v2) s.renderSize / 2.0f;
	v3 pos    = { offset.x, offset.y, 500 };
	v3 target = { offset.x, offset.y, 0 };

	s.cameraPos = pos;
	s.cameraRot = {};

	s.view = LookAt(pos, target);
	s.proj = Orthographic((v2) s.renderSize, 0, 10000);
	s.vp   = s.view * s.proj;

	s.iview = InvertRT(s.view);
}

// TODO: Think through how to handle left mouse going down and the exclusivity of selection and
// drag. Once to select, once to begin drag?
// TODO: We assume dragging happens in 'screen space'. Is that a good assumption?
static void
DragSelection(SimulationState& s)
{
	if (s.selected.length == 0) return;

	v4i rect = {};
	rect.pos  = s.mousePos + s.interactionRelPosStart;
	rect.size = s.interactionRect.size;

	v4i bounds = {};
	bounds.pos  = v2i{ 0, 0 };
	bounds.size = (v2i) s.renderSize;

	rect = ClampRect(rect, bounds);
	if (rect.pos != s.interactionRect.pos)
	{
		v2i deltaPos = rect.pos - s.interactionRect.pos;
		s.interactionRect.pos = rect.pos;

		for (u32 i = 0; i < s.selected.length; i++)
		{
			FullWidgetRef selected = s.selected[i];

			Widget& widget = GetWidget(s, selected);
			widget.position += (v2) deltaPos;
		}
	}
}

// TODO: This rendering detail is leaking awfully high up...
static void
HighlightWidgets(SimulationState& s, Slice<FullWidgetRef> refs, Outline::PSPerPass& compositeCBuf)
{
	Assert(refs.length != 0);

	// Re-render widgets
	// -> Depth 2
	{
		PluginContext context = {};
		context.s = &s;

		// TODO: Duplicating this is error prone
		WidgetAPI::Update widgetAPI = {};
		widgetAPI.t                       = s.currentTime;
		widgetAPI.sensors                 = s.sensorPlugins[0].sensors;
		widgetAPI.GetViewMatrix           = GetViewMatrix;
		widgetAPI.GetProjectionMatrix     = GetProjectionMatrix;
		widgetAPI.GetViewProjectionMatrix = GetViewProjectionMatrix;
		widgetAPI.UpdateVSConstantBuffer  = UpdateVSConstantBuffer;
		widgetAPI.UpdatePSConstantBuffer  = UpdatePSConstantBuffer;
		widgetAPI.DrawMesh                = DrawMesh;
		widgetAPI.PushVertexShader        = PushVertexShader;
		widgetAPI.PushPixelShader         = PushPixelShader;
		widgetAPI.PopVertexShader         = PopVertexShader;
		widgetAPI.PopPixelShader          = PopPixelShader;

		Renderer_PushEvent(*s.renderer, "Render Widgets Depth");
		Renderer_PushRenderTarget(*s.renderer, StandardRenderTarget::Null);
		Renderer_PushDepthBuffer(*s.renderer, s.tempDepthBuffers[2]);
		Renderer_ClearDepthBuffer(*s.renderer);
		for (u32 i = 0; i < refs.length ; i++)
		{
			FullWidgetRef widgetRef = refs[i];

			WidgetPlugin& widgetPlugin = s.widgetPlugins[widgetRef.pluginRef];
			WidgetData&   widgetData   = widgetPlugin.widgetDatas[widgetRef.dataRef];
			Widget&       widget       = widgetData.widgets[widgetRef.widgetRef];

			context.widgetPlugin = &widgetPlugin;
			context.success      = true;

			widgetAPI.widgets                = widget;
			// TODO: Can this be made simpler?
			widgetAPI.widgetsUserData        = widgetData.widgetsUserData[widgetData.desc.userDataSize * ToIndex(widget.ref)];
			widgetAPI.widgetsUserData.stride = widgetData.desc.userDataSize;

			// TODO: try/catch?
			widgetData.desc.Update(context, widgetAPI);
		}
		Renderer_PopDepthBuffer(*s.renderer);
		Renderer_PopRenderTarget(*s.renderer);
		Renderer_PopEvent(*s.renderer);
	}

	Renderer_PushDepthBuffer(*s.renderer, StandardDepthBuffer::Null);
	Renderer_PushVertexShader(*s.renderer, StandardVertexShader::ClipSpace);

	// Copy depth as solid color
	// Depth 2 -> Temp/Depth 0
	{
		Renderer_PushEvent(*s.renderer, "Copy Widgets Depth to Alpha");
		Renderer_SetBlendMode(*s.renderer, false);
		Renderer_PushRenderTarget(*s.renderer, s.tempRenderTargets[0]);
		Renderer_ClearRenderTarget(*s.renderer, Colors128::Clear);
		Renderer_PushDepthBuffer(*s.renderer, s.tempDepthBuffers[0]);
		Renderer_ClearDepthBuffer(*s.renderer);
		Renderer_PushPSResource(*s.renderer, s.tempDepthBuffers[2], 0);
		Renderer_PushPixelShader(*s.renderer, s.depthToAlphaShader);
		Renderer_DrawMesh(*s.renderer, StandardMesh::Fullscreen);
		Renderer_PopPixelShader(*s.renderer);
		Renderer_PopPSResource(*s.renderer, 0);
		Renderer_PopDepthBuffer(*s.renderer);
		Renderer_PopRenderTarget(*s.renderer);
		Renderer_PopEvent(*s.renderer);
	}

	// Expand to create outline
	// Temp/Depth 0 -> Temp/Depth 1 -> Temp/Depth 0
	{
		Renderer_PushEvent(*s.renderer, "Expand Widget Alpha");
		Renderer_SetBlendMode(*s.renderer, false);
		Renderer_PushPixelShader(*s.renderer, s.outlineShader);

		// Horizontal blur
		PSConstantBufferUpdate hCBufUpdate = {};
		hCBufUpdate.ps    = s.outlineShader;
		hCBufUpdate.index = 0;
		hCBufUpdate.data  = &s.outlinePSPerPass[0];

		Renderer_PushRenderTarget(*s.renderer, s.tempRenderTargets[1]);
		Renderer_ClearRenderTarget(*s.renderer, Colors128::Clear);
		Renderer_PushDepthBuffer(*s.renderer, s.tempDepthBuffers[1]);
		Renderer_ClearDepthBuffer(*s.renderer);
		Renderer_PushPSResource(*s.renderer, s.tempRenderTargets[0], 0);
		Renderer_PushPSResource(*s.renderer, s.tempDepthBuffers[0], 1);
		Renderer_UpdatePSConstantBuffer(*s.renderer, hCBufUpdate);
		Renderer_DrawMesh(*s.renderer, StandardMesh::Fullscreen);
		Renderer_PopPSResource(*s.renderer, 1);
		Renderer_PopPSResource(*s.renderer, 0);
		Renderer_PopDepthBuffer(*s.renderer);
		Renderer_PopRenderTarget(*s.renderer);

		// Vertical blur
		PSConstantBufferUpdate vCBufUpdate = {};
		vCBufUpdate.ps    = s.outlineShader;
		vCBufUpdate.index = 0;
		vCBufUpdate.data  = &s.outlinePSPerPass[1];

		Renderer_PushRenderTarget(*s.renderer, s.tempRenderTargets[0]);
		Renderer_PushDepthBuffer(*s.renderer, s.tempDepthBuffers[0]);
		Renderer_PushPSResource(*s.renderer, s.tempRenderTargets[1], 0);
		Renderer_PushPSResource(*s.renderer, s.tempDepthBuffers[1], 1);
		Renderer_UpdatePSConstantBuffer(*s.renderer, vCBufUpdate);
		Renderer_DrawMesh(*s.renderer, StandardMesh::Fullscreen);
		Renderer_PopPSResource(*s.renderer, 1);
		Renderer_PopPSResource(*s.renderer, 0);
		Renderer_PopDepthBuffer(*s.renderer);
		Renderer_PopRenderTarget(*s.renderer);

		Renderer_PopPixelShader(*s.renderer);
		Renderer_PopEvent(*s.renderer);
	}

	// Composite outline back into main render target
	// Temp 0 -> Main
	{
		PSConstantBufferUpdate cBufUpdate = {};
		cBufUpdate.ps    = s.outlineCompositeShader;
		cBufUpdate.index = 0;
		cBufUpdate.data  = &compositeCBuf;

		Renderer_PushEvent(*s.renderer, "Composite Outline");
		Renderer_SetBlendMode(*s.renderer, true);
		Renderer_PushRenderTarget(*s.renderer, StandardRenderTarget::Main);
		Renderer_PushPSResource(*s.renderer, s.tempRenderTargets[0], 0);
		Renderer_PushPSResource(*s.renderer, s.tempDepthBuffers[0], 1);
		Renderer_PushPSResource(*s.renderer, s.tempDepthBuffers[2], 2);
		Renderer_PushPSResource(*s.renderer, StandardDepthBuffer::Main, 3);
		Renderer_PushPixelShader(*s.renderer, s.outlineCompositeShader);
		Renderer_UpdatePSConstantBuffer(*s.renderer, cBufUpdate);
		Renderer_DrawMesh(*s.renderer, StandardMesh::Fullscreen);
		Renderer_PopPixelShader(*s.renderer);
		Renderer_PopPSResource(*s.renderer, 3);
		Renderer_PopPSResource(*s.renderer, 2);
		Renderer_PopPSResource(*s.renderer, 1);
		Renderer_PopPSResource(*s.renderer, 0);
		Renderer_PopRenderTarget(*s.renderer);
		Renderer_PopEvent(*s.renderer);
	}

	Renderer_PopVertexShader(*s.renderer);
	Renderer_PopDepthBuffer(*s.renderer);
}

// -------------------------------------------------------------------------------------------------
// Messages From GUI

// NOTE: The GUI is considered internal code. Therefore, validation and error checking in these
// functions is limited to things that could have become invalidated due to the the asynchronous
// nature of GUI messaging. Validation for programming mistakes is limited to assertions.

static void
FromGUI_TerminateSimulation(SimulationState& s, FromGUI::TerminateSimulation& terminateSim)
{
	UNUSED_ARGS(s, terminateSim);
	Platform_RequestQuit();
}

static void
FromGUI_MouseMove(SimulationState& s, FromGUI::MouseMove& mouseMove)
{
	s.mousePos = mouseMove.pos;
	switch (s.guiInteraction)
	{
		// Nothing to do
		case GUIInteraction::Null: break;
		case GUIInteraction::DragAndDrop: break;

		case GUIInteraction::MouseLook:     MouseLook(s);     break;
		case GUIInteraction::DragSelection: DragSelection(s); break;
	}
}

static void
FromGUI_SelectHovered(SimulationState& s, FromGUI::SelectHovered&)
{
	SelectWidgets(s, s.hovered);
}

static void
FromGUI_BeginMouseLook(SimulationState& s, FromGUI::BeginMouseLook&)
{
	Assert(s.guiInteraction == GUIInteraction::Null);
	s.guiInteraction = GUIInteraction::MouseLook;

	s.mousePosStart  = s.mousePos;
	s.cameraRotStart = s.cameraRot;
}

static void
FromGUI_EndMouseLook(SimulationState& s, FromGUI::EndMouseLook&)
{
	Assert(s.guiInteraction == GUIInteraction::MouseLook);
	s.guiInteraction  = GUIInteraction::Null;

	s.mousePosStart   = {};
	s.cameraRot      += s.cameraRotStart;
	s.cameraRotStart  = {};
}

static void
FromGUI_ResetCamera(SimulationState& s, FromGUI::ResetCamera&)
{
	ResetCamera(s);
}

// TODO: Why doesn't this handle errors?
static void
FromGUI_SetPluginLoadStates(SimulationState& s, FromGUI::SetPluginLoadStates& setStates)
{
	// TODO: If a plugin is unloaded and a new one is load it will take up the same slot and the ref
	// here will be invalidated (tombstone?)
	Assert(setStates.refs.length == setStates.loadStates.length);
	for (u32 i = 0; i < setStates.refs.length; i++)
	{
		PluginRef pluginRef = setStates.refs[i];
		PluginLoadState loadState = setStates.loadStates[i];

		if (!List_IsRefValid(s.plugins, pluginRef))
		{
			LOG(Severity::Warning, "Received SetPluginLoadState request for a plugin that no longer exists");
			continue;
		}

		Plugin& plugin = s.plugins[pluginRef];
		switch (loadState)
		{
			case PluginLoadState::Null:
			case PluginLoadState::Loading:
			case PluginLoadState::Unloading:
			case PluginLoadState::Broken:
				Assert(false);
				break;

			case PluginLoadState::Loaded:
				if (plugin.kind == PluginKind::Sensor)
					LoadSensorPlugin(s, plugin);
				else
					LoadWidgetPlugin(s, plugin);
				break;

			case PluginLoadState::Unloaded:
				if (plugin.kind == PluginKind::Sensor)
				{
					SensorPluginRef ref = { plugin.rawRefToKind };
					SensorPlugin& sensorPlugin = s.sensorPlugins[ref];
					UnloadSensorPlugin(s, sensorPlugin);
				}
				else
				{
					WidgetPluginRef ref = { plugin.rawRefToKind };
					WidgetPlugin& widgetPlugin = s.widgetPlugins[ref];
					UnloadWidgetPlugin(s, widgetPlugin);
				}
				break;
		}
	}
}

static void
FromGUI_DragDrop(SimulationState& s, FromGUI::DragDrop& dragDrop)
{
	if (dragDrop.pluginKind == PluginKind::Widget)
	{
		if (dragDrop.inProgress)
		{
			Assert(s.guiInteraction == GUIInteraction::Null);
			s.guiInteraction = GUIInteraction::DragAndDrop;
		}
		else
		{
			Assert(s.guiInteraction == GUIInteraction::DragAndDrop);
			s.guiInteraction = GUIInteraction::Null;
		}
	}
}

static void
FromGUI_AddWidget(SimulationState& s, FromGUI::AddWidget& addWidget)
{
	if (!List_IsRefValid(s.widgetPlugins, addWidget.ref.pluginRef))
	{
		LOG(Severity::Warning, "Received AddWidget request for a plugin that no longer exists");
		return;
	}

	WidgetPlugin& widgetPlugin = s.widgetPlugins[addWidget.ref.pluginRef];
	if (!List_IsRefValid(widgetPlugin.widgetDatas, addWidget.ref.dataRef))
	{
		LOG(Severity::Warning, "Received AddWidget request for a widget type that no longer exists");
		return;
	}

	WidgetData& widgetData = widgetPlugin.widgetDatas[addWidget.ref.dataRef];
	AddWidgets(s, widgetPlugin, widgetData, addWidget.position);
}

static void
FromGUI_RemoveWidget(SimulationState& s, FromGUI::RemoveWidget& removeWidget)
{
	if (!List_IsRefValid(s.widgetPlugins, removeWidget.ref.pluginRef))
	{
		LOG(Severity::Warning, "Received RemoveWidget request for a plugin that no longer exists");
		return;
	}

	WidgetPlugin& widgetPlugin = s.widgetPlugins[removeWidget.ref.pluginRef];
	if (!List_IsRefValid(widgetPlugin.widgetDatas, removeWidget.ref.dataRef))
	{
		LOG(Severity::Warning, "Received RemoveWidget request for a widget type that no longer exists");
		return;
	}

	WidgetData& widgetData = widgetPlugin.widgetDatas[removeWidget.ref.dataRef];
	if (!List_IsRefValid(widgetData.widgets, removeWidget.ref.widgetRef))
	{
		LOG(Severity::Warning, "Received RemoveWidget request for a widget that no longer exists");
		return;
	}

	RemoveWidgets(s, removeWidget.ref);
}

static void
FromGUI_RemoveSelectedWidgets(SimulationState& s, FromGUI::RemoveSelectedWidgets&)
{
	RemoveSelectedWidgets(s);
}

static void
FromGUI_BeginDragSelection(SimulationState& s, FromGUI::BeginDragSelection&)
{
	Assert(s.guiInteraction == GUIInteraction::Null);
	s.guiInteraction = GUIInteraction::DragSelection;

	s.mousePosStart = s.mousePos;

	if (s.selected.length != 0)
	{
		s.interactionRect = { i32Max, i32Max, i32Min, i32Min };

		for (u32 i = 0; i < s.selected.length; i++)
		{
			FullWidgetRef selected = s.selected[i];

			Widget& widget = GetWidget(s, selected);
			v4i widgetRect = (v4i) WidgetRect(widget);
			s.interactionRect = RectCombine(s.interactionRect, widgetRect);
		}

		s.interactionRelPosStart = s.interactionRect.pos - s.mousePosStart;
	}
	else
	{
		s.interactionRect = {};
		s.interactionRelPosStart = {};
	}
}

static void
FromGUI_EndDragSelection(SimulationState& s, FromGUI::EndDragSelection&)
{
	Assert(s.guiInteraction == GUIInteraction::DragSelection);
	s.guiInteraction = GUIInteraction::Null;

	s.mousePosStart  = {};
}

static void
FromGUI_SetWidgetSelection(SimulationState& s, FromGUI::SetWidgetSelection& widgetSelection)
{
	SelectWidgets(s, widgetSelection.refs);
}

// -------------------------------------------------------------------------------------------------

b8
Simulation_Initialize(
	SimulationState&   s,
	PluginLoaderState& pluginLoader,
	RendererState&     renderer,
	FT232HState&       ft232h,
	ILI9341State&      ili9341)
{
	s.pluginLoader = &pluginLoader;
	s.renderer     = &renderer;
	s.ft232h       = &ft232h;
	s.ili9341      = &ili9341;
	s.startTime    = Platform_GetTicks();
	s.renderSize   = { 320, 240 };

	s.outlinePSPerPass[0].textureSize   = s.renderSize;
	s.outlinePSPerPass[0].blurDirection = v2{ 1.0f, 0.0f };
	s.outlinePSPerPass[1].textureSize   = s.renderSize;
	s.outlinePSPerPass[1].blurDirection = v2{ 0.0f, 1.0f };

	s.outlinePSPerPassSelected.outlineColor = Color128(0, 122, 204, 255);
	s.outlinePSPerPassSelected.darkenColor  = Color128(0, 0, 0, 128);
	s.outlinePSPerPassHovered.outlineColor  = Color128(28, 151, 234, 255);
	s.outlinePSPerPassHovered.darkenColor   = Color128(0, 0, 0, 0);

	b8 success = PluginLoader_Initialize(*s.pluginLoader);
	if (!success) return false;

	List_Reserve(s.sensorPlugins, 8);
	List_Reserve(s.widgetPlugins, 8);

	// Setup Camera
	ResetCamera(s);

	// Create Standard Rendering Resources
	{
		Renderer_SetRenderSize(*s.renderer, s.renderSize);

		RenderTarget rt = Renderer_CreateRenderTarget(*s.renderer, "Main", true);
		if (!rt) return false;
		Assert(rt == StandardRenderTarget::Main);

		DepthBuffer db = Renderer_CreateDepthBuffer(*s.renderer, "Main", true);
		if (!db) return false;
		Assert(db == StandardDepthBuffer::Main);

		s.renderTargetCPUCopy = Renderer_CreateCPUTexture(*s.renderer, "CPU Copy");
		if (!s.renderTargetCPUCopy) return false;

		s.renderTargetGUICopy = Renderer_CreateSharedRenderTarget(*s.renderer, "GUI Copy", false);
		if (!s.renderTargetGUICopy) return false;
	}

	// Load Default Assets
	{
		// Vertex shader
		{
			VertexShader vs;

			VertexAttribute vsAttributes[] = {
				{ VertexAttributeSemantic::Position, VertexAttributeFormat::v3 },
				{ VertexAttributeSemantic::Color,    VertexAttributeFormat::v4 },
				{ VertexAttributeSemantic::TexCoord, VertexAttributeFormat::v2 },
			};

			vs = Renderer_LoadVertexShader(*s.renderer, "WVP", "Shaders/WVP.vs.cso", vsAttributes, sizeof(Matrix));
			LOG_IF(!vs, return false,
				Severity::Error, "Failed to load built-in wvp vertex shader");
			Assert(vs == StandardVertexShader::WVP);

			vs = Renderer_LoadVertexShader(*s.renderer, "Clip Space", "Shaders/Clip Space.vs.cso", vsAttributes, sizeof(Matrix));
			LOG_IF(!vs, return false,
				Severity::Error, "Failed to load built-in clip space vertex shader");
			Assert(vs == StandardVertexShader::ClipSpace);
		}

		// Pixel shader
		{
			PixelShader ps;

			u32 cBufSizes[] = {
				{ sizeof(SolidColor::PSInitialize) },
			};
			ps = Renderer_LoadPixelShader(*s.renderer, "Solid Colored", "Shaders/Solid Colored.ps.cso", cBufSizes);
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in solid colored pixel shader");
			Assert(ps == StandardPixelShader::SolidColored);

			ps = Renderer_LoadPixelShader(*s.renderer, "Vertex Colored", "Shaders/Vertex Colored.ps.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in vertex colored pixel shader");
			Assert(ps == StandardPixelShader::VertexColored);

			ps = Renderer_LoadPixelShader(*s.renderer, "Debug Coordinates", "Shaders/Debug Coordinates.ps.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in vertex colored pixel shader");
			Assert(ps == StandardPixelShader::DebugCoordinates);

			ps = Renderer_LoadPixelShader(*s.renderer, "Composite", "Shaders/Composite.ps.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load composite pixel shader");
			Assert(ps == StandardPixelShader::Composite);
		}

		// Triangle mesh
		{
			Vertex vertices[] = {
				{ { -0.5f, -0.433f, 0 }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.0f,  0.433f, 0 }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.5f, 0.0f } },
				{ {  0.5f, -0.433f, 0 }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			};

			Index indices[] = {
				0, 1, 2,
			};

			Mesh triangle = Renderer_CreateMesh(*s.renderer, "Triangle Mesh", vertices, indices);
			Assert(triangle == StandardMesh::Triangle);
		}

		// Quad mesh
		{
			Vertex vertices[] = {
				{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
			};

			Index indices[] = {
				0, 1, 2,
				2, 3, 0,
			};

			Mesh quad = Renderer_CreateMesh(*s.renderer, "Quad Mesh", vertices, indices);
			Assert(quad == StandardMesh::Quad);
		}

		// Cube Mesh
		{
			// NOTE: Directions are from the perspective of the mesh itself, not
			// an onlooker.

			Vertex vertices[] = {
				// Back
				{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
				{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },

				// Right
				{ {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
				{ {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },

				// Front
				{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 1.0f, 0.0f } },
				{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 1.0f, 1.0f } },

				// Left
				{ { -0.5f, -0.5f, -0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f,  0.5f, -0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f,  0.5f,  0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
				{ { -0.5f, -0.5f,  0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },

				// Top
				{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
				{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },

				// Bottom
				{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
				{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
			};

			Index indices[] = {
				// Back
				 0,  1,  2,
				 2,  3,  0,

				// Right
				 4,  5,  6,
				 6,  7,  4,

				// Front
				 8,  9, 10,
				10, 11,  8,

				// Left
				12, 13, 14,
				14, 15, 12,

				// Top
				16, 17, 18,
				18, 19, 16,

				// Bottom
				20, 21, 22,
				22, 23, 20,
			};

			Mesh quad = Renderer_CreateMesh(*s.renderer, "Cube Mesh", vertices, indices);
			Assert(quad == StandardMesh::Cube);
		}

		// Fullscreen mesh
		{
			Vertex vertices[] = {
				{ { -1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f,  1.0f } }, // BL
				{ { -1.0f,  3.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, -1.0f } }, // TL
				{ {  3.0f, -1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 2.0f,  1.0f } }, // BR
			};

			Index indices[] = {
				0, 1, 2,
			};

			Mesh fullscreen = Renderer_CreateMesh(*s.renderer, "Fullscreen Mesh", vertices, indices);
			Assert(fullscreen == StandardMesh::Fullscreen);
		}
	}

	// Other rendering resources
	{
		for (u32 i = 0; i < ArrayLength(s.tempRenderTargets); i++)
		{
			String name = String_Format("Temporary Render Target %", i);
			defer { String_Free(name); };

			s.tempRenderTargets[i] = Renderer_CreateRenderTargetWithAlpha(*s.renderer, name, true);
			if (!s.tempRenderTargets[i]) return false;
		}

		for (u32 i = 0; i < ArrayLength(s.tempDepthBuffers); i++)
		{
			String name = String_Format("Temporary Depth Buffer %", i);
			defer { String_Free(name); };

			s.tempDepthBuffers[i] = Renderer_CreateDepthBuffer(*s.renderer, name, true);
			if (!s.tempDepthBuffers[i]) return false;
		}

		u32 outlineCBufSizes[] = {
			{ sizeof(Outline::PSPerPass) }
		};
		s.outlineShader = Renderer_LoadPixelShader(*s.renderer, "Outline", "Shaders/Outline.ps.cso", outlineCBufSizes);
		LOG_IF(!s.outlineShader, return false,
			Severity::Error, "Failed to load outline pixel shader");

		s.outlineCompositeShader = Renderer_LoadPixelShader(*s.renderer, "Outline Composite", "Shaders/Outline Composite.ps.cso", outlineCBufSizes);
		LOG_IF(!s.outlineCompositeShader, return false,
			Severity::Error, "Failed to load outline composite pixel shader");

		s.depthToAlphaShader = Renderer_LoadPixelShader(*s.renderer, "Depth to Alpha", "Shaders/Depth to Alpha.ps.cso", {});
		LOG_IF(!s.depthToAlphaShader, return false,
			Severity::Error, "Failed to load depth to alpha pixel shader");
	}

	// DEBUG: Testing
	{
		Plugin& ohmPlugin = RegisterPlugin(s, "Sensor Plugins\\OpenHardwareMonitor", "Sensor.OpenHardwareMonitor.dll");
		success = LoadSensorPlugin(s, ohmPlugin);
		if (!success) return false;

		Plugin& filledBarPlugin = RegisterPlugin(s, "Widget Plugins\\Filled Bar", "Widget.FilledBar.dll");
		success = LoadWidgetPlugin(s, filledBarPlugin);
		if (!success) return false;

		//u32 debugSensorIndices[] = { 0, 1, 2, 3, 21 }; // Desktop 2080 Ti
		//u32 debugSensorIndices[] = { 6, 7, 8, 9, 33 }; // Desktop 780 Tis
		//u32 debugSensorIndices[] = { 0, 1, 2, 3, 12 }; // Laptop
		u32 debugSensorIndices[] = { u32Max, u32Max, u32Max, u32Max, u32Max }; // Empty

		WidgetPluginRef ref = { filledBarPlugin.rawRefToKind };
		WidgetPlugin& filledBarWidgetPlugin = s.widgetPlugins[ref];
		WidgetData& widgetData = filledBarWidgetPlugin.widgetDatas[0];

		FullWidgetDataRef fullRef = {};
		fullRef.pluginRef = filledBarWidgetPlugin.ref;
		fullRef.dataRef = widgetData.ref;

		v2 positions[ArrayLength(debugSensorIndices)] = {};
		AddWidgets(s, fullRef, positions);

		for (u32 i = 0; i < ArrayLength(debugSensorIndices); i++)
		{
			Widget& widget = widgetData.widgets[i];
			widget.position    = ((v2) s.renderSize) / 2.0f;
			widget.position.y += (2.0f - (r32) i) * (widget.size.y + 3.0f);
			widget.sensorRef   = ToRef<Sensor>(debugSensorIndices[i]);
		}
	}

	// Create a GUI Pipe
	{
		// TODO: Ensure there's only a single connection
		PipeResult result = Platform_CreatePipeServer("LCDHardwareMonitor GUI Pipe", s.guiConnection.pipe);
		LOG_IF(result == PipeResult::UnexpectedFailure, return false,
			Severity::Error, "Failed to create pipe for GUI communication");
	}

	success = Renderer_FinalizeResourceCreation(*s.renderer);
	if (!success) return false;

	return true;
}

void
Simulation_Update(SimulationState& s)
{
	s.currentTime = Platform_GetElapsedSeconds(s.startTime);

	// GUI Communication
	ConnectionState& guiCon = s.guiConnection;
	while (!guiCon.failure)
	{
		// Connection handling
		{
			b8 wasConnected = guiCon.pipe.state == PipeState::Connected;
			PipeResult result = Platform_UpdatePipeConnection(guiCon.pipe);
			HandleMessageResult(guiCon, result);

			b8 isConnected = guiCon.pipe.state == PipeState::Connected;
			if (isConnected && !wasConnected) OnConnect(s);
			if (!isConnected && wasConnected) OnDisconnect(s);
		}
		if (guiCon.pipe.state != PipeState::Connected) break;

		// Receive
		Bytes bytes = {};
		defer { List_Free(bytes); };

		i64 startTicks = Platform_GetTicks();
		while (MessageTimeLeft(startTicks))
		{
			b8 success = ReceiveMessage(guiCon, bytes);
			if (!success) break;

			#define HANDLE_MESSAGE(Type) \
				case IdOf<FromGUI::Type>: \
				{ \
					using Type = FromGUI::Type; \
					DeserializeMessage<Type>(bytes); \
					Type& message = (Type&) bytes[0]; \
					FromGUI_##Type(s, message); \
					break; \
				}

			Message::Header& header = (Message::Header&) bytes[0];
			switch (header.id)
			{
				case IdOf<Message::Null>: Assert(false); break;

				HANDLE_MESSAGE(TerminateSimulation);
				HANDLE_MESSAGE(MouseMove);
				HANDLE_MESSAGE(SelectHovered);
				HANDLE_MESSAGE(BeginMouseLook);
				HANDLE_MESSAGE(EndMouseLook);
				HANDLE_MESSAGE(ResetCamera);
				HANDLE_MESSAGE(SetPluginLoadStates);
				HANDLE_MESSAGE(DragDrop);
				HANDLE_MESSAGE(AddWidget);
				HANDLE_MESSAGE(RemoveWidget);
				HANDLE_MESSAGE(RemoveSelectedWidgets);
				HANDLE_MESSAGE(BeginDragSelection);
				HANDLE_MESSAGE(EndDragSelection);
				HANDLE_MESSAGE(SetWidgetSelection);
			}
		}

		// Send
		while (MessageTimeLeft(startTicks))
		{
			b8 success = SendMessage(guiCon);
			if (!success) break;
		}

		break;
	}

	// TODO: Only update plugins that are actually being used

	// Update Sensors
	{
		PluginContext context = {};
		context.s = &s;

		SensorPluginAPI::Update api = {};
		api.RegisterSensors   = RegisterSensors;
		api.UnregisterSensors = UnregisterSensors;

		for (u32 i = 0; i < s.sensorPlugins.length; i++)
		{
			SensorPlugin& sensorPlugin = s.sensorPlugins[i];

			// TODO: try/catch?
			if (sensorPlugin.functions.Update)
			{
				context.sensorPlugin = &sensorPlugin;
				context.success      = true;

				api.sensors = sensorPlugin.sensors;
				sensorPlugin.functions.Update(context, api);
			}
		}
	}

	Renderer_PushRenderTarget(*s.renderer, StandardRenderTarget::Main);
	Renderer_PushDepthBuffer(*s.renderer, StandardDepthBuffer::Main);
	Renderer_ClearRenderTarget(*s.renderer, Colors128::Clear);
	Renderer_ClearDepthBuffer(*s.renderer);
	Renderer_SetBlendMode(*s.renderer, true);

	// Update Widgets
	{
		// TODO: How sensor values propagate to widgets is an open question. Does
		// the application spin through the widgets and update the values (Con:
		// iterating over the list twice. Con: Lots of sensor lookups)? Do we
		// store a map from sensors to widgets that use them and update widgets
		// when the sensor value changes (Con: complexity maybe)? Do we store a
		// pointer in widgets to the sensor value (Con: Have to patch up the
		// pointer when sensors resize) (could be a relative pointer so update
		// happens on a single base pointer) (Con: drawing likely needs access to
		// the full sensor)?

		PluginContext context = {};
		context.s = &s;

		WidgetPluginAPI::Update pluginAPI = {};

		WidgetAPI::Update widgetAPI = {};
		widgetAPI.t                       = s.currentTime;
		widgetAPI.sensors                 = s.sensorPlugins[0].sensors;
		widgetAPI.GetViewMatrix           = GetViewMatrix;
		widgetAPI.GetProjectionMatrix     = GetProjectionMatrix;
		widgetAPI.GetViewProjectionMatrix = GetViewProjectionMatrix;
		widgetAPI.UpdateVSConstantBuffer  = UpdateVSConstantBuffer;
		widgetAPI.UpdatePSConstantBuffer  = UpdatePSConstantBuffer;
		widgetAPI.DrawMesh                = DrawMesh;
		widgetAPI.PushVertexShader        = PushVertexShader;
		widgetAPI.PushPixelShader         = PushPixelShader;
		widgetAPI.PopVertexShader         = PopVertexShader;
		widgetAPI.PopPixelShader          = PopPixelShader;

		for (u32 i = 0; i < s.widgetPlugins.length; i++)
		{
			WidgetPlugin& widgetPlugin = s.widgetPlugins[i];

			String eventName = String_Format("Update Widgets (%)", widgetPlugin.name);
			defer { String_Free(eventName); };
			Renderer_PushEvent(*s.renderer, eventName);

			context.widgetPlugin = &widgetPlugin;
			context.success      = true;

			// TODO: try/catch?
			if (widgetPlugin.functions.Update)
				widgetPlugin.functions.Update(context, pluginAPI);

			for (u32 j = 0; j < widgetPlugin.widgetDatas.length; j++)
			{
				WidgetData& widgetData = widgetPlugin.widgetDatas[i];
				if (widgetData.widgets.length == 0) continue;

				context.success = true;

				widgetAPI.widgets                = widgetData.widgets;
				widgetAPI.widgetsUserData        = widgetData.widgetsUserData;
				widgetAPI.widgetsUserData.stride = widgetData.desc.userDataSize;

				// TODO: try/catch?
				widgetData.desc.Update(context, widgetAPI);
			}

			Renderer_PopEvent(*s.renderer);
		}
	}

	// Update hovered widget
	if (guiCon.pipe.state == PipeState::Connected || s.previewWindow)
	{
		List_Clear(s.hovered);

		// TODO: Mouse selection is off by just a bit. Floating point issues?
		// TODO: Don't fully understand why the half renderSize offset is necessary
		v4 mousePos = (v4) ((v2) s.mousePos - (v2) s.renderSize / 2.0f);
		mousePos.w = 1.0f;
		mousePos *= s.iview;

		v4 mouseDir = { 0.0f, 0.0f, -1.0f, 0.0f };
		mouseDir *= s.iview;

		r32 selectionDepth = r32Max;
		for (u32 i = 0; i < s.widgetPlugins.length; i++)
		{
			WidgetPlugin& widgetPlugin = s.widgetPlugins[i];
			for (u32 j = 0; j < widgetPlugin.widgetDatas.length; j++)
			{
				WidgetData& widgetData = widgetPlugin.widgetDatas[i];
				for (u32 k = 0; k < widgetData.widgets.length; k++)
				{
					Widget& widget = widgetData.widgets[k];
					if (widget.depth < selectionDepth)
					{
						if (!ApproximatelyZero(mouseDir.z))
						{
							r32 dist = (-widget.depth - mousePos.z) / mouseDir.z;
							v4 selectionPos = mousePos + dist*mouseDir;

							v4 rect = WidgetRect(widget);
							if (RectContains(rect, (v2) selectionPos))
							{
								FullWidgetRef& hovered = List_Append(s.hovered);
								hovered.pluginRef = widgetPlugin.ref;
								hovered.dataRef   = { j + 1 };
								hovered.widgetRef = { k + 1 };

								if (IsWidgetSelected(s, hovered))
								{
									List_Duplicate(s.hovered, Slice(s.selected));
									break;
								}
							}
						}
					}
				}
			}
		}

		// Update highlight animations
		auto initializeHighlight = [](OutlineAnimation& anim, i64 currentTicks) {
			anim.isShowing  = true;
			anim.startTicks = currentTicks;
			anim.duration   = Platform_SecondsToTicks(0.05f);
			anim.startValue = 0.75f;
			anim.endValue   = 1.0f;
		};

		auto initializeFade = [](OutlineAnimation& anim, i64 currentTicks) {
			anim.isShowing  = false;
			anim.startTicks = currentTicks;
			anim.duration   = Platform_SecondsToTicks(0.4f);
			anim.startValue = 1.0f;
			anim.endValue   = 0.0f;
		};

		auto continueFromCurrentAlpha = [](OutlineAnimation& anim) {
			r32 currentAlpha = anim.psPerPass.outlineColor.a;
			r32 progress     = Clamp(InverseLerp(anim.startValue, anim.endValue, currentAlpha), 0.0f, 1.0f);
			i64 offset       = (i64) (progress * (r32) anim.duration);
			anim.startTicks -= offset;
		};

		b8 found = false;
		i64 currentTicks = Platform_GetTicks();
		for (u32 i = s.hoverAnimations.length - 1; (i32) i >= 0 ; i--)
		{
			OutlineAnimation& anim = s.hoverAnimations[i];

			b8 isHovered  = List_Equal(s.hovered, anim.widgets);
			b8 isFading   = !anim.isShowing;
			b8 isComplete = currentTicks >= anim.startTicks + anim.duration;
			found |= isHovered;

			// Update alpha
			i64 hoveredTicks = currentTicks - anim.startTicks;
			r32 hoveredFactor = Clamp((r32) hoveredTicks / (r32) anim.duration, 0.0f, 1.0f);
			anim.psPerPass.outlineColor.a = Lerp(anim.startValue, anim.endValue, hoveredFactor);

			// Was fading, switch to showing
			if (isHovered && isFading)
			{
				initializeHighlight(anim, currentTicks);
				continueFromCurrentAlpha(anim);
			}
			// Was showing, switch to fading
			else if (!isHovered && !isFading)
			{
				initializeFade(anim, currentTicks);
				continueFromCurrentAlpha(anim);
			}
			// Clear completed fades
			else if (isFading && isComplete)
			{
				List_Free(anim.widgets);
				List_RemoveFast(s.hoverAnimations, i);
			}
		}

		// No existing fade to use, start a new highlight
		if (!found && s.hovered.length != 0)
		{
			// A bit wasteful here with allocations, but meh
			OutlineAnimation& anim = List_Append(s.hoverAnimations);
			initializeHighlight(anim, currentTicks);
			anim.widgets   = List_Duplicate(Slice(s.hovered));
			anim.psPerPass = s.outlinePSPerPassHovered;
		}
	}

	// DEBUG: Draw Coordinate System
	{
		Matrix world = Identity();
		SetScale(world, v3 { 2000, 2000, 2000 });
		SetTranslation(world, s.cameraPos);
		static Matrix wvp;
		wvp = world * s.vp;

		PluginContext context = {};
		context.success = true;
		context.s = &s;

		Renderer_PushEvent(*s.renderer, "Draw Debug Coordinates");
		UpdateVSConstantBuffer(context, StandardVertexShader::WVP, 0, &wvp);
		PushVertexShader(context, StandardVertexShader::WVP);
		PushPixelShader(context, StandardPixelShader::DebugCoordinates);
		DrawMesh(context, StandardMesh::Cube);
		PopVertexShader(context);
		PopPixelShader(context);
		Renderer_PopEvent(*s.renderer);
	}

	// TODO: Fill amount is wrong because of the hacky fake sensor value
	// Draw selection and hover
	{
		if (s.selected.length != 0)
		{
			Renderer_PushEvent(*s.renderer, "Outline Selected Widgets");
			HighlightWidgets(s, s.selected, s.outlinePSPerPassSelected);
			Renderer_PopEvent(*s.renderer);
		}

		for (u32 i = 0; i < s.hoverAnimations.length; i++)
		{
			OutlineAnimation& anim = s.hoverAnimations[i];

			b8 skip = anim.isShowing && s.guiInteraction != GUIInteraction::Null;
			if (skip) continue;

			Renderer_PushEvent(*s.renderer, "Outline Hovered Widgets");
			HighlightWidgets(s, anim.widgets, anim.psPerPass);
			Renderer_PopEvent(*s.renderer);
		}
	}

	// Update CPU texture
	{
		Renderer_PushEvent(*s.renderer, "Update CPU Texture");
		Renderer_PushRenderTarget(*s.renderer, StandardRenderTarget::Null);
		Renderer_Copy(*s.renderer, StandardRenderTarget::Main, s.renderTargetCPUCopy);
		Renderer_PopRenderTarget(*s.renderer);
		Renderer_PopEvent(*s.renderer);
	}

	// Update GUI preview texture
	{
		Renderer_PushEvent(*s.renderer, "Update GUI Preview");
		Renderer_PushRenderTarget(*s.renderer, s.renderTargetGUICopy);
		Renderer_PushDepthBuffer(*s.renderer, StandardDepthBuffer::Null);
		Renderer_PushPSResource(*s.renderer, StandardRenderTarget::Main, 0);
		Renderer_PushVertexShader(*s.renderer, StandardVertexShader::ClipSpace);
		Renderer_PushPixelShader(*s.renderer, StandardPixelShader::Composite);
		Renderer_DrawMesh(*s.renderer, StandardMesh::Fullscreen);
		Renderer_PopVertexShader(*s.renderer);
		Renderer_PopPixelShader(*s.renderer);
		Renderer_PopPSResource(*s.renderer, 0);
		Renderer_PopDepthBuffer(*s.renderer);
		Renderer_PopRenderTarget(*s.renderer);
		Renderer_PopEvent(*s.renderer);
	}

	Renderer_PopDepthBuffer(*s.renderer);
	Renderer_PopRenderTarget(*s.renderer);
	Renderer_Render(*s.renderer);

	// Hardware Communication
	{
		if (FT232H_HasError(*s.ft232h))
		{
			s.ft232hRetryCount++;
			s.ft232hInitialized = false;
			ILI9341_Teardown(*s.ili9341);
			FT232H_Teardown(*s.ft232h);
		}

		if (!s.ft232hInitialized && s.ft232hRetryCount < 3)
		{
			FT232H_SetTracing(*s.ft232h, false);
			FT232H_SetDebugChecks(*s.ft232h, true);

			s.ft232hInitialized = FT232H_Initialize(*s.ft232h);
			if (s.ft232hInitialized)
			{
				s.ft232hRetryCount = 0;
				ILI9341_Initialize(*s.ili9341, *s.ft232h);
				ILI9341_BeginDrawFrames(*s.ili9341);
			}
			else
			{
				s.ft232hRetryCount++;
			}
		}

		if (s.ft232hInitialized)
		{
			// TODO: Handle pixel and row strides
			CPUTextureBytes frame = Renderer_GetCPUTextureBytes(*s.renderer, s.renderTargetCPUCopy);
			Assert(frame.pixelStride == sizeof(u16));
			Assert(frame.rowStride == frame.pixelStride * frame.size.x);
			ILI9341_DrawFrame(*s.ili9341, frame.bytes);
		}
	}
}

void
Simulation_Teardown(SimulationState& s)
{
	// TODO: Decide how much we really care about simulation level teardown.
	// Remove this once plugin loading and unloading is solidified. It's good to
	// do this for testing, but it's unnecessary work in the normal teardown
	// case. (We still need to teardown plugins though to give them a chance to
	// save changes and whatnot.)

	ConnectionState& guiCon = s.guiConnection;
	if (guiCon.pipe.state == PipeState::Connected)
		OnTeardown(guiCon);
	Connection_Teardown(guiCon);

	if (s.ft232hInitialized)
	{
		s.ft232hInitialized = false;
		ILI9341_Teardown(*s.ili9341);
		FT232H_Teardown(*s.ft232h);
	}

	for (u32 i = 0; i < s.plugins.length; i++)
	{
		Plugin& plugin = s.plugins[i];
		if (plugin.kind == PluginKind::Widget)
		{
			// TODO: Can probably make this cleaner
			WidgetPluginRef ref = { plugin.rawRefToKind };
			UnloadWidgetPlugin(s, s.widgetPlugins[ref]);
		}
		if (plugin.kind == PluginKind::Sensor)
		{
			SensorPluginRef ref = { plugin.rawRefToKind };
			UnloadSensorPlugin(s, s.sensorPlugins[ref]);
		}
		UnregisterPlugin(s, plugin);
	}
	List_Free(s.widgetPlugins);
	List_Free(s.sensorPlugins);
	List_Free(s.plugins);

	PluginLoader_Teardown(*s.pluginLoader);

	s = {};
}

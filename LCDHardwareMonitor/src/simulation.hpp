#include "Solid Colored.ps.h"

struct FullWidgetRef
{
	WidgetPluginRef pluginRef;
	WidgetDataRef   dataRef;
	WidgetRef       ref;
};

inline b8 operator== (FullWidgetRef lhs, FullWidgetRef rhs)
{
	b8 result = true;
	result &= lhs.pluginRef == rhs.pluginRef;
	result &= lhs.dataRef == rhs.dataRef;
	result &= lhs.ref == rhs.ref;
	return result;
}

inline b8 operator!= (FullWidgetRef lhs, FullWidgetRef rhs)
{
	b8 result = false;
	result |= lhs.pluginRef != rhs.pluginRef;
	result |= lhs.dataRef != rhs.dataRef;
	result |= lhs.ref != rhs.ref;
	return result;
}

struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;
	ConnectionState    guiConnection;

	List<Plugin>       plugins;
	List<SensorPlugin> sensorPlugins;
	List<WidgetPlugin> widgetPlugins;

	v2u                renderSize;
	v3                 cameraPos;
	Matrix             view;
	Matrix             proj;
	Matrix             vp; // TODO: Remove this when simulation talks to the renderer directly
	Matrix             iview;
	i64                startTime;
	r32                currentTime;

	b8                 mouseLook;
	v2                 mousePos;
	v2i                mousePosStart;
	v2                 cameraRot;
	v2                 cameraRotStart;
	FullWidgetRef      hovered;
	FullWidgetRef      selected;
};

struct PluginContext
{
	SimulationState* s;
	SensorPlugin*    sensorPlugin;
	WidgetPlugin*    widgetPlugin;
	b8               success;
};

static Widget*
CreateWidget(Plugin& plugin, WidgetData& widgetData)
{
	Widget* widget = List_Append(widgetData.widgets);
	LOG_IF(!widget, return nullptr,
		Severity::Error, "Failed to allocate Widget");

	b8 success = List_Reserve(widgetData.widgetsUserData, widgetData.desc.userDataSize);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to allocate space for % bytes of Widget user data '%'",
		widgetData.desc.userDataSize, plugin.info.name);
	widgetData.widgetsUserData.length += widgetData.desc.userDataSize;

	return widget;
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

static b8
GetNameFromPath(String& name, StringView path)
{
	if (path.length == 0) return false;

	StrPos first = String_FindLast(path, '/');
	if (first == StrPos::Null)
		first = String_GetFirstPos(path);

	StrPos last = String_FindLast(path, '.');
	if (last == StrPos::Null)
		last = String_GetLastPos(path);

	if (first == last) return false;

	StringSlice nameSlice = StringSlice_Create(path, first, last);

	b8 success = String_FromSlice(name, nameSlice);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate space for name");
	return true;
}

// -------------------------------------------------------------------------------------------------
// Sensor API

// TODO: All these functions should be setting context.success to false when failing
static void
RegisterSensors(PluginContext& context, Slice<Sensor> sensors)
{
	if (!context.success) return;
	context.success = false;

	SensorPlugin& sensorPlugin = *context.sensorPlugin;

	for (u32 i = 0; i < sensors.length; i++)
	{
		Sensor& sensor = sensors[i];
		sensor.ref = { sensorPlugin.sensors.length + i };
	}

	b8 success = List_AppendRange(sensorPlugin.sensors, sensors);
	LOG_IF(!success, return,
		Severity::Error, "Failed to allocate space for % Sensors '%'",
		sensors.length, context.s->plugins[sensorPlugin.pluginRef].info.name);

	// TODO: Re-use empty slots in the list (from removes)

	context.success = true;
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
	context.success = false;

	// TODO: Handle invalid widgetDef

	WidgetPlugin& widgetPlugin = *context.widgetPlugin;

	b8 success;
	for (u32 i = 0; i < widgetDescs.length; i++)
	{
		WidgetDesc& widgetDesc = widgetDescs[i];
		widgetDesc.ref = { widgetPlugin.widgetDatas.length + i };

		WidgetData widgetData = {};
		widgetData.desc = widgetDesc;

		success = List_Reserve(widgetData.widgets, 8);
		LOG_IF(!success, return,
			Severity::Error, "Failed to allocate space for Widgets list");

		success = List_Reserve(widgetData.widgetsUserData, 8 * widgetDesc.userDataSize);
		LOG_IF(!success, return,
			Severity::Error, "Failed to allocate space for Widget user data list. % bytes each '%'",
			widgetDesc.userDataSize, context.s->plugins[widgetPlugin.pluginRef].info.name);

		WidgetData* widgetData2 = List_Append(widgetPlugin.widgetDatas, widgetData);
		LOG_IF(!widgetData2, return,
			Severity::Error, "Failed to allocate WidgetData");
	}

	context.success = true;
}

static void
UnregisterAllWidgets(PluginContext& context)
{
	WidgetPlugin& widgetPlugin = *context.widgetPlugin;
	for (u32 i = 0; i < widgetPlugin.widgetDatas.length; i++)
	{
		WidgetData& widgetData = widgetPlugin.widgetDatas[i];
		List_Free(widgetData.widgets);
		List_Free(widgetData.widgetsUserData);
	}
	List_Free(widgetPlugin.widgetDatas);
}

static PixelShader
LoadPixelShader(PluginContext& context, StringView relPath, Slice<u32> cBufSizes)
{
	if (!context.success) return PixelShader::Null;
	context.success = false;

	b8 success;

	Plugin& plugin = context.s->plugins[context.widgetPlugin->pluginRef];
	RendererState& rendererState = *context.s->renderer;

	String path = {};
	defer { String_Free(path); };
	success = String_Format(path, "%/%", plugin.directory, relPath);
	LOG_IF(!success, return PixelShader::Null,
		Severity::Error, "Failed to format pixel shader path '%'", relPath);

	// TODO: This is super awkward
	StringView psName = {};
	String psNameTemp = {};
	defer { String_Free(psNameTemp); };
	success = GetNameFromPath(psNameTemp, path);
	psName = success ? psNameTemp : path;

	PixelShader ps = Renderer_LoadPixelShader(rendererState, psName, path, cBufSizes);
	LOG_IF(!ps, return PixelShader::Null,
		Severity::Error, "Failed to load pixel shader '%'", path);

	context.success = true;
	return ps;
}

static Material
CreateMaterial(PluginContext& context, Mesh mesh, VertexShader vs, PixelShader ps)
{
	RendererState& rendererState = *context.s->renderer;
	return Renderer_CreateMaterial(rendererState, mesh, vs, ps);
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
PushConstantBufferUpdate(PluginContext& context, Material material, ShaderStage shaderStage, u32 index, void* data)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;

	ConstantBufferUpdate* cBufUpdate = Renderer_PushConstantBufferUpdate(rendererState);
	LOG_IF(!cBufUpdate, return,
		Severity::Error, "Failed to allocate space for constant buffer update");
	cBufUpdate->material    = material;
	cBufUpdate->shaderStage = shaderStage;
	cBufUpdate->index       = index;
	cBufUpdate->data        = data;

	context.success = true;
}

static void
PushDrawCall(PluginContext& context, Material material)
{
	if (!context.success) return;
	context.success = false;

	RendererState& rendererState = *context.s->renderer;

	DrawCall* drawCall = Renderer_PushDrawCall(rendererState);
	LOG_IF(!drawCall, return,
		Severity::Error, "Failed to allocate space for draw call");
	drawCall->material = material;

	context.success = true;
}

// -------------------------------------------------------------------------------------------------
// GUI Messages

// NOTE: None of these functions return error codes because the messaging system handles errors
// internally. It will disconnect and stop attempting to send messages when a failure occurs.

static void
OnConnect(SimulationState& s, ConnectionState& con)
{
	b8 success;
	using namespace Message;

	{
		Connect connect = {};
		connect.version       = LHMVersion;
		connect.renderSurface = (size) Renderer_GetSharedRenderSurface(*s.renderer);
		connect.renderSize.x  = s.renderSize.x;
		connect.renderSize.y  = s.renderSize.y;
		success = SerializeAndQueueMessage(con, connect);
		if (!success) return;
	}

	{
		PluginsAdded pluginsAdded = {};
		pluginsAdded.refs  = List_MemberSlice(s.plugins, &Plugin::ref);
		pluginsAdded.kinds = List_MemberSlice(s.plugins, &Plugin::kind);
		pluginsAdded.infos = List_MemberSlice(s.plugins, &Plugin::info);
		success = SerializeAndQueueMessage(con, pluginsAdded);
		if (!success) return;

		PluginStatesChanged statesChanged = {};
		statesChanged.refs       = List_MemberSlice(s.plugins, &Plugin::ref);
		statesChanged.kinds      = List_MemberSlice(s.plugins, &Plugin::kind);
		statesChanged.loadStates = List_MemberSlice(s.plugins, &Plugin::loadState);
		success = SerializeAndQueueMessage(con, statesChanged);
		if (!success) return;
	}

	{
		SensorsAdded sensorsAdded = {};
		sensorsAdded.sensorPluginRefs = List_MemberSlice(s.sensorPlugins, &SensorPlugin::ref);
		sensorsAdded.sensors          = List_MemberSlice(s.sensorPlugins, &SensorPlugin::sensors);
		success = SerializeAndQueueMessage(con, sensorsAdded);
		if (!success) return;
	}

	{
		for (u32 i = 0; i < s.widgetPlugins.length; i++)
		{
			WidgetPlugin& widgetPlugin = s.widgetPlugins[i];

			// NOTE: It's surprisingly tricky to send a Slice<Slice<T>> when it's backed by
			// a set of List<T>. The inner slices are temporaries and need to be stored.
			Slice<WidgetDesc> descs = List_MemberSlice(widgetPlugin.widgetDatas, &WidgetData::desc);

			WidgetDescsAdded widgetDescsAdded = {};
			widgetDescsAdded.widgetPluginRefs = widgetPlugin.ref;
			widgetDescsAdded.descs            = descs;
			success = SerializeAndQueueMessage(con, widgetDescsAdded);
			if (!success) return;
		}
	}
}

static void
OnDisconnect(SimulationState& s, ConnectionState& con)
{
	s.hovered = {};
	s.selected = {};

	con.sendIndex = 0;
	con.recvIndex = 0;
}

static void
OnTeardown(ConnectionState& con)
{
	using namespace Message;
	PipeResult result;

	Disconnect disconnect = {};
	disconnect.header.id    = IdOf<Disconnect>;
	disconnect.header.index = con.sendIndex - (con.queue.length - con.queueIndex);
	disconnect.header.size  = sizeof(Disconnect);

	Bytes bytes = {};
	bytes.length   = sizeof(Disconnect);
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

static void
OnMouseMove(SimulationState& s, v2i mousePos)
{
	s.mousePos = (v2) mousePos;
	if (!s.mouseLook) return;

	v2i deltaPos = mousePos - s.mousePosStart;
	s.cameraRot = 0.0005f * (v2) deltaPos * 2*r32Pi;

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
OnLeftMouseDown(SimulationState& s, v2i mousePos)
{
	UNUSED(mousePos);
	s.selected = s.hovered;
}

static void
OnLeftMouseUp(SimulationState& s, v2i mousePos)
{
	UNUSED(s);
	UNUSED(mousePos);
}

static void
OnMiddleMouseDown(SimulationState& s, v2i mousePos)
{
	UNUSED(mousePos);

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

static void
OnMiddleMouseUp(SimulationState& s, v2i mousePos)
{
	UNUSED(s);
	UNUSED(mousePos);
}

static void
OnRightMouseDown(SimulationState& s, v2i mousePos)
{
	s.mouseLook      = true;
	s.mousePosStart  = mousePos;
	s.cameraRotStart = s.cameraRot;
}

static void
OnRightMouseUp(SimulationState& s, v2i mousePos)
{
	UNUSED(mousePos);
	s.cameraRot      += s.cameraRotStart;
	s.mouseLook       = false;
	s.mousePosStart   = {};
	s.cameraRotStart  = {};
}

// TODO: Lame.
static b8 LoadSensorPlugin(SimulationState&, Plugin&);
static b8 UnloadSensorPlugin(SimulationState&, SensorPlugin&);
static b8 LoadWidgetPlugin(SimulationState&, Plugin&);
static b8 UnloadWidgetPlugin(SimulationState&, WidgetPlugin&);

static void
OnSetPluginStates(SimulationState& s, Slice<PluginRef> refs, Slice<PluginLoadState> states)
{
	Assert(refs.length == states.length);
	for (u32 i = 0; i < refs.length; i++)
	{
		Plugin& plugin = s.plugins[refs[i]];
		switch (states[i])
		{
			default: Assert(false); break;

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
OnPluginStatesChanged(SimulationState& s, Slice<Plugin> plugins)
{
	using namespace Message;
	if (s.guiConnection.pipe.state != PipeState::Connected) return;

	for (u32 i = 0; i < plugins.length; i++)
	{
		Plugin& plugin = plugins[i];

		PluginStatesChanged statesChanged = {};
		statesChanged.refs       = plugin.ref;
		statesChanged.kinds      = plugin.kind;
		statesChanged.loadStates = plugin.loadState;
		b8 success = SerializeAndQueueMessage(s.guiConnection, statesChanged);
		if (!success) return;
	}
}

static void
OnWidgetSelect(SimulationState& s, FullWidgetRef ref)
{
	// TODO: Validate
	// TODO: Clear on widget remove
	s.selected = ref;
}

// -------------------------------------------------------------------------------------------------

static Plugin*
RegisterPlugin(SimulationState& s, StringView directory, StringView fileName)
{
	b8 success;

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
	{
		plugin = List_Append(s.plugins);
		LOG_IF(!plugin, return nullptr,
			Severity::Error, "Failed to allocate Plugin");
	}

	defer { OnPluginStatesChanged(s, *plugin); };
	auto pluginGuard = guard { plugin->loadState = PluginLoadState::Broken; };

	plugin->ref = List_GetLastRef(s.plugins);

	success = String_FromView(plugin->fileName, fileName);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to copy Plugin file name");

	success = String_FromView(plugin->directory, directory);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to copy Plugin directory");

	pluginGuard.dismiss = true;
	return plugin;
}

static void
UnregisterPlugin(SimulationState& s, Plugin& plugin)
{
	Assert(plugin.loadState != PluginLoadState::Loaded);
	defer { OnPluginStatesChanged(s, plugin); };

	String_Free(plugin.fileName);
	String_Free(plugin.directory);
	String_Free(plugin.info.name);
	String_Free(plugin.info.author);;
	plugin = {};
}

static b8
LoadSensorPlugin(SimulationState& s, Plugin& plugin)
{
	Assert(plugin.loadState != PluginLoadState::Loaded);

	b8 success;

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
	{
		sensorPlugin = List_Append(s.sensorPlugins);
		LOG_IF(!sensorPlugin, return nullptr,
			Severity::Error, "Failed to allocate Sensor plugin");
	}

	sensorPlugin->ref = List_GetLastRef(s.sensorPlugins);
	sensorPlugin->pluginRef = plugin.ref;

	plugin.rawRefToKind = sensorPlugin->ref;
	plugin.kind = PluginKind::Sensor;

	defer { OnPluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	success = PluginLoader_LoadSensorPlugin(*s.pluginLoader, plugin, *sensorPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to load Sensor plugin '%'", plugin.fileName);

	sensorPlugin->name = plugin.info.name;

	success = List_Reserve(sensorPlugin->sensors, 32);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Sensor list for Sensor plugin");

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
		LOG_IF(!success, return false,
			Severity::Error, "Failed to initialize Sensor plugin '%'", plugin.info.name);
	}

	pluginGuard.dismiss = true;
	return true;
}

static b8
UnloadSensorPlugin(SimulationState& s, SensorPlugin& sensorPlugin)
{
	Plugin& plugin = s.plugins[sensorPlugin.pluginRef];
	Assert(plugin.loadState != PluginLoadState::Unloaded);

	defer { OnPluginStatesChanged(s, plugin); };
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

	for (u32 i = 0; i < sensorPlugin.sensors.length; i++)
	{
		SensorRef sensorRef = List_GetRef(sensorPlugin.sensors, i);
		RemoveSensorRefs(s, sensorPlugin.ref, sensorRef);
	}

	b8 success = PluginLoader_UnloadSensorPlugin(*s.pluginLoader, plugin, sensorPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Sensor plugin '%'", plugin.info.name);

	plugin.rawRefToKind = 0;

	// NOTE: Any fields added to SensorPlugin may need to be cleared here.
	List_Free(sensorPlugin.sensors);
	sensorPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static b8
LoadWidgetPlugin(SimulationState& s, Plugin& plugin)
{
	Assert(plugin.loadState != PluginLoadState::Loaded);

	b8 success;

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
	{
		widgetPlugin = List_Append(s.widgetPlugins);
		LOG_IF(!widgetPlugin, return nullptr,
			Severity::Error, "Failed to allocate Widget plugin");
	}

	widgetPlugin->ref = List_GetLastRef(s.widgetPlugins);
	widgetPlugin->pluginRef = plugin.ref;

	plugin.rawRefToKind = widgetPlugin->ref;
	plugin.kind = PluginKind::Widget;

	defer { OnPluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	success = PluginLoader_LoadWidgetPlugin(*s.pluginLoader, plugin, *widgetPlugin);
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
		api.CreateMaterial  = CreateMaterial;

		success = widgetPlugin->functions.Initialize(context, api);
		success &= context.success;
		if (!success)
		{
			LOG(Severity::Error, "Failed to initialize Widget plugin");

			// TODO: There's a decent chance we'll want to keep widget descs
			// around for unloaded plugins
			UnregisterAllWidgets(context);
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

	defer { OnPluginStatesChanged(s, plugin); };
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	PluginContext context = {};
	context.s            = &s;
	context.widgetPlugin = &widgetPlugin;

	WidgetAPI::Teardown wigetAPI = {};
	for (u32 i = 0; i < widgetPlugin.widgetDatas.length; i++)
	{
		WidgetData& widgetData = widgetPlugin.widgetDatas[i];
		if (widgetData.widgets.length == 0) continue;

		context.success = true;

		wigetAPI.widgets                = widgetData.widgets;
		wigetAPI.widgetsUserData        = widgetData.widgetsUserData;
		wigetAPI.widgetsUserData.stride = widgetData.desc.userDataSize;

		// TODO: try/catch?
		if (widgetData.desc.Teardown)
			widgetData.desc.Teardown(context, wigetAPI);
	}

	// TODO: try/catch?
	if (widgetPlugin.functions.Teardown)
	{
		WidgetPluginAPI::Teardown pluginAPI = {};
		widgetPlugin.functions.Teardown(context, pluginAPI);
	}

	UnregisterAllWidgets(context);

	b8 success = PluginLoader_UnloadWidgetPlugin(*s.pluginLoader, plugin, widgetPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Widget plugin '%'", plugin.info.name);

	plugin.rawRefToKind = 0;

	// NOTE: Any fields added to WidgetPlugin may need to be cleared here.
	for (u32 i = 0; i < widgetPlugin.widgetDatas.length; i++)
	{
		WidgetData& widgetData = widgetPlugin.widgetDatas[i];
		List_Free(widgetData.widgets);
		List_Free(widgetData.widgetsUserData);
	}
	List_Free(widgetPlugin.widgetDatas);
	widgetPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static Widget&
GetWidget(SimulationState& s, FullWidgetRef ref)
{
	WidgetPlugin& plugin = s.widgetPlugins[ref.pluginRef];
	WidgetData& data = plugin.widgetDatas[ref.dataRef];
	Widget& widget = data.widgets[ref.ref];
	return widget;
}

b8
Simulation_Initialize(SimulationState& s, PluginLoaderState& pluginLoader, RendererState& renderer)
{
	b8 success;

	s.pluginLoader = &pluginLoader;
	s.renderer     = &renderer;
	s.startTime    = Platform_GetTicks();
	s.renderSize   = { 320, 240 };

	success = PluginLoader_Initialize(*s.pluginLoader);
	if (!success) return false;

	success = List_Reserve(s.sensorPlugins, 8);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Sensor plugins list");

	success = List_Reserve(s.widgetPlugins, 8);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Widget plugins list");

	// Setup Camera
	OnMiddleMouseDown(s, {});

	// Load Default Assets
	{
		// Vertex shader
		{
			VertexAttribute vsAttributes[] = {
				{ VertexAttributeSemantic::Position, VertexAttributeFormat::v3 },
				{ VertexAttributeSemantic::Color,    VertexAttributeFormat::v4 },
				{ VertexAttributeSemantic::TexCoord, VertexAttributeFormat::v2 },
			};

			VertexShader vs = Renderer_LoadVertexShader(*s.renderer, "Default", "Shaders/WVP.vs.cso", vsAttributes, sizeof(Matrix));
			LOG_IF(!vs, return false,
				Severity::Error, "Failed to load built-in wvp vertex shader");
			Assert(vs == StandardVertexShader::WVP);
		}

		// Pixel shader
		{
			PixelShader ps;

			u32 cBufSizes[] = {
				{ sizeof(PSInitialize) },
			};
			ps = Renderer_LoadPixelShader(*s.renderer, "Default", "Shaders/Solid Colored.ps.cso", cBufSizes);
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in solid colored pixel shader");
			Assert(ps == StandardPixelShader::SolidColored);

			ps = Renderer_LoadPixelShader(*s.renderer, "Default", "Shaders/Vertex Colored.ps.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in vertex colored pixel shader");
			Assert(ps == StandardPixelShader::VertexColored);

			ps = Renderer_LoadPixelShader(*s.renderer, "Default", "Shaders/Debug Coordinates.ps.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in vertex colored pixel shader");
			Assert(ps == StandardPixelShader::DebugCoordinates);
		}

		// Triangle mesh
		{
			Vertex vertices[] = {
				{ { -0.5f, -0.433f, 0 }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.0f,  0.433f, 0 }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.5f, 1.0f } },
				{ {  0.5f, -0.433f, 0 }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
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
				{ { -0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f,  0.5f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f,  0.5f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f, -0.5f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
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
			// TODO: Oh no, I think I fucked up the coordinate convention. If +Z
			// is backwards and the camera is looking forward, then we're
			// currently looking at the 'back' of objects. Need to do some
			// flipping because the current bars are drawing backwards and the
			// camera is positioned backwards.

			// NOTE: Directions are from the perspective of the mesh itself, not
			// an onlooker.

			Vertex vertices[] = {
				// Back
				{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },

				// Right
				{ {  0.5f, -0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f,  0.5f,  0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f,  0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f, -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },

				// Front
				{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 0.0f, 0.0f } },
				{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 1.0f, 1.0f } },
				{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 0.5f, 1.0f }, { 1.0f, 0.0f } },

				// Left
				{ { -0.5f, -0.5f, -0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f,  0.5f, -0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ { -0.5f,  0.5f,  0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ { -0.5f, -0.5f,  0.5f }, { 0.5f, 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },

				// Top
				{ { -0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f,  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f,  0.5f,  0.5f }, { 0.0f, 1.0f, 0.0f, 1.0f }, { 1.0f, 0.0f } },

				// Bottom
				{ { -0.5f, -0.5f, -0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 0.0f, 0.0f } },
				{ { -0.5f, -0.5f,  0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 0.0f, 1.0f } },
				{ {  0.5f, -0.5f,  0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 1.0f, 1.0f } },
				{ {  0.5f, -0.5f, -0.5f }, { 0.0f, 0.5f, 0.0f, 1.0f }, { 1.0f, 0.0f } },
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
	}

	// TODO: Watch directory contents for registering, unregistering, and
	// reloading plugins.

	// DEBUG: Testing
	{
		Plugin* ohmPlugin = RegisterPlugin(s, "Sensor Plugins\\OpenHardwareMonitor", "Sensor.OpenHardwareMonitor.dll");
		if (!ohmPlugin) return false;

		success = LoadSensorPlugin(s, *ohmPlugin);
		if (!success) return false;

		Plugin* filledBarPlugin = RegisterPlugin(s, "Widget Plugins\\Filled Bar", "Widget.FilledBar.dll");
		if (!filledBarPlugin) return false;

		success = LoadWidgetPlugin(s, *filledBarPlugin);
		if (!success) return false;

		//u32 debugSensorIndices[] = { 0, 1, 2, 3, 21 }; // Desktop 2080 Ti
		//u32 debugSensorIndices[] = { 6, 7, 8, 9, 33 }; // Desktop 780 Tis
		//u32 debugSensorIndices[] = { 0, 1, 2, 3, 12 }; // Laptop
		u32 debugSensorIndices[] = { u32Max, u32Max, u32Max, u32Max, u32Max }; // Empty
		WidgetPluginRef ref = { filledBarPlugin->rawRefToKind };
		WidgetPlugin& filledBarWidgetPlugin = s.widgetPlugins[ref];
		WidgetData& widgetData = filledBarWidgetPlugin.widgetDatas[0];
		for (u32 i = 0; i < ArrayLength(debugSensorIndices); i++)
		{
			Widget* widget = CreateWidget(*filledBarPlugin, widgetData);
			if (!widget) return false;

			widget->position         = ((v2) s.renderSize - v2{ 240, 12 }) / 2.0f;
			widget->position.y      += ((i32) i - 2) * 15.0f;
			widget->sensorPluginRef  = List_GetRef(s.sensorPlugins, 0);
			widget->sensorRef        = List_GetRef(s.sensorPlugins[0].sensors, debugSensorIndices[i]);

			PluginContext context = {};
			context.s            = &s;
			context.widgetPlugin = &filledBarWidgetPlugin;
			context.success      = true;

			WidgetAPI::Initialize api = {};
			u32 iLast = widgetData.widgets.length - 1;
			api.widgets                  = widgetData.widgets[iLast];
			api.widgetsUserData          = widgetData.widgetsUserData[iLast * widgetData.desc.userDataSize];
			api.PushConstantBufferUpdate = PushConstantBufferUpdate;

			widgetData.desc.Initialize(context, api);
		}
	}

	// Create a GUI Pipe
	{
		using namespace Message;

		// TODO: Ensure there's only a single connection
		PipeResult result = Platform_CreatePipeServer("LCDHardwareMonitor GUI Pipe", s.guiConnection.pipe);
		LOG_IF(result == PipeResult::UnexpectedFailure, return false,
			Severity::Error, "Failed to create pipe for GUI communication");
	}

	success = Renderer_RebuildSharedGeometryBuffers(*s.renderer);
	if (!success) return false;

	return true;
}

void
Simulation_Update(SimulationState& s)
{
	s.currentTime = Platform_GetElapsedSeconds(s.startTime);

	PluginContext context = {};
	context.s = &s;

	// TODO: Only update plugins that are actually being used

	// Update Sensors
	{
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

		WidgetPluginAPI::Update pluginAPI = {};

		WidgetAPI::Update wigetAPI = {};
		wigetAPI.t                        = s.currentTime;
		wigetAPI.sensors                  = s.sensorPlugins[0].sensors;
		wigetAPI.GetViewMatrix            = GetViewMatrix;
		wigetAPI.GetProjectionMatrix      = GetProjectionMatrix;
		wigetAPI.GetViewProjectionMatrix  = GetViewProjectionMatrix;
		wigetAPI.PushConstantBufferUpdate = PushConstantBufferUpdate;
		wigetAPI.PushDrawCall             = PushDrawCall;

		for (u32 i = 0; i < s.widgetPlugins.length; i++)
		{
			WidgetPlugin& widgetPlugin = s.widgetPlugins[i];

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

				wigetAPI.widgets                = widgetData.widgets;
				wigetAPI.widgetsUserData        = widgetData.widgetsUserData;
				wigetAPI.widgetsUserData.stride = widgetData.desc.userDataSize;

				// TODO: try/catch?
				widgetData.desc.Update(context, wigetAPI);
			}
		}
	}

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
			if (isConnected && !wasConnected) OnConnect(s, guiCon);
			if (!isConnected && wasConnected) OnDisconnect(s, guiCon);
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

			using namespace Message;
			Header& header = (Header&) bytes[0];
			switch (header.id)
			{
				default: Assert(false); break;

				case IdOf<TerminateSimulation>:
					Platform_RequestQuit();
					break;

				case IdOf<MouseMove>:
				{
					DeserializeMessage<MouseMove>(bytes);
					MouseMove& mouseMove = (MouseMove&) bytes[0];
					OnMouseMove(s, mouseMove.pos);
					break;
				}

				case IdOf<MouseButtonChange>:
				{
					DeserializeMessage<MouseButtonChange>(bytes);
					MouseButtonChange& mbChange = (MouseButtonChange&) bytes[0];
					switch (mbChange.button)
					{
						case MouseButton::Null: Assert(false);

						case MouseButton::Left:
						{
							Assert(mbChange.state);
							if (mbChange.state == ButtonState::Down)
								OnLeftMouseDown(s, mbChange.pos);
							else
								OnLeftMouseUp(s, mbChange.pos);
							break;
						}

						case MouseButton::Middle:
						{
							Assert(mbChange.state);
							if (mbChange.state == ButtonState::Down)
								OnMiddleMouseDown(s, mbChange.pos);
							else
								OnMiddleMouseUp(s, mbChange.pos);
							break;
						}

						case MouseButton::Right:
						{
							Assert(mbChange.state);
							if (mbChange.state == ButtonState::Down)
								OnRightMouseDown(s, mbChange.pos);
							else
								OnRightMouseUp(s, mbChange.pos);
							break;
						}
					}
					break;
				}

				case IdOf<SetPluginLoadStates>:
				{
					DeserializeMessage<SetPluginLoadStates>(bytes);
					SetPluginLoadStates& setState = (SetPluginLoadStates&) bytes[0];
					OnSetPluginStates(s, setState.refs, setState.loadStates);
					break;
				}
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

	// Simulation Drawing
	PluginContext simContext = {};
	simContext.success = true;
	simContext.s = &s;

	// DEBUG: Draw Coordinate System
	{
		Matrix world = Identity();
		SetScale(world, v3 { 2000, 2000, 2000 });
		SetTranslation(world, s.cameraPos);
		static Matrix wvp;
		wvp = world * s.vp;

		Material material = {};
		material.mesh = StandardMesh::Cube;
		material.vs   = StandardVertexShader::WVP;
		material.ps   = StandardPixelShader::DebugCoordinates;

		PushConstantBufferUpdate(simContext, material, ShaderStage::Vertex, 0, &wvp);
		PushDrawCall(simContext, material);
	}

	// Select mouse over widget
	if (guiCon.pipe.state == PipeState::Connected)
	{
		s.hovered = {};

		// TODO: Mouse selection is off by just a bit. Floating point issues?
		// TODO: Don't fully understand why the half renderSize offset is necessary
		v4 mousePos = (v4) (s.mousePos - (v2) s.renderSize / 2.0f);
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
								s.hovered.pluginRef = widgetPlugin.ref;
								s.hovered.dataRef   = { j + 1 };
								s.hovered.ref       = { k + 1 };
							}
						}
					}
				}
			}
		}
	}

	// TODO: Outline shader? How should we handle depth?
	// Draw Hover
	if (s.hovered.ref && s.hovered != s.selected)
	{
		Widget& widget = GetWidget(s, s.hovered);

		v2 position = WidgetPosition(widget);
		v2 size = widget.size + v2{ 8, 8 };

		Matrix world = Identity();
		SetPosition(world, position, -(widget.depth + 5.0f));
		SetScale   (world, size, 1.0f);
		static Matrix wvp;
		wvp = world * s.vp;
		static v4 color = Color32(28, 151, 234, 255);

		Material material = {};
		material.mesh = StandardMesh::Quad;
		material.vs   = StandardVertexShader::WVP;
		material.ps   = StandardPixelShader::SolidColored;

		PushConstantBufferUpdate(simContext, material, ShaderStage::Vertex, 0, &wvp);
		PushConstantBufferUpdate(simContext, material, ShaderStage::Pixel,  0, &color);
		PushDrawCall(simContext, material);
	}

	// Draw Selection
	if (s.selected.ref)
	{
		Widget& widget = GetWidget(s, s.selected);

		v2 position = WidgetPosition(widget);
		v2 size = widget.size + v2{ 8, 8 };

		Matrix world = Identity();
		SetPosition(world, position, -(widget.depth + 5.0f));
		SetScale   (world, size, 1.0f);
		static Matrix wvp;
		wvp = world * s.vp;
		static v4 color = Color32(0, 122, 204, 255);

		Material material = {};
		material.mesh = StandardMesh::Quad;
		material.vs   = StandardVertexShader::WVP;
		material.ps   = StandardPixelShader::SolidColored;

		PushConstantBufferUpdate(simContext, material, ShaderStage::Vertex, 0, &wvp);
		PushConstantBufferUpdate(simContext, material, ShaderStage::Pixel,  0, &color);
		PushDrawCall(simContext, material);
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
}

struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;

	List<SensorPlugin> sensorPlugins;
	List<WidgetPlugin> widgetPlugins;

	v2u                renderSize;
	v3                 cameraPos;
	Matrix             view;
	Matrix             proj;
	Matrix             vp; // TODO: Remove this when simulation talks to the renderer directly
	i64                startTime;
	r32                currentTime;

	b32                guiConnected;
	Pipe               guiPipe;
	u32                guiSendMsgIndex;
	u32                guiRecvMsgIndex;
	List<Bytes>        guiQueue;
	u32                guiQueueIndex;
	b32                guiFailure;
	u32                guiFailureCount;
};

struct PluginContext
{
	SimulationState* s;
	SensorPlugin*    sensorPlugin;
	WidgetPlugin*    widgetPlugin;
	b32              success;
};

static Widget*
CreateWidget(WidgetPlugin* widgetPlugin, WidgetData* widgetData)
{
	Widget* widget = List_Append(widgetData->widgets);
	LOG_IF(!widget, return nullptr,
		Severity::Error, "Failed to allocate Widget");

	b32 success = List_Reserve(widgetData->widgetsUserData, widgetData->desc.userDataSize);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to allocate space for %u bytes of Widget user data '%s'",
		widgetData->desc.userDataSize, widgetPlugin->info.name.data);
	widgetData->widgetsUserData.length += widgetData->desc.userDataSize;

	return widget;
}

static void
RemoveSensorRefs(SimulationState* s, SensorPluginRef sensorPluginRef, Slice<SensorRef> sensorRefs)
{
	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];
		// TODO: Widget iterator!
		for (u32 j = 0; j < s->widgetPlugins.length; j++)
		{
			WidgetPlugin* widgetPlugin = &s->widgetPlugins[j];
			for (u32 k = 0; k < widgetPlugin->widgetDatas.length; k++)
			{
				WidgetData* widgetData = &widgetPlugin->widgetDatas[k];
				for (u32 l = 0; l < widgetData->widgets.length; l++)
				{
					Widget* widget = &widgetData->widgets[l];
					if (widget->sensorPluginRef == sensorPluginRef && widget->sensorRef == sensorRef)
					{
						widget->sensorPluginRef = SensorPluginRef::Null;
						widget->sensorRef       = SensorRef::Null;
					}
				}
			}
		}
	}
}

static StringSlice
GetNameFromPath(String& path)
{
	if (path.length == 0) return {};

	String::RefT first = List_FindLast(path, '/');
	if (first == String::RefT::Null)
		first = List_GetFirstRef(path);

	String::RefT last = List_FindLast(path, '.');
	if (last == String::RefT::Null)
		last = List_GetLastRef(path);

	if (first == last) return {};

	StringSlice result = {};
	result.length = last.index - first.index + 1;
	result.stride = sizeof(c8);
	result.data   = &path[first];
	return result;
}

// === Sensor API ==================================================================================

static void
RegisterSensors(PluginContext* context, Slice<Sensor> sensors)
{
	if (!context->success) return;
	context->success = false;

	SensorPlugin* sensorPlugin = context->sensorPlugin;

	for (u32 i = 0; i < sensors.length; i++)
	{
		Sensor* sensor = &sensors[i];
		sensor->ref = { sensorPlugin->sensors.length + i };
	}

	b32 success = List_AppendRange(sensorPlugin->sensors, sensors);
	LOG_IF(!success, return,
		Severity::Error, "Failed to allocate space for %u Sensors '%s'",
		sensors.length, context->sensorPlugin->info.name.data);

	// TODO: Re-use empty slots in the list (from removes)

	context->success = true;
}

static void
UnregisterSensors(PluginContext* context, Slice<SensorRef> sensorRefs)
{
	if (!context->success) return;
	context->success = false;

	SensorPlugin* sensorPlugin = context->sensorPlugin;

	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];

		b32 valid = true;
		valid = valid && List_IsRefValid(sensorPlugin->sensors, sensorRef);
		valid = valid && sensorPlugin->sensors[sensorRef].ref == sensorRef;
		LOG_IF(!valid, return,
			Severity::Error, "Sensor plugin gave a bad SensorRef '%s'", sensorPlugin->info.name.data);
	}

	RemoveSensorRefs(context->s, sensorPlugin->ref, sensorRefs);

	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];
		sensorPlugin->sensors[sensorRef] = {};
	}

	context->success = true;
}

// === Widget API ==================================================================================

static void
RegisterWidgets(PluginContext* context, Slice<WidgetDesc> widgetDescs)
{
	if (!context->success) return;
	context->success = false;

	// TODO: Handle invalid widgetDef

	WidgetPlugin* widgetPlugin = context->widgetPlugin;

	b32 success;
	for (u32 i = 0; i < widgetDescs.length; i++)
	{
		WidgetDesc* widgetDesc = &widgetDescs[i];
		widgetDesc->ref = { widgetPlugin->widgetDatas.length + i };

		WidgetData widgetData = {};
		widgetData.desc = *widgetDesc;

		success = List_Reserve(widgetData.widgets, 8);
		LOG_IF(!success, return,
			Severity::Error, "Failed to allocate space for Widget instances list");

		success = List_Reserve(widgetData.widgetsUserData, 8 * widgetDesc->userDataSize);
		LOG_IF(!success, return,
			Severity::Error, "Failed to allocate space for Widget user data list. %u bytes each '%s'",
			widgetDesc->userDataSize, widgetPlugin->info.name.data);

		WidgetData* widgetData2 = List_Append(widgetPlugin->widgetDatas, widgetData);
		LOG_IF(!widgetData2, return,
			Severity::Error, "Failed to allocate WidgetData");
	}

	context->success = true;
}

static void
UnregisterAllWidgets(PluginContext* context)
{
	WidgetPlugin* widgetPlugin = context->widgetPlugin;
	for (u32 i = 0; i < widgetPlugin->widgetDatas.length; i++)
	{
		WidgetData* widgetData = &widgetPlugin->widgetDatas[i];
		List_Free(widgetData->widgets);
		List_Free(widgetData->widgetsUserData);
	}
	List_Free(widgetPlugin->widgetDatas);
}

static PixelShader
LoadPixelShader(PluginContext* context, c8* relPath, Slice<u32> cBufSizes)
{
	if (!context->success) return PixelShader::Null;
	context->success = false;

	WidgetPlugin* widgetPlugin = context->widgetPlugin;

	String path = {};
	defer { List_Free(path); };

	b32 success = String_Format(path, "%s/%s", widgetPlugin->header.directory, relPath);
	LOG_IF(!success, return PixelShader::Null,
		Severity::Error, "Failed to format pixel shader path '%s'", relPath);

	StringSlice psName = GetNameFromPath(path);
	LOG_IF(!psName.data, psName = path,
		Severity::Warning, "Failed to get pixel shader name from path '%s'", relPath);

	PixelShader ps = Renderer_LoadPixelShader(context->s->renderer, psName, path.data, cBufSizes);
	LOG_IF(!ps, return PixelShader::Null,
		Severity::Error, "Failed to load pixel shader '%s'", path.data);

	context->success = true;
	return ps;
}

static Material
CreateMaterial(PluginContext* context, Mesh mesh, VertexShader vs, PixelShader ps)
{
	return Renderer_CreateMaterial(context->s->renderer, mesh, vs, ps);
}

static Matrix
GetViewMatrix(PluginContext* context)
{
	return context->s->view;
}

static Matrix
GetProjectionMatrix(PluginContext* context)
{
	return context->s->proj;
}

static Matrix
GetViewProjectionMatrix(PluginContext* context)
{
	return context->s->vp;
}

static void
PushConstantBufferUpdate(PluginContext* context, Material material, ShaderStage shaderStage, u32 index, void* data)
{
	if (!context->success) return;
	context->success = false;

	ConstantBufferUpdate* cBufUpdate = Renderer_PushConstantBufferUpdate(context->s->renderer);
	LOG_IF(!cBufUpdate, return,
		Severity::Error, "Failed to allocate space for constant buffer update");
	cBufUpdate->material    = material;
	cBufUpdate->shaderStage = shaderStage;
	cBufUpdate->index       = index;
	cBufUpdate->data        = data;

	context->success = true;
}

static void
PushDrawCall(PluginContext* context, Material material)
{
	if (!context->success) return;
	context->success = false;

	DrawCall* drawCall = Renderer_PushDrawCall(context->s->renderer);
	LOG_IF(!drawCall, return,
		Severity::Error, "Failed to allocate space for draw call");
	drawCall->material = material;

	context->success = true;
}

// =================================================================================================

static SensorPlugin*
LoadSensorPlugin(SimulationState* s, c8* directory, c8* fileName)
{
	b32 success;

	SensorPlugin* sensorPlugin = List_Append(s->sensorPlugins);
	LOG_IF(!sensorPlugin, return nullptr,
		Severity::Error, "Failed to allocate Sensor plugin");

	auto pluginGuard = guard { sensorPlugin->header.loadState = PluginLoadState::Broken; };

	sensorPlugin->ref              = List_GetLastRef(s->sensorPlugins);
	sensorPlugin->header.fileName  = fileName;
	sensorPlugin->header.directory = directory;
	sensorPlugin->header.kind      = PluginKind::Sensor;

	success = PluginLoader_LoadSensorPlugin(s->pluginLoader, sensorPlugin);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to load Sensor plugin '%s'", fileName);

	success = List_Reserve(sensorPlugin->sensors, 32);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to allocate Sensor list for Sensor plugin");

	// TODO: try/catch?
	if (sensorPlugin->functions.initialize)
	{
		PluginContext context = {};
		context.s            = s;
		context.sensorPlugin = sensorPlugin;
		context.success      = true;

		SensorPluginAPI::Initialize api = {};
		api.RegisterSensors = RegisterSensors;

		success = sensorPlugin->functions.initialize(&context, api);
		success &= context.success;
		LOG_IF(!success, return nullptr,
			Severity::Error, "Failed to initialize Sensor plugin '%s'", sensorPlugin->info.name.data);
	}

	pluginGuard.dismiss = true;
	return sensorPlugin;
}

static b32
UnloadSensorPlugin(SimulationState* s, SensorPlugin* sensorPlugin)
{
	auto pluginGuard = guard { sensorPlugin->header.loadState = PluginLoadState::Broken; };

	// TODO: try/catch?
	if (sensorPlugin->functions.teardown)
	{
		PluginContext context = {};
		context.s            = s;
		context.sensorPlugin = sensorPlugin;
		context.success      = true;

		SensorPluginAPI::Teardown api = {};
		api.sensors = sensorPlugin->sensors;

		sensorPlugin->functions.teardown(&context, api);
	}

	for (u32 i = 0; i < sensorPlugin->sensors.length; i++)
	{
		SensorRef sensorRef = List_GetRef(sensorPlugin->sensors, i);
		RemoveSensorRefs(s, sensorPlugin->ref, sensorRef);
	}

	b32 success = PluginLoader_UnloadSensorPlugin(s->pluginLoader, sensorPlugin);
	List_Free(sensorPlugin->sensors);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Sensor plugin '%s'", sensorPlugin->info.name.data);

	// TODO: Add and remove plugin infos based on directory contents instead of
	// loaded state.
	*sensorPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static WidgetPlugin*
LoadWidgetPlugin(SimulationState* s, c8* directory, c8* fileName)
{
	b32 success;

	WidgetPlugin* widgetPlugin = List_Append(s->widgetPlugins);
	auto pluginGuard = guard { widgetPlugin->header.loadState = PluginLoadState::Broken; };

	LOG_IF(!widgetPlugin, return nullptr,
		Severity::Error, "Failed to allocate WidgetPlugin");

	widgetPlugin->ref              = List_GetLastRef(s->widgetPlugins);
	widgetPlugin->header.fileName  = fileName;
	widgetPlugin->header.directory = directory;
	widgetPlugin->header.kind      = PluginKind::Widget;

	success = PluginLoader_LoadWidgetPlugin(s->pluginLoader, widgetPlugin);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to load Widget plugin '%s'", fileName);

	// TODO: try/catch?
	if (widgetPlugin->functions.initialize)
	{
		PluginContext context = {};
		context.s            = s;
		context.widgetPlugin = widgetPlugin;
		context.success      = true;

		WidgetPluginAPI::Initialize api = {};
		api.RegisterWidgets = RegisterWidgets;
		api.LoadPixelShader = LoadPixelShader;
		api.CreateMaterial  = CreateMaterial;

		success = widgetPlugin->functions.initialize(&context, api);
		success &= context.success;
		if (!success)
		{
			LOG(Severity::Error, "Failed to initialize Widget plugin");

			// TODO: There's a decent chance we'll want to keep widget descs
			// around for unloaded plugins
			UnregisterAllWidgets(&context);
			return nullptr;
		}
	}

	pluginGuard.dismiss = true;
	return widgetPlugin;
}

static b32
UnloadWidgetPlugin(SimulationState* s, WidgetPlugin* widgetPlugin)
{
	auto pluginGuard = guard { widgetPlugin->header.loadState = PluginLoadState::Broken; };

	PluginContext context = {};
	context.s            = s;
	context.widgetPlugin = widgetPlugin;

	WidgetInstanceAPI::Teardown instancesAPI = {};
	for (u32 i = 0; i < widgetPlugin->widgetDatas.length; i++)
	{
		WidgetData* widgetData = &widgetPlugin->widgetDatas[i];
		if (widgetData->widgets.length == 0) continue;

		context.success = true;

		instancesAPI.widgets                = widgetData->widgets;
		instancesAPI.widgetsUserData        = widgetData->widgetsUserData;
		instancesAPI.widgetsUserData.stride = widgetData->desc.userDataSize;

		// TODO: try/catch?
		if (widgetData->desc.teardown)
			widgetData->desc.teardown(&context, instancesAPI);
	}

	// TODO: try/catch?
	if (widgetPlugin->functions.teardown)
	{
		WidgetPluginAPI::Teardown api = {};
		widgetPlugin->functions.teardown(&context, api);
	}

	UnregisterAllWidgets(&context);

	b32 success = PluginLoader_UnloadWidgetPlugin(s->pluginLoader, widgetPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Widget plugin '%s'", widgetPlugin->info.name.data);

	// TODO: Add and remove plugin infos based on directory contents instead
	// of loaded state.
	*widgetPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static void
HandleGUIResult(SimulationState* s, b32 success)
{
	using namespace Message;

	if (success)
	{
		s->guiFailureCount = 0;
	}
	else
	{
		s->guiFailureCount++;
		if (s->guiFailureCount < 3)
		{
			LOG(Severity::Error, "A GUI operation failed. Retrying...");
		}
		else
		{
			LOG(Severity::Error, "Too many GUI operations have failed. Giving up.");

			s->guiFailure = true;

			PipeResult result = Platform_DisconnectPipe(&s->guiPipe);
			LOG_IF(result != PipeResult::Success, IGNORE,
				Severity::Error, "Failed to disconnect GUI pipe");
		}
	}
}

// TODO: The error messages in here suck
template <typename T>
static void
SendGUIMessage(SimulationState* s, T& message)
{
	Bytes bytes = {};
	b32 success = SerializeMessage(bytes, message, s->guiSendMsgIndex);
	HandleGUIResult(s, success);
	if (!success) return;

	Bytes* result = List_Append(s->guiQueue, bytes);
	HandleGUIResult(s, result != nullptr);
	if (result == nullptr) return;

	s->guiSendMsgIndex++;
}

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	b32 success;

	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;
	s->startTime    = Platform_GetTicks();
	s->renderSize   = { 320, 240 };

	success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	success = List_Reserve(s->sensorPlugins, 8);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Sensor plugins list");

	success = List_Reserve(s->widgetPlugins, 8);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Widget plugins list");

	// Setup Camera
	{
		v2 offset = (v2) s->renderSize / 2.0f;
		v3 pos    = { offset.x, offset.y, 500 };
		v3 target = { offset.x, offset.y, 0 };

		s->cameraPos = pos;
		s->view      = LookAt(pos, target);
		s->proj      = Orthographic((v2) s->renderSize, 0, 10000);
		s->vp        = s->view * s->proj;
	}

	// Load Default Assets
	{
		// Vertex shader
		{
			VertexAttribute vsAttributes[] = {
				{ VertexAttributeSemantic::Position, VertexAttributeFormat::v3 },
				{ VertexAttributeSemantic::Color,    VertexAttributeFormat::v4 },
				{ VertexAttributeSemantic::TexCoord, VertexAttributeFormat::v2 },
			};

			VertexShader vs = Renderer_LoadVertexShader(s->renderer, "Default", "Shaders/WVP.vs.cso", vsAttributes, sizeof(Matrix));
			LOG_IF(!vs, return false,
				Severity::Error, "Failed to load built-in wvp vertex shader");
			Assert(vs == StandardVertexShader::WVP);
		}

		// Pixel shader
		{
			PixelShader ps;

			ps = Renderer_LoadPixelShader(s->renderer, "Default", "Shaders/Vertex Colored.ps.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load built-in vertex colored pixel shader");
			Assert(ps == StandardPixelShader::VertexColored);

			ps = Renderer_LoadPixelShader(s->renderer, "Default", "Shaders/Debug Coordinates.ps.cso", {});
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

			Mesh triangle = Renderer_CreateMesh(s->renderer, "Triangle Mesh", vertices, indices);
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

			Mesh quad = Renderer_CreateMesh(s->renderer, "Quad Mesh", vertices, indices);
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

			Mesh quad = Renderer_CreateMesh(s->renderer, "Cube Mesh", vertices, indices);
			Assert(quad == StandardMesh::Cube);
		}
	}

	// DEBUG: Testing
	{
		SensorPlugin* ohmPlugin = LoadSensorPlugin(s, "Sensor Plugins\\OpenHardwareMonitor", "Sensor.OpenHardwareMonitor.dll");
		if (!ohmPlugin) return false;

		WidgetPlugin* filledBarPlugin = LoadWidgetPlugin(s, "Widget Plugins\\Filled Bar", "Widget.FilledBar.dll");
		if (!filledBarPlugin) return false;

		u32 debugSensorIndices[] = { 0, 1, 2, 3, 21 }; // Desktop 2080 Ti
		//u32 debugSensorIndices[] = { 6, 7, 8, 9, 33 }; // Desktop 780 Tis
		//u32 debugSensorIndices[] = { 0, 1, 2, 3, 12 }; // Laptop
		//u32 debugSensorIndices[] = { u32Max, u32Max, u32Max, u32Max, u32Max }; // Empty
		WidgetData* widgetData = &filledBarPlugin->widgetDatas[0];
		for (u32 i = 0; i < ArrayLength(debugSensorIndices); i++)
		{
			Widget* widget = CreateWidget(filledBarPlugin, widgetData);
			if (!widget) return false;

			widget->position         = ((v2) s->renderSize - v2{ 240, 12 }) / 2.0f;
			widget->position.y      += ((i32) i - 2) * 15.0f;
			widget->sensorPluginRef  = List_GetRef(s->sensorPlugins, 0);
			widget->sensorRef        = List_GetRef(s->sensorPlugins[0].sensors, debugSensorIndices[i]);

			PluginContext context = {};
			context.s            = s;
			context.widgetPlugin = filledBarPlugin;
			context.success      = true;

			WidgetInstanceAPI::Initialize api = {};
			u32 iLast = widgetData->widgets.length - 1;
			api.widgets                  = widgetData->widgets[iLast];
			api.widgetsUserData          = widgetData->widgetsUserData[iLast * widgetData->desc.userDataSize];
			api.PushConstantBufferUpdate = PushConstantBufferUpdate;

			widgetData->desc.initialize(&context, api);
		}
	}

	// Create a GUI Pipe
	{
		using namespace Message;

		// TODO: Ensure there's only a single connection
		PipeResult result = Platform_CreatePipeServer("LCDHardwareMonitor GUI Pipe", &s->guiPipe);
		LOG_IF(result == PipeResult::UnexpectedFailure, return false,
			Severity::Error, "Failed to create pipe for GUI communication");
	}

	success = Renderer_RebuildSharedGeometryBuffers(s->renderer);
	if (!success) return false;

	return true;
}

void
Simulation_Update(SimulationState* s)
{
	s->currentTime = Platform_GetElapsedSeconds(s->startTime);

	PluginContext context = {};
	context.s = s;

	// TODO: Only update plugins that are actually being used

	// Update Sensors
	{
		SensorPluginAPI::Update api = {};
		api.RegisterSensors   = RegisterSensors;
		api.UnregisterSensors = UnregisterSensors;

		for (u32 i = 0; i < s->sensorPlugins.length; i++)
		{
			SensorPlugin* sensorPlugin = &s->sensorPlugins[i];

			// TODO: try/catch?
			if (sensorPlugin->functions.update)
			{
				context.sensorPlugin = sensorPlugin;
				context.success      = true;

				api.sensors = sensorPlugin->sensors;
				sensorPlugin->functions.update(&context, api);
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

		WidgetInstanceAPI::Update instancesAPI = {};
		instancesAPI.t                        = s->currentTime;
		instancesAPI.sensors                  = s->sensorPlugins[0].sensors;
		instancesAPI.GetViewMatrix            = GetViewMatrix;
		instancesAPI.GetProjectionMatrix      = GetProjectionMatrix;
		instancesAPI.GetViewProjectionMatrix  = GetViewProjectionMatrix;
		instancesAPI.PushConstantBufferUpdate = PushConstantBufferUpdate;
		instancesAPI.PushDrawCall             = PushDrawCall;

		for (u32 i = 0; i < s->widgetPlugins.length; i++)
		{
			WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];

			context.widgetPlugin = widgetPlugin;
			context.success      = true;

			// TODO: try/catch?
			if (widgetPlugin->functions.update)
				widgetPlugin->functions.update(&context, pluginAPI);

			for (u32 j = 0; j < widgetPlugin->widgetDatas.length; j++)
			{
				WidgetData* widgetData = &widgetPlugin->widgetDatas[i];
				if (widgetData->widgets.length == 0) continue;

				context.success = true;

				instancesAPI.widgets                = widgetData->widgets;
				instancesAPI.widgetsUserData        = widgetData->widgetsUserData;
				instancesAPI.widgetsUserData.stride = widgetData->desc.userDataSize;

				// TODO: try/catch?
				widgetData->desc.update(&context, instancesAPI);
			}
		}
	}

	// GUI Communication
	if (!s->guiFailure)
	{
		// Connect
		if (!s->guiConnected)
		{
			PipeResult result = Platform_ConnectPipe(&s->guiPipe);
			HandleGUIResult(s, result != PipeResult::UnexpectedFailure);

			// Blast the entire current state
			if (s->guiPipe.state == PipeState::Connected)
			{
				s->guiConnected = true;
				using namespace Message;

				{
					Connect connect = {};
					connect.version       = LHMVersion;
					connect.renderSurface = (size) Renderer_GetSharedRenderSurface(s->renderer);
					connect.renderSize.x  = s->renderSize.x;
					connect.renderSize.y  = s->renderSize.y;
					SendGUIMessage(s, connect);
				}

				{
					// TODO: Send PluginHeader along with PluginInfo
					PluginsAdded pluginsAdded = {};
					pluginsAdded.kind  = PluginKind::Sensor;
					pluginsAdded.refs  = List_MemberSlice(s->sensorPlugins, &SensorPlugin::ref);
					pluginsAdded.infos = List_MemberSlice(s->sensorPlugins, &SensorPlugin::info);
					SendGUIMessage(s, pluginsAdded);

					PluginStatesChanged statesChanged = {};
					statesChanged.kind       = PluginKind::Sensor;
					statesChanged.refs       = List_MemberSlice(s->sensorPlugins, &SensorPlugin::ref);
					statesChanged.loadStates = List_MemberSlice(s->sensorPlugins, &SensorPlugin::header, &PluginHeader::loadState);
					SendGUIMessage(s, statesChanged);
				}

				{
					PluginsAdded pluginsAdded = {};
					pluginsAdded.kind  = PluginKind::Widget;
					pluginsAdded.refs  = List_MemberSlice(s->widgetPlugins, &WidgetPlugin::ref);
					pluginsAdded.infos = List_MemberSlice(s->widgetPlugins, &WidgetPlugin::info);
					SendGUIMessage(s, pluginsAdded);

					PluginStatesChanged statesChanged = {};
					statesChanged.kind       = PluginKind::Widget;
					statesChanged.refs       = List_MemberSlice(s->widgetPlugins, &WidgetPlugin::ref);
					statesChanged.loadStates = List_MemberSlice(s->widgetPlugins, &WidgetPlugin::header, &PluginHeader::loadState);
					SendGUIMessage(s, statesChanged);
				}

				{
					SensorsAdded sensorsAdded = {};
					sensorsAdded.pluginRefs = List_MemberSlice(s->sensorPlugins, &SensorPlugin::ref);
					sensorsAdded.sensors    = List_MemberSlice(s->sensorPlugins, &SensorPlugin::sensors);
					SendGUIMessage(s, sensorsAdded);
				}

				{
					for (u32 i = 0; i < s->widgetPlugins.length; i++)
					{
						WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];

						// NOTE: It's surprisingly tricky to send a Slice<Slice<T>> when it's backed by
						// a set of List<T>. The inner slices are temporaries and need to be stored.
						Slice<WidgetDesc> descs = List_MemberSlice(widgetPlugin->widgetDatas, &WidgetData::desc);

						WidgetDescsAdded widgetDescsAdded = {};
						widgetDescsAdded.pluginRefs = widgetPlugin->ref;
						widgetDescsAdded.descs      = descs;
						SendGUIMessage(s, widgetDescsAdded);
					}
				}
			}
		}

		// TODO: Flush this buffer on guiFailure
		// TODO: Probably don't want to let this buffer build up in high traffic scenarios
		// Send
		for (; s->guiQueueIndex < s->guiQueue.length; s->guiQueueIndex++)
		{
			Bytes bytes = s->guiQueue[s->guiQueueIndex];
			PipeResult result = Platform_WritePipe(&s->guiPipe, bytes);
			HandleGUIResult(s, result != PipeResult::UnexpectedFailure);

			if (result != PipeResult::Success) break;
		}
		if (s->guiQueueIndex == s->guiQueue.length)
		{
			for (u32 i = 0; i < s->guiQueue.length; i++)
				s->guiQueue[i].length = 0;
			s->guiQueue.length = 0;
			s->guiQueueIndex = 0;
		}

		// Receive
		for (u32 i = 0; i < 1; i++)
		{
			// TODO: Implement a CheckConnection function
			// NOTE: This read also ensures we'll detect pipe disconnects. Otherwise when we have no
			// messages to send we'll chug along thinking the pipe is still connected indefinintely
			// and never allow reconnections.

			using namespace Message;

			Bytes bytes = {};
			defer { List_Free(bytes); };

			// TODO: Loop, deserialize
			PipeResult result = Platform_ReadPipe(&s->guiPipe, bytes);
			HandleGUIResult(s, result != PipeResult::UnexpectedFailure);
			if (result != PipeResult::Success) continue;

			if (bytes.length == 0) continue;

			// TODO: Possible failures in Update. Uh-oh.
			LOG_IF(bytes.length < sizeof(Header), return /*false*/,
				Severity::Warning, "Corrupted message received");

			Header* header = (Header*) bytes.data;

			LOG_IF(bytes.length != header->size, return /*false*/,
				Severity::Warning, "Incorrectly sized message received");

			LOG_IF(header->index != s->guiRecvMsgIndex, return /*false*/,
				Severity::Warning, "Unexpected message received");
			s->guiRecvMsgIndex++;

			switch (header->id)
			{
				default: Assert(false); break;
				case IdOf<Null>: break;

				case IdOf<TerminateSimulation>:
					Platform_RequestQuit();
					break;
			}
		}

		// Reset on disconnects
		{
			if (s->guiPipe.state == PipeState::Disconnecting
			 || s->guiPipe.state == PipeState::Disconnected)
			{
				s->guiConnected = false;
				s->guiSendMsgIndex = 0;
				s->guiRecvMsgIndex = 0;
			}
		}
	}

	// DEBUG: Draw Coordinate System
	{
		Matrix world = Identity();
		SetScale(world, v3 { 2000, 2000, 2000 });
		SetTranslation(world, s->cameraPos);
		static Matrix wvp;
		wvp = world * s->vp;

		Material material = {};
		material.mesh = StandardMesh::Cube;
		material.vs   = StandardVertexShader::WVP;
		material.ps   = StandardPixelShader::DebugCoordinates;

		PluginContext context2 = {};
		context2.success = true;
		context2.s = s;
		PushConstantBufferUpdate(&context2, material, ShaderStage::Vertex, 0, &wvp);
		PushDrawCall(&context2, material);
	}
}

void
Simulation_Teardown(SimulationState* s)
{
	// TODO: Decide how much we really care about simulation level teardown.
	// Remove this once plugin loading and unloading is solidified. It's good to
	// do this for testing, but it's unnecessary work in the normal teardown
	// case.

	if (s->guiConnected)
	{
		using namespace Message;

		Disconnect disconnect = {};
		disconnect.header.id    = IdOf<Disconnect>;
		disconnect.header.index = s->guiSendMsgIndex - (s->guiQueue.length - s->guiQueueIndex);
		disconnect.header.size  = sizeof(Disconnect);

		Bytes bytes = {};
		bytes.length   = sizeof(Disconnect);
		bytes.capacity = bytes.length;
		bytes.data     = (u8*) &disconnect;

		PipeResult result = Platform_WritePipe(&s->guiPipe, bytes);
		LOG_IF(result != PipeResult::Success, IGNORE,
				Severity::Error, "Failed to send GUI disconnect signal");
	}

	for (u32 i = 0; i < s->guiQueue.capacity; i++)
		List_Free(s->guiQueue[i]);
	List_Free(s->guiQueue);

	PipeResult result = Platform_FlushPipe(&s->guiPipe);
	LOG_IF(result == PipeResult::UnexpectedFailure, IGNORE,
		Severity::Error, "Failed to flush GUI communication pipe");
	Platform_DestroyPipe(&s->guiPipe);

	for (u32 i = 0; i < s->widgetPlugins.length; i++)
		UnloadWidgetPlugin(s, &s->widgetPlugins[i]);
	List_Free(s->widgetPlugins);

	for (u32 i = 0; i < s->sensorPlugins.length; i++)
		UnloadSensorPlugin(s, &s->sensorPlugins[i]);
	List_Free(s->sensorPlugins);

	PluginLoader_Teardown(s->pluginLoader);
}

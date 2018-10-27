struct SimulationState
{
	PluginLoaderState*     pluginLoader;
	RendererState*         renderer;

	v2u                    renderSize = { 320, 240 };
	List<PluginHeader>     pluginHeaders;
	List<SensorPlugin>     sensorPlugins;
	List<WidgetPlugin>     widgetPlugins;
	List<WidgetDefinition> widgetDefinitions;

	u64 startTime;
	r32 currentTime;
};

struct PluginContext
{
	SimulationState* s;
	PluginHeaderRef  ref;
	b32              success;
	DrawCall         onFailDC;
};

static PluginHeader*
CreatePluginHeader(SimulationState* s, c8* directory, c8* name, PluginKind kind)
{
	// Reuse any empty spots left by unloading plugins
	PluginHeader* pluginHeader = nullptr;
	for (u32 i = 0; i < s->pluginHeaders.length; i++)
	{
		PluginHeader* pluginSlot = &s->pluginHeaders[i];
		if (!pluginSlot->name)
		{
			pluginHeader = pluginSlot;
			break;
		}
	}
	if (!pluginHeader)
		pluginHeader = List_Append(s->pluginHeaders);

	pluginHeader->ref       = List_GetRef(s->pluginHeaders, s->pluginHeaders.length - 1);
	pluginHeader->name      = name;
	pluginHeader->directory = directory;
	pluginHeader->kind      = kind;

	return pluginHeader;
}

static PluginHeader*
GetPluginHeader(SimulationState* s, PluginHeaderRef ref)
{
	PluginHeader* pluginHeader = &s->pluginHeaders[ref];
	Assert(pluginHeader->ref == ref);
	return pluginHeader;
}

static SensorPlugin*
GetSensorPlugin(SimulationState* s, PluginHeaderRef ref)
{
	for (u32 i = 0; i < s->sensorPlugins.length; i++)
	{
		SensorPlugin* sensorPlugin = &s->sensorPlugins[i];
		if (sensorPlugin->pluginHeaderRef == ref)
			return sensorPlugin;
	}

	// Unreachable
	Assert(false);
	return nullptr;
}

static WidgetPlugin*
GetWidgetPlugin(SimulationState* s, PluginHeaderRef ref)
{
	for (u32 i = 0; i < s->widgetPlugins.length; i++)
	{
		WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];
		if (widgetPlugin->pluginHeaderRef == ref)
			return widgetPlugin;
	}

	// Unreachable
	Assert(false);
	return nullptr;
}

static SensorPlugin*
LoadSensorPlugin(SimulationState* s, c8* directory, c8* name)
{
	// TODO: Plugin name in errors

	PluginHeader* pluginHeader = CreatePluginHeader(s, directory, name, PluginKind::Sensor);

	// TODO: Ensure this is removed from the list in partial initialization conditions
	SensorPlugin* sensorPlugin = List_Append(s->sensorPlugins);
	sensorPlugin->pluginHeaderRef = pluginHeader->ref;

	b32 success;
	success = PluginLoader_LoadSensorPlugin(s->pluginLoader, pluginHeader, sensorPlugin);
	LOG_IF(!success, "Failed to load sensor plugin", Severity::Warning, return nullptr);

	List_Reserve(sensorPlugin->sensors, 32);
	LOG_IF(!sensorPlugin->sensors, "Sensor allocation failed", Severity::Error, return nullptr);

	// TODO: try/catch?
	if (sensorPlugin->initialize)
	{
		PluginContext context = {};
		context.s       = s;
		context.ref     = pluginHeader->ref;
		context.success = true;

		SensorPlugin::InitializeAPI api = {};
		success = sensorPlugin->initialize(&context, api);
		success &= context.success;
		LOG_IF(!success, "Failed to initialize sensor plugin", Severity::Warning, return nullptr);
	}

	pluginHeader->isWorking = true;
	return sensorPlugin;
}

static b32
UnloadSensorPlugin(SimulationState* s, SensorPlugin* sensorPlugin)
{
	// TODO: Plugin name in errors

	PluginHeader* pluginHeader = GetPluginHeader(s, sensorPlugin->pluginHeaderRef);
	auto pluginGuard = guard { pluginHeader->isWorking = false; };

	// TODO: try/catch?
	if (sensorPlugin->teardown)
	{
		PluginContext context = {};
		context.s       = s;
		context.ref     = pluginHeader->ref;
		context.success = true;

		SensorPlugin::TeardownAPI api = {};
		sensorPlugin->teardown(&context, api);
	}

	b32 success;
	success = PluginLoader_UnloadSensorPlugin(s->pluginLoader, pluginHeader, sensorPlugin);
	// TODO: Widgets will be referencing these sensors.
	List_Free(sensorPlugin->sensors);
	*sensorPlugin = {};
	LOG_IF(!success, "Failed to unload sensor plugin.", Severity::Warning, return false);

	// TODO: Add and remove plugin headers based on directory contents instead of loaded state.
	*pluginHeader = {};

	pluginGuard.dismiss = true;
	return true;
}

static void
AddWidgetDefinition(PluginContext* context, WidgetDefinition* widgetDef)
{
	if (!context->success) return;
	context->success = false;

	// TODO: Use empty slots
	widgetDef = List_Append(context->s->widgetDefinitions, *widgetDef);
	LOG_IF(!widgetDef, "Failed to allocate space for widget definition", Severity::Warning, return);

	widgetDef->ref = context->ref;

	context->success = true;
}

static void
RemoveWidgetDefinitions(PluginContext* context)
{
	for (u32 i = 0; i < context->s->widgetDefinitions.length; i++)
	{
		WidgetDefinition* widgetDef = &context->s->widgetDefinitions[i];
		if (widgetDef->ref == context->ref)
			*widgetDef = {};
	}
}

static PixelShader
LoadPixelShader(PluginContext* context, c8* _path, Slice<ConstantBufferDesc> cBufDescs)
{
	if (!context->success) return PixelShader::Null;
	context->success = false;

	PluginHeader* pluginHeader = GetPluginHeader(context->s, context->ref);
	String path = {};
	// TODO: Why doesn't this return the string?
	context->success = String_Format(path, "%s/%s", pluginHeader->directory, _path);
	LOG_IF(!context->success, "Failed to format pixel shader path", Severity::Warning, return PixelShader::Null);

	PixelShader ps = Renderer_LoadPixelShader(context->s->renderer, path, cBufDescs);
	LOG_IF(!ps, "Failed to load pixel shader", Severity::Warning, return PixelShader::Null);

	context->success = true;
	return ps;
}

static WidgetPlugin*
LoadWidgetPlugin(SimulationState* s, c8* directory, c8* name)
{
	PluginHeader* pluginHeader = CreatePluginHeader(s, directory, name, PluginKind::Widget);

	WidgetPlugin* widgetPlugin = List_Append(s->widgetPlugins);
	widgetPlugin->pluginHeaderRef = pluginHeader->ref;

	b32 success;
	success = PluginLoader_LoadWidgetPlugin(s->pluginLoader, pluginHeader, widgetPlugin);
	LOG_IF(!success, "Failed to load widget plugin", Severity::Warning, return nullptr);

	// TODO: try/catch?
	if (widgetPlugin->initialize)
	{
		// TODO: Hoist these parameters up
		PluginContext context = {};
		context.s       = s;
		context.ref     = pluginHeader->ref;
		context.success = true;

		WidgetPlugin::InitializeAPI api = {};
		api.AddWidgetDefinition = AddWidgetDefinition;
		api.LoadPixelShader     = LoadPixelShader;

		success = widgetPlugin->initialize(&context, api);
		success &= context.success;
		if (!success)
		{
			LOG("Failed to initialize widget plugin", Severity::Warning);

			// TODO: There's a decent chance we'll want to keep widget definitions
			// around for unloaded plugins
			RemoveWidgetDefinitions(&context);
			return nullptr;
		}
	}

	pluginHeader->isWorking = true;
	return widgetPlugin;
}

static b32
UnloadWidgetPlugin(SimulationState* s, WidgetPlugin* widgetPlugin)
{
	// TODO: Plugin name in errors

	PluginHeader* pluginHeader = GetPluginHeader(s, widgetPlugin->pluginHeaderRef);
	auto pluginGuard = guard { pluginHeader->isWorking = false; };

	PluginContext context = {};
	context.s       = s;
	context.ref     = pluginHeader->ref;
	context.success = true;

	// TODO: try/catch?
	if (widgetPlugin->teardown)
	{
		WidgetPlugin::TeardownAPI api = {};
		widgetPlugin->teardown(&context, api);
	}

	RemoveWidgetDefinitions(&context);

	b32 success;
	success = PluginLoader_UnloadWidgetPlugin(s->pluginLoader, pluginHeader, widgetPlugin);
	LOG_IF(!success, "Failed to unload widget plugin.", Severity::Warning, return false);

	// TODO: Add and remove plugin headers based on directory contents instead
	// of loaded state.
	*widgetPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static Widget*
CreateWidget(WidgetDefinition* widgetDef)
{
	u32 widgetOffset = widgetDef->instances.length;
	u32 size = sizeof(Widget) + widgetDef->size;

	b32 success = List_Reserve(widgetDef->instances, widgetDef->instances.length + size);
	LOG_IF(!success, "Failed to allocate widget", Severity::Error, return nullptr);
	widgetDef->instances.length += size;

	Widget* widget = (Widget*) &widgetDef->instances[widgetOffset];
	return widget;
}

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;
	s->startTime    = Platform_GetTicks();

	b32 success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	List_Reserve(s->pluginHeaders, 16);
	LOG_IF(!s->pluginHeaders, "Failed to allocate plugin headers list", Severity::Error, return false);

	List_Reserve(s->sensorPlugins, 8);
	LOG_IF(!s->sensorPlugins, "Failed to allocate sensor plugins list", Severity::Error, return false);

	List_Reserve(s->widgetPlugins, 8);
	LOG_IF(!s->widgetPlugins, "Failed to allocate widget plugins list", Severity::Error, return false);

	List_Reserve(s->widgetDefinitions, 8);
	LOG_IF(!s->widgetDefinitions, "Failed to allocate widget definitions list", Severity::Error, return false);

	// Load Default Assets
	{
		// Vertex shader
		{
			VertexAttribute vsAttributes[] = {
				{ VertexAttributeSemantic::Position, VertexAttributeFormat::Float3 },
				{ VertexAttributeSemantic::Color,    VertexAttributeFormat::Float4 },
				{ VertexAttributeSemantic::TexCoord, VertexAttributeFormat::Float2 },
			};

			ConstantBufferDesc cBufDesc = {};
			cBufDesc.size      = sizeof(Matrix);
			cBufDesc.frequency = ConstantBufferFrequency::PerObject;

			VertexShader vs = Renderer_LoadVertexShader(s->renderer, "Shaders/Basic Vertex Shader.cso", vsAttributes, cBufDesc);
			LOG_IF(!vs, "Failed to load default vertex shader", Severity::Error, return false);
			Assert(vs == StandardVertexShader::Debug);
		}

		// Pixel shader
		{
			PixelShader ps = Renderer_LoadPixelShader(s->renderer, "Shaders/Basic Pixel Shader.cso", {});
			LOG_IF(!ps, "Failed to load default pixel shader", Severity::Error, return false);
			Assert(ps == StandardPixelShader::Debug);
		}
	}

	// DEBUG: Testing
	{
		//SensorPlugin* ohmPlugin = LoadSensorPlugin(s, "Sensor Plugins\\OpenHardwareMonitor", "Sensor Plugin - OpenHardwareMonitor");
		//if (!ohmPlugin) return false;

		//Widget* w = List_Append(s->widgets);
		//w->mesh   = Mesh::Quad;
		//w->sensor = List_GetRef(ohmPlugin->sensors, 0);
		//w->size   = { 240, 12 };

		WidgetPlugin* filledBarPlugin = LoadWidgetPlugin(s, "Widget Plugins\\Filled Bar", "Widget Plugin - Filled Bar");
		if (!filledBarPlugin) return false;

		WidgetDefinition* widgetDef = &s->widgetDefinitions[0];
		Widget* widget = CreateWidget(widgetDef);
		if (!widget) return false;
		widget->position = { 10, 10 };
		widgetDef->initialize(widget);
	}

	return true;
}

static void
PushDrawCall(PluginContext* context, DrawCall dc)
{
	if (!context->success) return;
	context->success = false;

	DrawCall* drawCall = Renderer_PushDrawCall(context->s->renderer);
	LOG_IF(!drawCall, "Failed to allocate space for draw call", Severity::Warning, return);
	*drawCall = dc;

	context->success = true;
}

static Matrix*
GetWVPPointer(PluginContext* context)
{
	return Renderer_GetWVPPointer(context->s->renderer);
}

void
Simulation_Update(SimulationState* s)
{
	PluginContext context = {};
	context.s = s;

	// TODO: Only update plugins that are actually being used

	// Update Sensors
	{
		SensorPlugin::UpdateAPI api = {};
		for (u32 i = 0; i < s->sensorPlugins.length; i++)
		{
			SensorPlugin* sensorPlugin = &s->sensorPlugins[i];

			// TODO: try/catch?
			if (sensorPlugin->update)
			{
				context.ref     = sensorPlugin->pluginHeaderRef;
				context.success = true;
				sensorPlugin->update(&context, api);
			}
		}
	}

	// Update Widgets
	#if false
	{
		WidgetPlugin::UpdateAPI api = {};
		for (u32 i = 0; i < s->widgetPlugins.length; i++)
		{
			WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];

			// TODO: try/catch?
			if (widgetPlugin->update)
			{
				context.ref     = widgetPlugin->pluginHeaderRef;
				context.success = true;
				widgetPlugin->update(&context, api);
			}
		}
	}
	#else
	{
		WidgetPlugin::UpdateAPI api = {};
		api.t             = Platform_GetElapsedSeconds(s->startTime);
		api.PushDrawCall  = PushDrawCall;
		api.GetWVPPointer = GetWVPPointer;

		for (u32 i = 0; i < s->widgetDefinitions.length; i++)
		{
			WidgetDefinition* widgetDef = &s->widgetDefinitions[i];

			if (widgetDef->instances.length > 0)
			{
				WidgetPlugin* widgetPlugin = GetWidgetPlugin(s, widgetDef->ref);

				// DEBUG: Remove this once we have proper API to draw widgets
				Assert(widgetPlugin->update);

				// TODO: try/catch?
				if (widgetPlugin->update)
				{
					context.ref     = widgetPlugin->pluginHeaderRef;
					context.success = true;

					api.widgetDefinition = widgetDef;
					widgetPlugin->update(&context, api);
				}
			}
		}
	}
	#endif
}

void
Simulation_Teardown(SimulationState* s)
{
	// TODO: Decide how much we really care about simulation level teardown.
	// Remove this once plugin loading and unloading is solidified. It's good to
	// do this for testing, but it's unnecessary work in the normal teardown
	// case.

	for (u32 i = 0; i < s->widgetDefinitions.length; i++)
		List_Free(s->widgetDefinitions[i].instances);
	List_Free(s->widgetDefinitions);

	for (u32 i = 0; i < s->widgetPlugins.length; i++)
		UnloadWidgetPlugin(s, &s->widgetPlugins[i]);
	List_Free(s->widgetPlugins);

	for (u32 i = 0; i < s->sensorPlugins.length; i++)
		UnloadSensorPlugin(s, &s->sensorPlugins[i]);
	List_Free(s->sensorPlugins);

	List_Free(s->pluginHeaders);

	PluginLoader_Teardown(s->pluginLoader);
}

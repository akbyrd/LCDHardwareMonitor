struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;

	v2u                renderSize = { 320, 240 };
	List<SensorPlugin> sensorPlugins;
	List<WidgetPlugin> widgetPlugins;

	u64 startTime;
	r32 currentTime;
};

struct PluginContext
{
	SimulationState* s;
	SensorPluginRef  sensorPluginRef;
	WidgetPluginRef  widgetPluginRef;
	b32              success;
};

static SensorPlugin*
GetSensorPlugin(PluginContext* context)
{
	Assert(context->sensorPluginRef);
	return &context->s->sensorPlugins[context->sensorPluginRef];
}

static WidgetPlugin*
GetWidgetPlugin(PluginContext* context)
{
	Assert(context->widgetPluginRef);
	return &context->s->widgetPlugins[context->widgetPluginRef];
}

static Widget*
CreateWidget(WidgetType* widgetType)
{
	u32 widgetOffset = widgetType->instances.length;
	u32 size = sizeof(Widget) + widgetType->definition.size;

	b32 success = List_Reserve(widgetType->instances, widgetType->instances.length + size);
	LOG_IF(!success, "Failed to allocate widget", Severity::Error, return nullptr);
	widgetType->instances.length += size;

	Widget* widget = (Widget*) &widgetType->instances[widgetOffset];
	return widget;
}

// === Sensor API ==================================================================================

// TODO: Refactor this to take a Slice<Sensor>
// TODO: Implement RemoveSensors
// TODO: Re-use empty slots in the list (from removes)
static void
AddSensor(PluginContext* context, Sensor sensor)
{
	if (!context->success) return;
	context->success = false;

	SensorPlugin* sensorPlugin = GetSensorPlugin(context);

	Sensor* sensor2 = List_Append(sensorPlugin->sensors, sensor);
	LOG_IF(!sensor2, "Failed to allocate space for sensor", Severity::Warning, return);

	context->success = true;
}

// === Widget API ==================================================================================

// TODO: Refactor this to take a Slice<WidgetDefinition>
static void
AddWidgetDefinition(PluginContext* context, WidgetDefinition widgetDef)
{
	if (!context->success) return;
	context->success = false;

	WidgetPlugin* widgetPlugin = GetWidgetPlugin(context);

	WidgetType widgetType = {};
	widgetType.definition = widgetDef;

	List_Reserve(widgetType.instances, 8);
	LOG_IF(!widgetType.instances, "Failed to allocate widget instances list", Severity::Warning, return);

	// TODO: Use empty slots
	WidgetType* widgetType2 = List_Append(widgetPlugin->widgetTypes, widgetType);
	LOG_IF(!widgetType2, "Failed to allocate space for widget definition", Severity::Warning, return);

	context->success = true;
}

static void
RemoveAllWidgetDefinitions(PluginContext* context)
{
	WidgetPlugin* widgetPlugin = GetWidgetPlugin(context);
	for (u32 i = 0; i < widgetPlugin->widgetTypes.length; i++)
	{
		WidgetType* widgetType = &widgetPlugin->widgetTypes[i];
		List_Clear(widgetType->instances);
	}
	List_Clear(widgetPlugin->widgetTypes);
}

static PixelShader
LoadPixelShader(PluginContext* context, c8* _path, Slice<ConstantBufferDesc> cBufDescs)
{
	if (!context->success) return PixelShader::Null;
	context->success = false;

	WidgetPlugin* widgetPlugin = GetWidgetPlugin(context);

	String path = {};
	// TODO: Why doesn't this return the string?
	context->success = String_Format(path, "%s/%s", widgetPlugin->header.directory, _path);
	LOG_IF(!context->success, "Failed to format pixel shader path", Severity::Warning, return PixelShader::Null);

	PixelShader ps = Renderer_LoadPixelShader(context->s->renderer, path, cBufDescs);
	LOG_IF(!ps, "Failed to load pixel shader", Severity::Warning, return PixelShader::Null);

	context->success = true;
	return ps;
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

// =================================================================================================

static SensorPlugin*
LoadSensorPlugin(SimulationState* s, c8* directory, c8* name)
{
	// TODO: Plugin name in errors

	// TODO: Ensure this is removed from the list in partial initialization conditions
	// TODO: Can fail?
	SensorPlugin* sensorPlugin = List_Append(s->sensorPlugins);
	sensorPlugin->ref              = List_GetRefLast(s->sensorPlugins);
	sensorPlugin->header.fileName  = name;
	sensorPlugin->header.directory = directory;
	sensorPlugin->header.kind      = PluginKind::Sensor;

	b32 success = PluginLoader_LoadSensorPlugin(s->pluginLoader, sensorPlugin);
	LOG_IF(!success, "Failed to load sensor plugin", Severity::Warning, return nullptr);

	List_Reserve(sensorPlugin->sensors, 32);
	LOG_IF(!sensorPlugin->sensors, "Sensor allocation failed", Severity::Error, return nullptr);

	// TODO: try/catch?
	if (sensorPlugin->functions.initialize)
	{
		PluginContext context = {};
		context.s               = s;
		context.sensorPluginRef = sensorPlugin->ref;
		context.success         = true;

		SensorPluginAPI::Initialize api = {};
		api.AddSensor = AddSensor;

		success = sensorPlugin->functions.initialize(&context, api);
		success &= context.success;
		LOG_IF(!success, "Failed to initialize sensor plugin", Severity::Warning, return nullptr);
	}

	sensorPlugin->header.isWorking = true;
	return sensorPlugin;
}

static b32
UnloadSensorPlugin(SimulationState* s, SensorPlugin* sensorPlugin)
{
	// TODO: Plugin name in errors

	auto pluginGuard = guard { sensorPlugin->header.isWorking = false; };

	// TODO: try/catch?
	if (sensorPlugin->functions.teardown)
	{
		PluginContext context = {};
		context.s               = s;
		context.sensorPluginRef = sensorPlugin->ref;
		context.success         = true;

		SensorPluginAPI::Teardown api = {};
		api.sensors = sensorPlugin->sensors;

		sensorPlugin->functions.teardown(&context, api);
	}

	b32 success;
	success = PluginLoader_UnloadSensorPlugin(s->pluginLoader, sensorPlugin);
	// TODO: Widgets will be referencing these sensors.
	List_Free(sensorPlugin->sensors);
	LOG_IF(!success, "Failed to unload sensor plugin.", Severity::Warning, return false);

	// TODO: Add and remove plugin infos based on directory contents instead of
	// loaded state.
	*sensorPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static WidgetPlugin*
LoadWidgetPlugin(SimulationState* s, c8* directory, c8* name)
{
	// TODO: Can fail?
	WidgetPlugin* widgetPlugin = List_Append(s->widgetPlugins);
	widgetPlugin->ref              = List_GetRefLast(s->widgetPlugins);
	widgetPlugin->header.fileName  = name;
	widgetPlugin->header.directory = directory;
	widgetPlugin->header.kind      = PluginKind::Widget;

	b32 success;
	success = PluginLoader_LoadWidgetPlugin(s->pluginLoader, widgetPlugin);
	LOG_IF(!success, "Failed to load widget plugin", Severity::Warning, return nullptr);

	// TODO: try/catch?
	if (widgetPlugin->functions.initialize)
	{
		// TODO: Hoist these parameters up
		PluginContext context = {};
		context.s               = s;
		context.widgetPluginRef = widgetPlugin->ref;
		context.success         = true;

		WidgetPluginAPI::Initialize api = {};
		api.AddWidgetDefinition = AddWidgetDefinition;
		api.LoadPixelShader     = LoadPixelShader;

		success = widgetPlugin->functions.initialize(&context, api);
		success &= context.success;
		if (!success)
		{
			LOG("Failed to initialize widget plugin", Severity::Warning);

			// TODO: There's a decent chance we'll want to keep widget definitions
			// around for unloaded plugins
			RemoveAllWidgetDefinitions(&context);
			return nullptr;
		}
	}

	widgetPlugin->header.isWorking = true;
	return widgetPlugin;
}

static b32
UnloadWidgetPlugin(SimulationState* s, WidgetPlugin* widgetPlugin)
{
	// TODO: Plugin name in errors

	auto pluginGuard = guard { widgetPlugin->header.isWorking = false; };

	PluginContext context = {};
	context.s               = s;
	context.widgetPluginRef = widgetPlugin->ref;
	context.success         = true;

	// TODO: try/catch?
	if (widgetPlugin->functions.teardown)
	{
		WidgetPluginAPI::Teardown api = {};
		widgetPlugin->functions.teardown(&context, api);
	}

	RemoveAllWidgetDefinitions(&context);

	b32 success;
	success = PluginLoader_UnloadWidgetPlugin(s->pluginLoader, widgetPlugin);
	LOG_IF(!success, "Failed to unload widget plugin.", Severity::Warning, return false);

	// TODO: Add and remove plugin infos based on directory contents instead
	// of loaded state.
	*widgetPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;
	s->startTime    = Platform_GetTicks();

	b32 success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	List_Reserve(s->sensorPlugins, 8);
	LOG_IF(!s->sensorPlugins, "Failed to allocate sensor plugins list", Severity::Error, return false);

	List_Reserve(s->widgetPlugins, 8);
	LOG_IF(!s->widgetPlugins, "Failed to allocate widget plugins list", Severity::Error, return false);

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
		SensorPlugin* ohmPlugin = LoadSensorPlugin(s, "Sensor Plugins\\OpenHardwareMonitor", "Sensor Plugin - OpenHardwareMonitor");
		if (!ohmPlugin) return false;

		WidgetPlugin* filledBarPlugin = LoadWidgetPlugin(s, "Widget Plugins\\Filled Bar", "Widget Plugin - Filled Bar");
		if (!filledBarPlugin) return false;

		#if false
		WidgetType* widgetType = &filledBarPlugin->widgetTypes[0];
		for (i32 i = 0; i < 16; i++)
		{
			Widget* widget = CreateWidget(widgetType);
			if (!widget) return false;

			//widget->sensor = List_GetRef(ohmPlugin->sensors, 0);
			widget->position = { i * 4.0f + 10.0f, i * 15.0f + 2.0f};
			widget->sensor = SensorRef::Null;
			widgetType->definition.initialize(widget);
		}
		#else
		WidgetType* widgetType = &filledBarPlugin->widgetTypes[0];
		for (i32 i = 0; i < 5; i++)
		{
			Widget* widget = CreateWidget(widgetType);
			if (!widget) return false;

			widget->position = ((v2) s->renderSize - v2{ 240, 12 }) / 2.0f;
			widget->position.y += (i - 2) * 15.0f;
			widget->sensor = SensorRef::Null;
			widgetType->definition.initialize(widget);
		}
		u32 stride = sizeof(Widget) + widgetType->definition.size;
		Widget* widget;
		widget = (Widget*) (widgetType->instances.data +  0*stride); widget->sensor.index =  6; // CPU 0 %
		widget = (Widget*) (widgetType->instances.data +  1*stride); widget->sensor.index =  7; // CPU 1 %
		widget = (Widget*) (widgetType->instances.data +  2*stride); widget->sensor.index =  8; // CPU 2 %
		widget = (Widget*) (widgetType->instances.data +  3*stride); widget->sensor.index =  9; // CPU 3 %
		widget = (Widget*) (widgetType->instances.data +  4*stride); widget->sensor.index = 33; // GPU %
		widget = nullptr;
		#endif
	}

	return true;
}

void
Simulation_Update(SimulationState* s)
{
	PluginContext context = {};
	context.s = s;

	// TODO: Only update plugins that are actually being used

	// Update Sensors
	{
		SensorPluginAPI::Update api = {};
		for (u32 i = 0; i < s->sensorPlugins.length; i++)
		{
			SensorPlugin* sensorPlugin = &s->sensorPlugins[i];

			// TODO: try/catch?
			if (sensorPlugin->functions.update)
			{
				context.sensorPluginRef = sensorPlugin->ref;
				context.success         = true;

				api.sensors = sensorPlugin->sensors;
				sensorPlugin->functions.update(&context, api);
			}
		}
	}

	// Update Widgets
	{
		WidgetPluginAPI::Update api = {};
		api.t             = Platform_GetElapsedSeconds(s->startTime);
		// HACK TODO: Fuuuuck. Sensors separated by plugin, so refs need to know
		// which plugin they're referring to. Maaaybe a Slice<Slice<Sensor>> will
		// work once we add a stride? Can also make it a 2 part ref: plugin +
		// sensor.
		api.sensors       = s->sensorPlugins[0].sensors;
		api.PushDrawCall  = PushDrawCall;
		api.GetWVPPointer = GetWVPPointer;

		for (u32 i = 0; i < s->widgetPlugins.length; i++)
		{
			WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];
			if (!widgetPlugin->functions.update) continue;

			for (u32 j = 0; j < widgetPlugin->widgetTypes.length; j++)
			{
				WidgetType* widgetType = &widgetPlugin->widgetTypes[i];
				if (widgetType->instances.length == 0) continue;

				// TODO: How sensor values propagate to widgets is an open
				// question. Does the application spin through the widgets and
				// update the values (Con: iterating over the list twice. Con: Lots
				// of sensor lookups)? Do we store a map from sensors to widgets
				// that use them and update widgets when the sensor value changes
				// (Con: complexity maybe)? Do we store a pointer in widgets to the
				// sensor value (Con: Have to patch up the pointer when sensors
				// resize) (could be a relative pointer so update happens on a
				// single base pointer) (Con: drawing likely needs access to the
				// full sensor)?

				context.widgetPluginRef = widgetPlugin->ref;
				context.success         = true;

				// TODO: try/catch?
				api.widgetInstances = widgetType->instances;
				widgetPlugin->functions.update(&context, api);
			}
		}
	}
}

void
Simulation_Teardown(SimulationState* s)
{
	// TODO: Decide how much we really care about simulation level teardown.
	// Remove this once plugin loading and unloading is solidified. It's good to
	// do this for testing, but it's unnecessary work in the normal teardown
	// case.

	for (u32 i = 0; i < s->widgetPlugins.length; i++)
		UnloadWidgetPlugin(s, &s->widgetPlugins[i]);
	List_Free(s->widgetPlugins);

	for (u32 i = 0; i < s->sensorPlugins.length; i++)
		UnloadSensorPlugin(s, &s->sensorPlugins[i]);
	List_Free(s->sensorPlugins);

	PluginLoader_Teardown(s->pluginLoader);
}

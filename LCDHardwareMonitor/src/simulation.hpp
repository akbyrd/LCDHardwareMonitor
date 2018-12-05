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
	Widget* widget = List_Append(widgetType->widgets);
	LOG_IF(!widget, return nullptr,
		Severity::Error, "Failed to allocate Widget");

	b32 success = List_Reserve(widgetType->widgetData, widgetType->definition.size);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to allocate Widget data list");
	widgetType->widgetData.length += widgetType->definition.size;

	return widget;
}

static void
RemoveSensorRefs(SimulationState* s, Slice<SensorRef> sensorRefs)
{
	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];
		// TODO: Widget iterator!
		for (u32 j = 0; j < s->widgetPlugins.length; j++)
		{
			WidgetPlugin* widgetPlugin = &s->widgetPlugins[j];
			for (u32 k = 0; k < widgetPlugin->widgetTypes.length; k++)
			{
				WidgetType* widgetType = &widgetPlugin->widgetTypes[k];
				for (u32 l = 0; l < widgetType->widgets.length; l++)
				{
					Widget* widget = &widgetType->widgets[l];
					if (widget->sensorRef.plugin == sensorRef.plugin
					 && widget->sensorRef.sensor == sensorRef.sensor)
						widget->sensorRef = SensorRef::Null;
				}
			}
		}
	}
}

static Slice<c8>
GetNameFromPath(String& path)
{
	if (path.length == 0) return {};

	String::RefT first = List_FindLast(path, '/');
	if (first == String::RefT::Null)
		first = List_GetFirst(path);

	String::RefT last = List_FindLast(path, '.');
	if (last == String::RefT::Null)
		last = List_GetLast(path);

	if (first == last) return {};

	Slice<c8> result = {};
	result.length = last.index - first.index + 1;
	result.stride = sizeof(c8);
	result.data   = (u8*) &path[first];
	return result;
}

// === Sensor API ==================================================================================

static void
AddSensors(PluginContext* context, Slice<Sensor> sensors)
{
	if (!context->success) return;
	context->success = false;

	SensorPlugin* sensorPlugin = GetSensorPlugin(context);

	b32 success = List_AppendRange(sensorPlugin->sensors, sensors);
	LOG_IF(!success, return,
		Severity::Error, "Failed to allocate Sensors");

	// TODO: Re-use empty slots in the list (from removes)

	context->success = true;
}

static void
RemoveSensors(PluginContext* context, Slice<SensorRef> sensorRefs)
{
	if (!context->success) return;
	context->success = false;

	SensorPlugin* sensorPlugin = GetSensorPlugin(context);

	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];

		b32 valid = true;
		valid = valid && sensorRef.plugin != sensorPlugin->ref;
		valid = valid && List_IsRefValid(sensorPlugin->sensors, sensorRef.sensor);
		valid = valid && sensorPlugin->sensors[sensorRef.sensor].name != nullptr;
		LOG_IF(!valid, return,
			Severity::Error, "Sensor plugin gave a bad SensorRef '%s'", sensorPlugin->info.name);
	}

	RemoveSensorRefs(context->s, sensorRefs);

	for (u32 i = 0; i < sensorRefs.length; i++)
	{
		SensorRef sensorRef = sensorRefs[i];
		sensorPlugin->sensors[sensorRef.sensor] = {};
	}

	context->success = true;
}

// === Widget API ==================================================================================

static void
AddWidgetDefinitions(PluginContext* context, Slice<WidgetDefinition> widgetDefs)
{
	if (!context->success) return;
	context->success = false;

	// TODO: Handle invalid widgetDef

	WidgetPlugin* widgetPlugin = GetWidgetPlugin(context);

	b32 success;
	for (u32 i = 0; i < widgetDefs.length; i++)
	{
		WidgetDefinition* widgetDef = &widgetDefs[i];

		WidgetType widgetType = {};
		widgetType.definition = *widgetDef;

		success = List_Reserve(widgetType.widgets, 8);
		LOG_IF(!success, return,
			Severity::Error, "Failed to allocate Widget instances list");

		success = List_Reserve(widgetType.widgetData, 8 * widgetDef->size);
		LOG_IF(!success, return,
			Severity::Error, "Failed to allocate Widget data list");

		WidgetType* widgetType2 = List_Append(widgetPlugin->widgetTypes, widgetType);
		LOG_IF(!widgetType2, return,
			Severity::Error, "Failed to allocate WidgetType");
	}

	context->success = true;
}

static void
RemoveAllWidgetDefinitions(PluginContext* context)
{
	WidgetPlugin* widgetPlugin = GetWidgetPlugin(context);
	for (u32 i = 0; i < widgetPlugin->widgetTypes.length; i++)
	{
		WidgetType* widgetType = &widgetPlugin->widgetTypes[i];
		List_Free(widgetType->widgets);
		List_Free(widgetType->widgetData);
	}
	List_Free(widgetPlugin->widgetTypes);
}

static PixelShader
LoadPixelShader(PluginContext* context, c8* relPath, Slice<ConstantBufferDesc> cBufDescs)
{
	if (!context->success) return PixelShader::Null;
	context->success = false;

	WidgetPlugin* widgetPlugin = GetWidgetPlugin(context);

	String path = {};
	defer { List_Free(path); };

	b32 success = String_Format(path, "%s/%s", widgetPlugin->header.directory, relPath);
	LOG_IF(!success, return PixelShader::Null,
		Severity::Error, "Failed to format pixel shader path '%s'", relPath);

	Slice<c8> psName = GetNameFromPath(path);
	LOG_IF(!psName.data, psName = path,
		Severity::Warning, "Failed to get pixel shader name from path '%s'", relPath);

	PixelShader ps = Renderer_LoadPixelShader(context->s->renderer, psName, path.data, cBufDescs);
	LOG_IF(!ps, return PixelShader::Null,
		Severity::Error, "Failed to load pixel shader '%s'", path);

	context->success = true;
	return ps;
}

static void
PushDrawCall(PluginContext* context, DrawCall dc)
{
	if (!context->success) return;
	context->success = false;

	DrawCall* drawCall = Renderer_PushDrawCall(context->s->renderer);
	LOG_IF(!drawCall, return,
		Severity::Error, "Failed to allocate space for draw call");
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
LoadSensorPlugin(SimulationState* s, c8* directory, c8* fileName)
{
	b32 success;

	SensorPlugin* sensorPlugin = List_Append(s->sensorPlugins);
	LOG_IF(!sensorPlugin, return nullptr,
		Severity::Error, "Failed to allocate Sensor plugin");

	sensorPlugin->ref              = List_GetLast(s->sensorPlugins);
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
		context.s               = s;
		context.sensorPluginRef = sensorPlugin->ref;
		context.success         = true;

		SensorPluginAPI::Initialize api = {};
		api.AddSensors = AddSensors;

		success = sensorPlugin->functions.initialize(&context, api);
		success &= context.success;
		LOG_IF(!success, return nullptr,
			Severity::Error, "Failed to initialize Sensor plugin '%s'", sensorPlugin->info.name);
	}

	sensorPlugin->header.isWorking = true;
	return sensorPlugin;
}

static b32
UnloadSensorPlugin(SimulationState* s, SensorPlugin* sensorPlugin)
{
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

	SensorRef sensorRef = {};
	sensorRef.plugin = sensorPlugin->ref;
	for (u32 i = 0; i < sensorPlugin->sensors.length; i++)
	{
		sensorRef.sensor = { i };
		RemoveSensorRefs(s, sensorRef);
	}

	b32 success;
	success = PluginLoader_UnloadSensorPlugin(s->pluginLoader, sensorPlugin);
	List_Free(sensorPlugin->sensors);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Sensor plugin '%s'", sensorPlugin->info.name);

	// TODO: Add and remove plugin infos based on directory contents instead of
	// loaded state.
	*sensorPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

static WidgetPlugin*
LoadWidgetPlugin(SimulationState* s, c8* directory, c8* fileName)
{
	WidgetPlugin* widgetPlugin = List_Append(s->widgetPlugins);
	LOG_IF(!widgetPlugin, return nullptr,
		Severity::Error, "Failed to allocate WidgetPlugin");

	widgetPlugin->ref              = List_GetLast(s->widgetPlugins);
	widgetPlugin->header.fileName  = fileName;
	widgetPlugin->header.directory = directory;
	widgetPlugin->header.kind      = PluginKind::Widget;

	b32 success;
	success = PluginLoader_LoadWidgetPlugin(s->pluginLoader, widgetPlugin);
	LOG_IF(!success, return nullptr,
		Severity::Error, "Failed to load Widget plugin '%s'", fileName);

	// TODO: try/catch?
	if (widgetPlugin->functions.initialize)
	{
		// TODO: Hoist these parameters up
		PluginContext context = {};
		context.s               = s;
		context.widgetPluginRef = widgetPlugin->ref;
		context.success         = true;

		WidgetPluginAPI::Initialize api = {};
		api.AddWidgetDefinitions = AddWidgetDefinitions;
		api.LoadPixelShader      = LoadPixelShader;

		success = widgetPlugin->functions.initialize(&context, api);
		success &= context.success;
		if (!success)
		{
			LOG(Severity::Error, "Failed to initialize Widget plugin");

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
	auto pluginGuard = guard { widgetPlugin->header.isWorking = false; };

	PluginContext context = {};
	context.s               = s;
	context.widgetPluginRef = widgetPlugin->ref;

	WidgetInstanceAPI::Teardown instancesAPI = {};
	for (u32 i = 0; i < widgetPlugin->widgetTypes.length; i++)
	{
		WidgetType* widgetType = &widgetPlugin->widgetTypes[i];
		if (widgetType->widgets.length == 0) continue;

		context.success = true;

		instancesAPI.widgets           = widgetType->widgets;
		instancesAPI.widgetData        = widgetType->widgetData;
		instancesAPI.widgetData.stride = widgetType->definition.size;

		// TODO: try/catch?
		if (widgetType->definition.teardown)
			widgetType->definition.teardown(&context, instancesAPI);
	}

	// TODO: try/catch?
	if (widgetPlugin->functions.teardown)
	{
		WidgetPluginAPI::Teardown api = {};
		widgetPlugin->functions.teardown(&context, api);
	}

	RemoveAllWidgetDefinitions(&context);

	b32 success;
	success = PluginLoader_UnloadWidgetPlugin(s->pluginLoader, widgetPlugin);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to unload Widget plugin '%s'", widgetPlugin->info.name);

	// TODO: Add and remove plugin infos based on directory contents instead
	// of loaded state.
	*widgetPlugin = {};

	pluginGuard.dismiss = true;
	return true;
}

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	b32 success;

	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;
	s->startTime    = Platform_GetTicks();

	success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	success = List_Reserve(s->sensorPlugins, 8);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Sensor plugins list");

	success = List_Reserve(s->widgetPlugins, 8);
	LOG_IF(!success, return false,
		Severity::Error, "Failed to allocate Widget plugins list");

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

			VertexShader vs = Renderer_LoadVertexShader(s->renderer, "Default", "Shaders/Basic Vertex Shader.cso", vsAttributes, cBufDesc);
			LOG_IF(!vs, return false,
				Severity::Error, "Failed to load default vertex shader");
			Assert(vs == StandardVertexShader::Debug);
		}

		// Pixel shader
		{
			PixelShader ps = Renderer_LoadPixelShader(s->renderer, "Default", "Shaders/Basic Pixel Shader.cso", {});
			LOG_IF(!ps, return false,
				Severity::Error, "Failed to load default pixel shader");
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
			// TODO: Pretty sure this should be in CreateWidget
			b32 success = widgetType->definition.initialize(widget);
			Assert(success);
		}
		#else
		//u32 debugSensorIndices[] = { 6, 7, 8, 9, 33 }; // Desktop 780 Tis
		u32 debugSensorIndices[] = { 6, 7, 8, 9, 32 }; // Desktop 2080 Ti
		//u32 debugSensorIndices[] = { 0, 1, 2, 3, 12 }; // Laptop
		WidgetType* widgetType = &filledBarPlugin->widgetTypes[0];
		for (u32 i = 0; i < ArrayLength(debugSensorIndices); i++)
		{
			Widget* widget = CreateWidget(widgetType);
			if (!widget) return false;

			widget->position         = ((v2) s->renderSize - v2{ 240, 12 }) / 2.0f;
			widget->position.y      += ((i32) i - 2) * 15.0f;
			widget->sensorRef.plugin = { 1 };
			widget->sensorRef.sensor = { debugSensorIndices[i] };

			PluginContext context = {};
			context.s               = s;
			context.widgetPluginRef = filledBarPlugin->ref;
			context.success         = true;

			WidgetInstanceAPI::Initialize api = {};
			u32 iLast = widgetType->widgets.length - 1;
			api.widgets    = widgetType->widgets[iLast];
			api.widgetData = widgetType->widgetData[iLast * widgetType->definition.size];

			widgetType->definition.initialize(&context, api);
		}
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
		api.AddSensors    = AddSensors;
		api.RemoveSensors = RemoveSensors;

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
		// TODO: How sensor values propagate to widgets is an open question. Does
		// the application spin through the widgets and update the values (Con:
		// iterating over the list twice. Con: Lots of sensor lookups)? Do we
		// store a map from sensors to widgets that use them and update widgets
		// when the sensor value changes (Con: complexity maybe)? Do we store a
		// pointer in widgets to the sensor value (Con: Have to patch up the
		// pointer when sensors resize) (could be a relative pointer so update
		// happens on a single base pointer) (Con: drawing likely needs access to
		// the full sensor)?

		// HACK TODO: Fuuuuck. Sensors separated by plugin, so refs need to know
		// which plugin they're referring to. Maaaybe a Slice<Slice<Sensor>> will
		// work once we add a stride? Can also make it a 2 part ref: plugin +
		// sensor.

		WidgetPluginAPI::Update pluginAPI = {};

		WidgetInstanceAPI::Update instancesAPI = {};
		instancesAPI.t             = Platform_GetElapsedSeconds(s->startTime);
		instancesAPI.sensors       = s->sensorPlugins[0].sensors;
		instancesAPI.PushDrawCall  = PushDrawCall;
		instancesAPI.GetWVPPointer = GetWVPPointer;

		for (u32 i = 0; i < s->widgetPlugins.length; i++)
		{
			WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];

			context.widgetPluginRef = widgetPlugin->ref;
			context.success         = true;

			// TODO: try/catch?
			if (widgetPlugin->functions.update)
				widgetPlugin->functions.update(&context, pluginAPI);

			for (u32 j = 0; j < widgetPlugin->widgetTypes.length; j++)
			{
				WidgetType* widgetType = &widgetPlugin->widgetTypes[i];
				if (widgetType->widgets.length == 0) continue;

				context.success = true;

				instancesAPI.widgets           = widgetType->widgets;
				instancesAPI.widgetData        = widgetType->widgetData;
				instancesAPI.widgetData.stride = widgetType->definition.size;

				// TODO: try/catch?
				widgetType->definition.update(&context, instancesAPI);
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

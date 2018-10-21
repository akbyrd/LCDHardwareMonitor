struct SimulationState
{
	PluginLoaderState*     pluginLoader;
	RendererState*         renderer;

	v2i                    renderSize = { 320, 240 };
	List<PluginHeader>     pluginHeaders;
	List<SensorPlugin>     sensorPlugins;
	List<WidgetPlugin>     widgetPlugins;
	List<WidgetDefinition> widgetDefinitions;
	//List<Widget>           widgets;
};

struct PluginContext
{
	SimulationState* s;
	PluginHeaderRef  ref;
	b32              success;
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
	for (u32 i = 0; i < s->pluginHeaders.length; i++)
	{
		PluginHeader* pluginHeader = &s->pluginHeaders[i];
		if (pluginHeader->ref == ref)
			return pluginHeader;
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

		success = sensorPlugin->initialize(&context, &api);
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
		sensorPlugin->teardown(nullptr, nullptr);

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

static b32
AddWidgetDefinition(PluginContext* context, WidgetDefinition* widgetDef)
{
	if (!context->success) return false;
	context->success = false;

	// TODO: Use empty slots
	widgetDef->ref = context->ref;
	widgetDef = List_Append(context->s->widgetDefinitions, *widgetDef);
	LOG_IF(!widgetDef, "Failed to allocate space for widget definition", Severity::Warning, return false);

	context->success = true;
	return true;
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
		WidgetPlugin::InitializeAPI api = {};
		api.AddWidgetDefinition = AddWidgetDefinition;

		PluginContext context = {};
		context.s       = s;
		context.ref     = pluginHeader->ref;
		context.success = true;

		success = widgetPlugin->initialize(&context, &api);
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

	// TODO: try/catch?
	if (widgetPlugin->teardown)
		widgetPlugin->teardown(nullptr, nullptr);

	PluginContext context = {};
	context.s       = s;
	context.ref     = pluginHeader->ref;
	context.success = true;
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

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;

	b32 success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	List_Reserve(s->pluginHeaders, 16);
	LOG_IF(!s->pluginHeaders, "Failed to allocate plugin headers list", Severity::Error, return false);

	List_Reserve(s->sensorPlugins, 8);
	LOG_IF(!s->sensorPlugins, "Failed to allocate sensor plugins list", Severity::Error, return false);

	List_Reserve(s->widgetPlugins, 8);
	LOG_IF(!s->widgetPlugins, "Failed to allocate widget plugins list", Severity::Error, return false);

	//List_Reserve(s->widgets, 8);
	//LOG_IF(!s->widgets, "Failed to allocate widgets list", Severity::Error, return false);

	// DEBUG: Testing
	{
		SensorPlugin* ohmPlugin       = LoadSensorPlugin(s, "Sensor Plugins\\OpenHardwareMonitor", "Sensor Plugin - OpenHardwareMonitor");
		WidgetPlugin* filledBarPlugin = LoadWidgetPlugin(s, "Widget Plugins\\Filled Bar",          "Widget Plugin - Filled Bar");

		UNUSED(ohmPlugin);
		UNUSED(filledBarPlugin);

		//Widget* w = List_Append(s->widgets);
		//w->mesh   = Mesh::Quad;
		//w->sensor = List_GetRef(ohmPlugin->sensors, 0);
		//w->size   = { 240, 12 };
	}

	return true;
}

void
Simulation_Teardown(SimulationState* s)
{
	// TODO: Decide how much we really care about simulation level teardown.
	// Remove this once plugin loading and unloading is solidified. It's good to
	// do this for testing, but it's unnecessary work in the normal teardown
	// case.

	//List_Free(s->widgets);

	for (u32 i = 0; i < s->widgetPlugins.length; i++)
		UnloadWidgetPlugin(s, &s->widgetPlugins[i]);
	List_Free(s->widgetPlugins);

	for (u32 i = 0; i < s->sensorPlugins.length; i++)
		UnloadSensorPlugin(s, &s->sensorPlugins[i]);
	List_Free(s->sensorPlugins);

	PluginLoader_Teardown(s->pluginLoader);

	List_Free(s->pluginHeaders);
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
				sensorPlugin->update(&context, &api);
			}
		}
	}

	// Update Widgets
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
				widgetPlugin->update(&context, &api);
			}
		}
	}

	// NOTE: Build a command list of things to draw. Let the renderer handle
	// sorting and iterating the list. Here, we should just be parameters that go
	// with the command.
	//for (u32 i = 0; i < s->widgets.length; i++)
	//	;
}

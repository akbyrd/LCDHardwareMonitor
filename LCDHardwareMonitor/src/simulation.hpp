struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;

	V2i                renderSize = { 320, 240 };
	List<PluginHeader> pluginHeaders;
	List<SensorPlugin> sensorPlugins;
	List<WidgetPlugin> widgetPlugins;
	//List<Widget>       widgets;
};

static PluginHeader*
CreatePluginHeader(SimulationState* s, c8* directory, c8* name, PluginKind kind)
{
	// Reuse any empty spots left by unloading plugins
	PluginHeader* pluginHeader = nullptr;
	for (i32 i = 0; i < s->pluginHeaders.length; i++)
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

	pluginHeader->name      = name;
	pluginHeader->directory = directory;
	pluginHeader->kind      = kind;

	return pluginHeader;
}

static PluginHeader*
GetPluginHeader(SimulationState* s, PluginHeaderRef ref)
{
	for (i32 i = 0; i < s->pluginHeaders.length; i++)
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
		sensorPlugin->initialize(sensorPlugin);

	return sensorPlugin;
}

static b32
UnloadSensorPlugin(SimulationState* s, SensorPlugin* sensorPlugin)
{
	// TODO: Plugin name in errors

	PluginHeader* pluginHeader = GetPluginHeader(s, sensorPlugin->pluginHeaderRef);

	// TODO: try/catch?
	if (sensorPlugin->teardown)
		sensorPlugin->teardown(sensorPlugin);

	b32 success;
	success = PluginLoader_UnloadSensorPlugin(s->pluginLoader, pluginHeader, sensorPlugin);
	// TODO: Widgets will be referencing these sensors.
	List_Free(sensorPlugin->sensors);
	*sensorPlugin = {};
	LOG_IF(!success, "Failed to unload sensor plugin.", Severity::Warning, return false);

	// TODO: Add and remove plugin headers based on directory contents instead of loaded state.
	*pluginHeader = {};

	return true;
}

static WidgetPlugin*
LoadWidgetPlugin(SimulationState* s, c8* directory, c8* name)
{
	PluginHeader* pluginHeader = CreatePluginHeader(s, directory, name, PluginKind::Widget);

	// TODO: Ensure this is removed from the list in partial initialization conditions
	WidgetPlugin* widgetPlugin = List_Append(s->widgetPlugins);
	widgetPlugin->pluginHeaderRef = pluginHeader->ref;

	b32 success;
	success = PluginLoader_LoadWidgetPlugin(s->pluginLoader, pluginHeader, widgetPlugin);
	LOG_IF(!success, "Failed to load widget plugin", Severity::Warning, return nullptr);

	// TODO: try/catch?
	if (widgetPlugin->initialize)
		widgetPlugin->initialize(widgetPlugin);

	return widgetPlugin;
}

static b32
UnloadWidgetPlugin(SimulationState* s, WidgetPlugin* widgetPlugin)
{
	// TODO: Plugin name in errors

	PluginHeader* pluginHeader = GetPluginHeader(s, widgetPlugin->pluginHeaderRef);

	// TODO: try/catch?
	if (widgetPlugin->teardown)
		widgetPlugin->teardown(widgetPlugin);

	b32 success;
	success = PluginLoader_UnloadWidgetPlugin(s->pluginLoader, pluginHeader, widgetPlugin);
	LOG_IF(!success, "Failed to unload widget plugin.", Severity::Warning, return false);

	// TODO: Add and remove plugin headers based on directory contents instead of loaded state.
	*widgetPlugin = {};

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

	for (i32 i = 0; i < s->widgetPlugins.length; i++)
		UnloadWidgetPlugin(s, &s->widgetPlugins[i]);
	List_Free(s->widgetPlugins);

	for (i32 i = 0; i < s->sensorPlugins.length; i++)
		UnloadSensorPlugin(s, &s->sensorPlugins[i]);
	List_Free(s->sensorPlugins);

	PluginLoader_Teardown(s->pluginLoader);

	List_Free(s->pluginHeaders);
}

void
Simulation_Update(SimulationState* s)
{
	// TODO: Only update plugins that are actually being used
	for (i32 i = 0; i < s->sensorPlugins.length; i++)
	{
		SensorPlugin* sensorPlugin = &s->sensorPlugins[i];

		// TODO: try/catch?
		if (sensorPlugin->update)
			sensorPlugin->update(sensorPlugin);
	}

	for (i32 i = 0; i < s->widgetPlugins.length; i++)
	{
		WidgetPlugin* widgetPlugin = &s->widgetPlugins[i];

		// TODO: try/catch?
		if (widgetPlugin->update)
			widgetPlugin->update(widgetPlugin);
	}

	// NOTE: Build a command list of things to draw. Let the renderer handle
	// sorting and iterating the list. Here, we should just be parameters that go
	// with the command.
	//for (i32 i = 0; i < s->widgets.length; i++)
	//	;
}

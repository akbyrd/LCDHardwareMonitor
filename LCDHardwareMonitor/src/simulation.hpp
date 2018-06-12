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
GetPluginHeader(SimulationState* s, PluginHeaderRef ref)
{
	for (i32 i = 0; i < s->pluginHeaders.length; i++)
	{
		PluginHeader* pluginHeader = &s->pluginHeaders[i];
		if (pluginHeader->ref == ref)
			return pluginHeader;
	}

	//Unreachable
	Assert(false);
	return nullptr;
}

static SensorPlugin*
LoadSensorPlugin(SimulationState* s, c16* directory, c16* name)
{
	//TODO: Plugin name in errors

	//Reuse any empty spots left by unloading plugins
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

	//TODO: Ensure this is removed from the list in partial initialization conditions
	SensorPlugin* sensorPlugin = List_Append(s->sensorPlugins);
	sensorPlugin->pluginHeaderRef = pluginHeader->ref;

	b32 success;
	success = PluginLoader_LoadSensorPlugin(s->pluginLoader, pluginHeader, sensorPlugin);
	LOG_IF(!success, L"Failed to load plugin", Severity::Warning, return nullptr);

	List_Reserve(sensorPlugin->sensors, 32);
	LOG_IF(!sensorPlugin->sensors, L"Sensor allocation failed", Severity::Error, return nullptr);

	//TODO: try/catch?
	if (sensorPlugin->initialize)
		sensorPlugin->initialize(sensorPlugin);

	return sensorPlugin;
}

static b32
UnloadSensorPlugin(SimulationState* s, SensorPlugin* sensorPlugin)
{
	//TODO: Plugin name in errors

	PluginHeader* pluginHeader = GetPluginHeader(s, sensorPlugin->pluginHeaderRef);

	//TODO: try/catch?
	if (sensorPlugin->teardown)
		sensorPlugin->teardown(sensorPlugin);

	b32 success;
	success = PluginLoader_UnloadSensorPlugin(s->pluginLoader, pluginHeader, sensorPlugin);
	//TODO: Widgets will be referencing these sensors.
	List_Free(sensorPlugin->sensors);
	*sensorPlugin = {};
	LOG_IF(!success, L"UnloadPlugin failed", Severity::Warning, return false);

	//TODO: Add and remove plugin info based on directory contents instead of loaded state.
	*pluginHeader = {};

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
	LOG_IF(!s->pluginHeaders, L"Failed to allocate plugin headers list", Severity::Error, return false);

	List_Reserve(s->sensorPlugins, 8);
	LOG_IF(!s->sensorPlugins, L"Failed to allocate sensor plugins list", Severity::Error, return false);

	//List_Reserve(s->widgets, 8);
	//LOG_IF(!s->widgets, L"Failed to allocate widgets list", Severity::Error, return false);

	//DEBUG: Testing
	{
		SensorPlugin* ohmPlugin = LoadSensorPlugin(s, L"Sensor Plugins\\OpenHardwareMonitor", L"Sensor Plugin - OpenHardwareMonitor");
		//WidgetPlugin* filledBarPlugin = LoadSensorPlugin(s, L"Widget Plugins\\Filled Bar", L"Filled Bar Plugin");

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
	/* TODO: Remove this once plugin loading and unloading is solidified. It's
	 * good to do this for testing, but it's unnecessary work in the normal
	 * teardown case. */
	for (i32 i = 0; i < s->sensorPlugins.length; i++)
	  UnloadSensorPlugin(s, &s->sensorPlugins[i]);

	//TODO: I don't think this is necessary
	PluginLoader_Teardown(s->pluginLoader);

	//TODO: Decide how much we really care about simulation level teardown
	//List_Free(s->widgets);
	List_Free(s->sensorPlugins);
	List_Free(s->pluginHeaders);
}

void
Simulation_Update(SimulationState* s)
{
	//TODO: Only update plugins that are actually being used
	for (i32 i = 0; i < s->sensorPlugins.length; i++)
	{
		SensorPlugin* sensorPlugin = &s->sensorPlugins[i];
		sensorPlugin->update(sensorPlugin);
	}

	/* NOTE:
	 * Build a command list of things to draw.
	 * Let the renderer handle sorting and iterating the list.
	 * Here, we should just be parameters that go with the command.
	 */
	//for (i32 i = 0; i < s->widgets.length; i++)
	//	;
}

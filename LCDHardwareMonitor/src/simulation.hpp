struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;

	V2i                renderSize = { 320, 240 };
	List<PluginInfo>   pluginInfos;
	List<DataSource>   dataSources;
	//List<Widget>       widgets;
};

static DataSource*
LoadDataSource(SimulationState* s, c16* directory, c16* name)
{
	//TODO: Plugin name in errors

	//Reuse any empty spots left by unloading plugins
	PluginInfo* pluginInfo = nullptr;
	for (i32 i = 0; i < s->pluginInfos.length; i++)
	{
		PluginInfo* pluginSlot = &s->pluginInfos[i];
		if (!pluginSlot->name)
		{
			pluginInfo = pluginSlot;
			break;
		}
	}
	if (!pluginInfo)
		pluginInfo = List_Append(s->pluginInfos);

	pluginInfo->name      = name;
	pluginInfo->directory = directory;

	DataSource dataSource = {};
	dataSource.pluginInfo = pluginInfo;

	b32 success;
	success = PluginLoader_LoadDataSource(s->pluginLoader, &dataSource);
	LOG_IF(!success, L"Failed to load plugin", Severity::Warning, return nullptr);

	//List_Reserve(dataSource.sensors, 32);
	//LOG_IF(!dataSource.sensors, L"Sensor allocation failed", Severity::Error, return nullptr);

	//TODO: try/catch?
	if (dataSource.initialize)
		dataSource.initialize();

	return List_Append(s->dataSources, dataSource);
}

static b32
UnloadDataSource(SimulationState* s, DataSource* dataSource)
{
	//TODO: Plugin name in errors

	PluginInfo* pluginInfo = dataSource->pluginInfo;

	//TODO: try/catch?
	if (dataSource->teardown)
		dataSource->teardown();

	b32 success;
	success = PluginLoader_UnloadDataSource(s->pluginLoader, dataSource);
	//TODO: Widgets will be referencing these sensors.
	//List_Free(dataSource->sensors);
	*dataSource = {};
	LOG_IF(!success, L"UnloadPlugin failed", Severity::Warning, return false);

	//TODO: Add and remove plugin info based on directory contents instead of loaded state.
	*pluginInfo = {};

	return true;
}

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;

	b32 success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	s->pluginInfos = {};
	List_Reserve(s->pluginInfos, 16);
	LOG_IF(!s->pluginInfos, L"PluginInfo allocation failed", Severity::Error, return false);

	s->dataSources = {};
	List_Reserve(s->dataSources, 8);
	LOG_IF(!s->dataSources, L"Failed to allocate data sources list", Severity::Error, return false);

	//s->widgets = {};
	//List_Reserve(s->widgets, 8);
	//LOG_IF(!s->dataSources, L"Failed to allocate widgets list", Severity::Error, return false);

	/* TODO: Exceptions are being raised due to OpenHardwareLib and
	 * OpenHardware Plugin failing to load. It looks like they fail and then we
	 * use AssemblyResolve to find them using the full path.
	 */
	DataSource* ohmDataSource = LoadDataSource(s, L"Data Sources\\OpenHardwareMonitor Source", L"OpenHardwareMonitor Plugin");

	//Widget& w = *List_Append(s->widgets);
	//w.mesh   = Mesh::Quad;
	//w.sensor = &s->dataSources[0].sensors[0];
	//w.size   = {240, 12};

	return true;
}

void
Simulation_Teardown(SimulationState* s)
{
	/* TODO: Remove this once plugin loading and unloading is solidified. It's
	 * good to do this for testing, but it's unnecessary work in the normal
	 * teardown case. */
	for (i32 i = 0; i < s->dataSources.length; i++)
	  UnloadDataSource(s, &s->dataSources[i]);

	//TODO: I don't think this is necessary
	PluginLoader_Teardown(s->pluginLoader);

	//TODO: Decide how much we really care about simulation level teardown
	//List_Free(s->widgets);
	List_Free(s->dataSources);
	List_Free(s->pluginInfos);
}

void
Simulation_Update(SimulationState* s)
{
	//TODO: Only update data sources that are actually being used
	for (i32 i = 0; i < s->dataSources.length; i++)
		;// s->dataSources[i].update(s->dataSources[i].sensors);

	/* NOTES:
	 * Build a command list of things to draw.
	 * Let the renderer handle sorting and iterating the list.
	 * Here, we should just be parameters that go with the command.
	 */
	//for (i32 i = 0; i < s->widgets.length; i++)
	//	;
}

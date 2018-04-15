struct DataSource
{
	//TODO: This pointer will break when the list resizes
	Plugin*              plugin;
	c16*                 name;
	List<Sensor>         sensors;
	DataSourceInitialize initialize;
	DataSourceUpdate     update;
	DataSourceTeardown   teardown;
};

struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;

	V2i                renderSize = { 320, 240 };
	List<DataSource>   dataSources;
	List<Widget>       widgets;
};

static DataSource*
LoadDataSource(SimulationState* s, c16* directory, c16* name)
{
	DataSource dataSource = {};
	dataSource.name = name;

	dataSource.plugin = PluginLoader_LoadPlugin(s->pluginLoader, directory, name);
	LOG_IF(!dataSource.plugin, L"Failed to load plugin", Severity::Warning, return nullptr);

	dataSource.initialize = (DataSourceInitialize) PluginLoader_GetPluginSymbol(s->pluginLoader, dataSource.plugin, "Initialize");
	dataSource.update     = (DataSourceUpdate)     PluginLoader_GetPluginSymbol(s->pluginLoader, dataSource.plugin, "Update");
	dataSource.teardown   = (DataSourceTeardown)   PluginLoader_GetPluginSymbol(s->pluginLoader, dataSource.plugin, "Teardown");

	dataSource.sensors = {};
	List_Reserve(dataSource.sensors, 128);
	LOG_IF(!dataSource.sensors, L"Sensor allocation failed", Severity::Error, return nullptr);

	//TODO: try/catch?
	if (dataSource.initialize)
		dataSource.initialize(dataSource.sensors);

	return List_Append(s->dataSources, dataSource);
}

static b32
UnloadDataSource(SimulationState* s, DataSource* dataSource)
{
	//TODO: try/catch?
	if (dataSource->teardown)
		dataSource->teardown(dataSource->sensors);

	//If UnloadPlugin fails, don't keep using the plugin.
	dataSource->initialize = nullptr;
	dataSource->update     = nullptr;
	dataSource->teardown   = nullptr;

	//TODO: Widgets will be referencing these sensors.
	List_Free(dataSource->sensors);

	b32 success = PluginLoader_UnloadPlugin(s->pluginLoader, dataSource->plugin);
	LOG_IF(!success, L"UnloadPlugin failed", Severity::Warning, return false);

	return true;
}

b32
Simulation_Initialize(SimulationState* s, PluginLoaderState* pluginLoader, RendererState* renderer)
{
	s->pluginLoader = pluginLoader;
	s->renderer     = renderer;

	b32 success = PluginLoader_Initialize(s->pluginLoader);
	if (!success) return false;

	s->dataSources = {};
	List_Reserve(s->dataSources, 8);
	LOG_IF(!s->dataSources, L"Failed to allocate data sources list", Severity::Error, return false);

	s->widgets = {};
	List_Reserve(s->widgets, 8);
	LOG_IF(!s->dataSources, L"Failed to allocate widgets list", Severity::Error, return false);

	/* TODO: Exceptions are being raised due to OpenHardwareLib and
	 * OpenHardware Plugin failing to load. It looks like they fail and then we
	 * use AssemblyResolve to find them using the full path.
	 */
	DataSource* ohmDataSource = LoadDataSource(s, L"Data Sources\\OpenHardwareMonitor Source", L"OpenHardwareMonitor Plugin");

	Widget& w = *List_Append(s->widgets);
	w.mesh   = Mesh::Quad;
	w.sensor = &s->dataSources[0].sensors[0];
	w.size   = {240, 12};

	return true;
}

void
Simulation_Teardown(SimulationState* s)
{
	for (i32 i = 0; i < s->dataSources.length; i++)
	  UnloadDataSource(s, &s->dataSources[i]);

	//TODO: Decide how much we really care about simulation level teardown
	List_Free(s->widgets);
	List_Free(s->dataSources);

	//TODO: I don't think this is necessary
	PluginLoader_Teardown(s->pluginLoader);
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
	for (i32 i = 0; i < s->widgets.length; i++)
		;
}

enum struct Severity
{
	Info,
	Warning,
	Error
};

#define WIDEHELPER(x) L##x
#define WIDE(x) WIDEHELPER(x)

#define LOG(message, severity) Log(message, severity, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
#define LOG_IF(expression, message, severity, ...) IF(expression, LOG(message, severity); __VA_ARGS__)

void
Log(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	//TODO: Decide on logging allocation policy
	//TODO: Check for swprintf failure (overflow).
	c16 buffer[512];
	swprintf(buffer, ArrayLength(buffer), L"%s - %s\n\t%s(%i)\n", function, message, file, line);
	ConsolePrint(buffer);

	if (severity > Severity::Info)
		__debugbreak();
}

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
	PlatformState*   platform;
	RendererState*   renderer;

	V2i              renderSize = {320, 240};
	List<DataSource> dataSources;
	List<Widget>     widgets;
};

DataSource*
LoadDataSource(SimulationState* s, c16* directory, c16* name)
{
	DataSource dataSource = {};
	dataSource.name = name;

	dataSource.plugin = LoadPlugin(s->platform, directory, name);
	LOG_IF(!dataSource.plugin, L"Failed to load plugin", Severity::Warning, return {});

	dataSource.initialize = (DataSourceInitialize) GetPluginSymbol(dataSource.plugin, "Initialize");
	dataSource.update     = (DataSourceUpdate)     GetPluginSymbol(dataSource.plugin, "Update");
	dataSource.teardown   = (DataSourceTeardown)   GetPluginSymbol(dataSource.plugin, "Teardown");

	dataSource.sensors = {};
	List_Reserve(dataSource.sensors, 128);
	LOG_IF(!dataSource.sensors, L"Sensor allocation failed", Severity::Error, return nullptr);

	//TODO: try/catch?
	if (dataSource.initialize)
		dataSource.initialize(dataSource.sensors);

	return List_Append(s->dataSources, dataSource);
}

b32
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

	b32 success = UnloadPlugin(s->platform, dataSource->plugin);
	LOG_IF(!success, L"UnloadPlugin failed", Severity::Warning, return false);

	return true;
}

void
Simulation_Initialize(SimulationState* s)
{
	s->dataSources = {};
	List_Reserve(s->dataSources, 8);
	LOG_IF(!s->dataSources, L"Failed to allocate data sources list", Severity::Error, return false);

	s->widgets = {};
	List_Reserve(s->widgets, 8);
	LOG_IF(!s->dataSources, L"Failed to allocate widgets list", Severity::Error, return false);

	DataSource* ohmDataSource = LoadDataSource(s, L"Data Sources\\OpenHardwareMonitor Source", L"OpenHardwareMonitor Plugin");

	Widget& w = *List_Append(s->widgets);
	w.mesh   = Mesh::Quad;
	w.sensor = &s->dataSources[0].sensors[0];
	w.size   = {240, 12};
}

void
Simulation_Teardown(SimulationState* s)
{
	for (i32 i = 0; i < s->dataSources.length; i++)
	  UnloadDataSource(s, &s->dataSources[i]);

	//TODO: Decide how much we really care about simulation level teardown
	List_Free(s->widgets);
	List_Free(s->dataSources);
}

void
Simulation_Update(SimulationState* s)
{
	//TODO: Only update data sources that are actually being used
	for (i32 i = 0; i < s->dataSources.length; i++)
		s->dataSources[i].update(s->dataSources[i].sensors);

	/* NOTES:
	 * Build a command list of things to draw.
	 * Let the renderer handle sorting and iterating the list.
	 * Here, we should just be parameters that go with the command.
	 */
	for (i32 i = 0; i < s->widgets.length; i++)
		;
}
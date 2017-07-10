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
//#define LOGI_IF(expression, message, ...) LOG_IF(expression, message, Severity::Info,    ##__VA_ARGS__)
//#define LOGW_IF(expression, message, ...) LOG_IF(expression, message, Severity::Warning, ##__VA_ARGS__)
//#define LOGE_IF(expression, message, ...) LOG_IF(expression, message, Severity::Error,   ##__VA_ARGS__)
void
Log(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	//TODO: Decide on logging allocation policy
	//TODO: Check for swprintf failure (overflow).
	c16 buffer[512];
	swprintf(buffer, ArrayCount(buffer), L"%s - %s\n\t%s(%i)\n", function, message, file, line);
	ConsolePrint(buffer);

	if (severity > Severity::Info)
		__debugbreak();
}

struct Widget
{
	//TODO: This pointer will break when the list resizes
	//Eventually we'll want to store something more indirect than a pointer.
	//Maybe a plugin id and sensor id pair?  Maybe ids are never reused or invalid
	//This will tie closely into how we save layouts between sessions
	Sensor* sensor;
	Mesh    mesh;
	V2i     size;
	float   minValue;
	float   maxValue;
	//c16*    displayFormat;
};

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
	V2i              renderSize = {320, 240};
	List<DataSource> dataSources;
};

DataSource*
LoadDataSource(SimulationState* s, PlatformState* platformState, c16* directory, c16* name)
{
	DataSource dataSource = {};
	dataSource.name = name;

	dataSource.plugin = LoadPlugin(platformState, directory, name);
	LOG_IF(!dataSource.plugin, L"Failed to load plugin", Severity::Warning, return {});

	dataSource.initialize = (DataSourceInitialize) GetPluginSymbol(dataSource.plugin, "Initialize");
	dataSource.update     = (DataSourceUpdate)     GetPluginSymbol(dataSource.plugin, "Update");
	dataSource.teardown   = (DataSourceTeardown)   GetPluginSymbol(dataSource.plugin, "Teardown");

	dataSource.sensors = List_Create<Sensor>(128);
	LOG_IF(!dataSource.sensors.items, L"Sensor allocation failed", Severity::Error, return {});

	//TODO: try/catch?
	if (dataSource.initialize)
		dataSource.initialize(dataSource.sensors);

	return List_Append(s->dataSources, dataSource);
}

b32
UnloadDataSource(SimulationState* s, PlatformState* platformState, DataSource* dataSource)
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

	b32 success = UnloadPlugin(platformState, dataSource->plugin);
	LOG_IF(!success, L"UnloadPlugin failed", Severity::Warning, return false);

	return true;
}

void
Simulation_Initialize(SimulationState* s)
{
	s->dataSources = List_Create<DataSource>(8);
}

void
Simulation_Teardown(SimulationState* s)
{
	List_Free(s->dataSources);
}

//TODO: Do something about D3DRenderState being known here.
void
Simulation_Update(SimulationState* s, RendererState* rendererState)
{
	for (i32 i = 0; i < s->dataSources.count; i++)
		s->dataSources[i].update(s->dataSources[i].sensors);

	Widget w = {};
	w.sensor   = &s->dataSources[0].sensors[0];
	w.mesh     = Mesh::Quad;
	w.minValue = 0;
	w.maxValue = 100;

	//DrawCall* dc = PushDrawCall(rendererState);
	//dc->mesh = w.mesh;
	//float sizeX = w.size.x * (w.sensor->value - w.minValue) / (w.maxValue - w.minValue);
	//XMStoreFloat4x4((XMFLOAT4X4*) &dc->worldM, XMMatrixScaling(sizeX, w.size.y, 1) * XMMatrixIdentity());
}
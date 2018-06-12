//TODO: Widgets should be loaded as plugins!
//#include "widget_filledbar.hpp"

struct SimulationState
{
	PluginLoaderState* pluginLoader;
	RendererState*     renderer;

	V2i                renderSize = { 320, 240 };
	List<PluginHeader> pluginHeaders;
	List<DataSource>   dataSources;
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

static DataSource*
LoadDataSource(SimulationState* s, c16* directory, c16* name)
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
	DataSource* dataSource = List_Append(s->dataSources);
	dataSource->pluginHeaderRef = pluginHeader->ref;

	b32 success;
	success = PluginLoader_LoadDataSource(s->pluginLoader, pluginHeader, dataSource);
	LOG_IF(!success, L"Failed to load plugin", Severity::Warning, return nullptr);

	List_Reserve(dataSource->sensors, 32);
	LOG_IF(!dataSource->sensors, L"Sensor allocation failed", Severity::Error, return nullptr);

	//TODO: try/catch?
	if (dataSource->initialize)
		dataSource->initialize(dataSource);

	return dataSource;
}

static b32
UnloadDataSource(SimulationState* s, DataSource* dataSource)
{
	//TODO: Plugin name in errors

	PluginHeader* pluginHeader = GetPluginHeader(s, dataSource->pluginHeaderRef);

	//TODO: try/catch?
	if (dataSource->teardown)
		dataSource->teardown(dataSource);

	b32 success;
	success = PluginLoader_UnloadDataSource(s->pluginLoader, pluginHeader, dataSource);
	//TODO: Widgets will be referencing these sensors.
	List_Free(dataSource->sensors);
	*dataSource = {};
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
	LOG_IF(!s->pluginHeaders, L"PluginHeader allocation failed", Severity::Error, return false);

	List_Reserve(s->dataSources, 8);
	LOG_IF(!s->dataSources, L"Failed to allocate data sources list", Severity::Error, return false);

	//List_Reserve(s->widgets, 8);
	//LOG_IF(!s->widgets, L"Failed to allocate widgets list", Severity::Error, return false);

	//DEBUG: Testing
	{
		DataSource* ohmDataSource = LoadDataSource(s, L"Data Sources\\OpenHardwareMonitor Source", L"OpenHardwareMonitor Plugin");

		//Widget* w = List_Append(s->widgets);
		//w->mesh   = Mesh::Quad;
		//w->sensor = List_GetRef(ohmDataSource->sensors, 0);
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
	for (i32 i = 0; i < s->dataSources.length; i++)
	  UnloadDataSource(s, &s->dataSources[i]);

	//TODO: I don't think this is necessary
	PluginLoader_Teardown(s->pluginLoader);

	//TODO: Decide how much we really care about simulation level teardown
	//List_Free(s->widgets);
	List_Free(s->dataSources);
	List_Free(s->pluginHeaders);
}

void
Simulation_Update(SimulationState* s)
{
	//TODO: Only update data sources that are actually being used
	for (i32 i = 0; i < s->dataSources.length; i++)
	{
		DataSource* dataSource = &s->dataSources[i];
		dataSource->update(dataSource);
	}

	/* NOTE:
	 * Build a command list of things to draw.
	 * Let the renderer handle sorting and iterating the list.
	 * Here, we should just be parameters that go with the command.
	 */
	//for (i32 i = 0; i < s->widgets.length; i++)
	//	;
}

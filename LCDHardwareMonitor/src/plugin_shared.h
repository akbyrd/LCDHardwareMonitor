enum struct PluginKind
{
	Null,
	Sensor,
	Widget,
};

enum struct PluginLanguage
{
	Null,
	Native,
	Managed,
};

struct PluginHeader
{
	b32            isLoaded;
	b32            isWorking;
	c8*            fileName;
	c8*            directory;
	PluginKind     kind;
	PluginLanguage language;
	void*          userData;
};

struct SensorPlugin;
using SensorPluginRef = List<SensorPlugin>::RefT;

struct SensorPlugin
{
	SensorPluginRef       ref;
	PluginHeader          header;
	PluginInfo            info;
	SensorPluginFunctions functions;

	List<Sensor>  sensors;
	//List<SensorRef> activeSensors;
};

struct WidgetInstances
{
	WidgetDefinition definition;
	Bytes            instances;
};

struct WidgetPlugin;
using WidgetPluginRef = List<WidgetPlugin>::RefT;

struct WidgetPlugin
{
	WidgetPluginRef       ref;
	PluginHeader          header;
	PluginInfo            info;
	WidgetPluginFunctions functions;

	// TODO: Rename this (WidgetTypes?)
	List<WidgetInstances> widgetInstances;
};

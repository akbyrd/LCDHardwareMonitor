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

struct PluginHeader;
using PluginHeaderRef = List<PluginHeader>::RefT;

// TODO: Maybe union SensorPlugin and WidgetPlugin into here?
// TODO: Right now I don't think it's possible to get from a PluginHeader to
// the actual Sensor/WidgetPlugin
struct PluginHeader
{
	PluginHeaderRef ref;
	b32             isLoaded;
	b32             isWorking;
	c8*             fileName;
	c8*             directory;
	PluginKind      kind;
	PluginLanguage  language;
	void*           userData;
};

struct WidgetInstances
{
	WidgetDefinition definition;
	Bytes            instances;
};

struct WidgetPlugin
{
	PluginHeaderRef       pluginHeaderRef;
	PluginInfo            pluginInfo;
	WidgetPluginFunctions functions;

	// TODO: Rename this (WidgetTypes?)
	List<WidgetInstances> widgetInstances;
};

struct SensorPlugin
{
	PluginHeaderRef       pluginHeaderRef;
	PluginInfo            pluginInfo;
	SensorPluginFunctions functions;

	List<Sensor>  sensors;
	//List<SensorRef> activeSensors;
};

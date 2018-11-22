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

	List<Sensor>          sensors;
	//List<SensorRefT> activeSensors;
};

struct WidgetType
{
	WidgetDefinition definition;
	List<Widget>     widgets;
	Bytes            widgetData;
};

struct WidgetPlugin;
using WidgetPluginRef = List<WidgetPlugin>::RefT;

struct WidgetPlugin
{
	WidgetPluginRef       ref;
	PluginHeader          header;
	PluginInfo            info;
	WidgetPluginFunctions functions;

	List<WidgetType>      widgetTypes;
};

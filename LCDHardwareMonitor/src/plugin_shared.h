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

struct SensorPlugin
{
	SensorPluginRef       ref;
	PluginHeader          header;
	PluginInfo            info;
	SensorPluginFunctions functions;

	List<Sensor>          sensors;
	//List<SensorRef> activeSensors;
};

struct WidgetData
{
	WidgetDesc   desc;
	List<Widget> widgets;
	Bytes        widgetsUserData;
};

struct WidgetPlugin
{
	WidgetPluginRef       ref;
	PluginHeader          header;
	PluginInfo            info;
	WidgetPluginFunctions functions;

	List<WidgetData>      widgetDatas;
};

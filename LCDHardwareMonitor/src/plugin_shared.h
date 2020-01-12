namespace LCDHardwareMonitor {

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

enum struct PluginLoadState
{
	Null,
	Loaded,
	Unloaded,
	Broken,
};

struct PluginHeader
{
	PluginLoadState loadState;
	String          fileName;
	String          directory;
	PluginKind      kind;
	PluginLanguage  language;
	void*           userData;
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

struct WidgetData;
using WidgetDataRef = List<WidgetData>::RefT;

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

}

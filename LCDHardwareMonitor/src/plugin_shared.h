template <typename T>
struct Handle
{
	u32 value = 0;
};

struct PluginInfo
{
	String name;
	String author;
	u32    version;
	u32    lhmVersion;
};

enum struct PluginKind
{
	Null,
	Sensor,
	Widget,
};
b8 operator!(PluginKind kind) { return kind == PluginKind::Null; }

enum struct PluginLanguage
{
	Null,
	Builtin,
	Native,
	Managed,
};

enum struct PluginLoadState
{
	Null,
	Loading,
	Loaded,
	Unloading,
	Unloaded,
	Broken,
};

struct Plugin
{
	Handle<Plugin>  handle;
	u32             rawRefToKind;
	PluginInfo      info;
	PluginKind      kind;
	PluginLanguage  language;
	PluginLoadState loadState;
	String          fileName;
	String          directory;
	void*           loaderData;
};

struct SensorPlugin
{
	SensorPluginRef       ref;
	Handle<Plugin>        pluginHandle;
	StringSlice           name;
	SensorPluginFunctions functions;
	List<Sensor>          sensors;
	//List<SensorRef> activeSensors;
};

struct FullSensorRef
{
	SensorPluginRef pluginRef;
	SensorRef       sensorRef;
};

struct WidgetData;
using WidgetDataRef = List<WidgetData>::RefT;

struct WidgetData
{
	WidgetDataRef ref;
	WidgetDesc    desc;
	List<Widget>  widgets;
	Bytes         widgetsUserData;
};

struct WidgetPlugin
{
	WidgetPluginRef       ref;
	Handle<Plugin>        pluginHandle;
	StringSlice           name;
	WidgetPluginFunctions functions;
	List<WidgetData>      widgetDatas;
};

struct FullWidgetDataRef
{
	WidgetPluginRef pluginRef;
	WidgetDataRef   dataRef;
};

struct FullWidgetRef
{
	WidgetPluginRef pluginRef;
	WidgetDataRef   dataRef;
	WidgetRef       widgetRef;
};

inline b8 operator== (FullWidgetRef lhs, FullWidgetRef rhs)
{
	b8 result = true;
	result &= lhs.pluginRef == rhs.pluginRef;
	result &= lhs.dataRef == rhs.dataRef;
	result &= lhs.widgetRef == rhs.widgetRef;
	return result;
}
inline b8 operator!= (FullWidgetRef lhs, FullWidgetRef rhs) { return !(lhs == rhs); }

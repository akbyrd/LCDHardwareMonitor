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
	Handle<SensorPlugin>  handle;
	Handle<Plugin>        pluginHandle;
	StringSlice           name;
	SensorPluginFunctions functions;
	List<Sensor>          sensors;
	//List<Handle<Sensor>> activeSensors;
};

struct WidgetPlugin;

struct WidgetType
{
	using InitializeFn = b8  (PluginContext&, WidgetAPI::Initialize);
	using UpdateFn     = void(PluginContext&, WidgetAPI::Update);
	using TeardownFn   = void(PluginContext&, WidgetAPI::Teardown);

	Handle<WidgetType>   handle;
	Handle<WidgetPlugin> widgetPluginHandle;
	String               name;
	u32                  userDataSize;
	List<Widget>         widgets;
	Bytes                widgetsUserData;
	InitializeFn*        Initialize;
	UpdateFn*            Update;
	TeardownFn*          Teardown;
};

struct WidgetPlugin
{
	Handle<WidgetPlugin>  handle;
	Handle<Plugin>        pluginHandle;
	StringSlice           name;
	WidgetPluginFunctions functions;
	List<WidgetType>      widgetTypes;
};

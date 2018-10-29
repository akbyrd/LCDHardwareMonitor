// TODO: Rename this file

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

using PluginHeaderRef = List<struct PluginHeader>::RefT;

// TODO: Maybe union SensorPlugin and WidgetPlugin into here?
// TODO: Right now I don't think it's possible to get from a PluginHeader to
// the actual Sensor/WidgetPlugin
// TODO: Rename to PluginInfo
struct PluginHeader
{
	PluginHeaderRef ref;
	b32             isLoaded;
	b32             isWorking;
	c8*             name;
	c8*             directory;
	PluginKind      kind;
	PluginLanguage  language;
	void*           userData;
};

struct WidgetPlugin
{
	using InitializeFn = b32 (PluginContext* context, WidgetPluginAPI::Initialize api);
	using UpdateFn     = void(PluginContext* context, WidgetPluginAPI::Update     api);
	using TeardownFn   = void(PluginContext* context, WidgetPluginAPI::Teardown   api);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;
};

struct SensorPlugin
{
	using InitializeFn = b32 (PluginContext* context, SensorPluginAPI::Initialize api);
	using UpdateFn     = void(PluginContext* context, SensorPluginAPI::Update     api);
	using TeardownFn   = void(PluginContext* context, SensorPluginAPI::Teardown   api);

	PluginHeaderRef pluginHeaderRef;
	InitializeFn*   initialize;
	UpdateFn*       update;
	TeardownFn*     teardown;

	List<Sensor>    sensors;
	//List<SensorRef> activeSensors;
};

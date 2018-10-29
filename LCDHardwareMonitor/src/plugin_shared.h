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

struct PluginInfo;
using PluginInfoRef = List<PluginInfo>::RefT;

// TODO: Maybe union SensorPlugin and WidgetPlugin into here?
// TODO: Right now I don't think it's possible to get from a PluginInfo to
// the actual Sensor/WidgetPlugin
// TODO: Rename to PluginInfo
struct PluginInfo
{
	PluginInfoRef  ref;
	b32            isLoaded;
	b32            isWorking;
	c8*            name;
	c8*            directory;
	PluginKind     kind;
	PluginLanguage language;
	void*          userData;
};

struct WidgetPlugin
{
	using InitializeFn = b32 (PluginContext* context, WidgetPluginAPI::Initialize api);
	using UpdateFn     = void(PluginContext* context, WidgetPluginAPI::Update     api);
	using TeardownFn   = void(PluginContext* context, WidgetPluginAPI::Teardown   api);

	PluginInfoRef pluginInfoRef;
	InitializeFn* initialize;
	UpdateFn*     update;
	TeardownFn*   teardown;
};

struct SensorPlugin
{
	using InitializeFn = b32 (PluginContext* context, SensorPluginAPI::Initialize api);
	using UpdateFn     = void(PluginContext* context, SensorPluginAPI::Update     api);
	using TeardownFn   = void(PluginContext* context, SensorPluginAPI::Teardown   api);

	PluginInfoRef pluginInfoRef;
	InitializeFn* initialize;
	UpdateFn*     update;
	TeardownFn*   teardown;

	List<Sensor>  sensors;
	//List<SensorRef> activeSensors;
};

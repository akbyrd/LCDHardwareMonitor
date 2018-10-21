#ifndef LHM_PLUGINHEADER
#define LHM_PLUGINHEADER

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
struct PluginHeader
{
	PluginHeaderRef ref;
	b32             isLoaded;
	b32             isWorking;
	c8*             name;
	c8*             directory;
	PluginKind      kind;
	PluginLanguage  language;
	PluginInfo      info;
	void*           userData;
};

#endif

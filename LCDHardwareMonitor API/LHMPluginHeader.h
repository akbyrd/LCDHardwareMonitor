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
struct PluginHeader
{
	PluginHeaderRef ref;
	b32             isLoaded;
	c8*             name;
	c8*             directory;
	PluginKind      kind;
	PluginLanguage  language;
	PluginInfo      info;
	void*           userData;
};

#endif

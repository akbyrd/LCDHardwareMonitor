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
struct PluginHeader
{
	PluginHeaderRef ref;
	b32             isLoaded;
	c16*            name;
	c16*            directory;
	PluginKind      kind;
	PluginLanguage  language;
	PluginInfo      info;

	// TODO: Try using COM pointers instead.
	void* appDomain;
	void* pluginLoader;
};

#endif

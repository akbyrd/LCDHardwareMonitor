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
	c8*             name;
	c8*             directory;
	PluginKind      kind;
	PluginLanguage  language;
	PluginInfo      info;

	union
	{
		struct
		{
			void* module;
		} native;

		// TODO: Try using COM pointers instead.
		struct
		{
			void* appDomain;
			void* pluginLoader;
		} managed;
	};
};

#endif

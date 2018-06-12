#ifndef LHMPLUGINHEADER
#define LHMPLUGINHEADER

enum struct PluginKind
{
	Null,
	Native,
	Managed,
};

typedef List<struct PluginHeader>::RefT PluginHeaderRef;
struct PluginHeader
{
	PluginHeaderRef ref;
	b32             isLoaded;
	c16*            name;
	c16*            directory;
	PluginKind      kind;
	PluginInfo      info;

	//TODO: Try using COM pointers instead.
	void* appDomain;
	void* pluginLoader;
};

#endif

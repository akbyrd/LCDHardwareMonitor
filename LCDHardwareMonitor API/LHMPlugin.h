#ifndef LHM_Plugin
#define LHM_Plugin

struct PluginInfo
{
	String name;
	String author;
	u32    version;

	// TODO: Use to verify plugin and application versions match
	//u32 lhmVersion;
};

struct PluginContext;

#endif

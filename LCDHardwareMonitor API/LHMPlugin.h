#ifndef LHM_Plugin
#define LHM_Plugin

struct PluginInfo
{
	StringSlice name;
	StringSlice author;
	u32         version;

	// TODO: Use to verify plugin and application versions match
	//u32 lhmVersion;
};

struct PluginContext;

#endif

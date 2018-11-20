#ifndef LHM_Plugin
#define LHM_Plugin

struct PluginInfo
{
	c8* name;
	c8* author;
	u32 version;

	// TODO: Use to verify plugin and application versions match
	//u32 lhmVersion;
};

struct PluginContext;

#endif

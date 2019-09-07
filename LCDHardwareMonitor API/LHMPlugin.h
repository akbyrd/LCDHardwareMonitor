#ifndef LHM_Plugin
#define LHM_Plugin

// TODO: This is only separate because it's sent to plugins to fill in. Consider flattening it.
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

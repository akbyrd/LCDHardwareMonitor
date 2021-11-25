#ifndef LHM_Plugin
#define LHM_Plugin

struct PluginDesc
{
	StringView name;
	StringView author;
	u32        version;
	u32        lhmVersion;
};

struct PluginContext;

#endif

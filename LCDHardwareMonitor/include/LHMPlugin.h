#ifndef LHM_Plugin
#define LHM_Plugin

template <typename T>
struct Handle
{
	u32 value = 0;
};

struct PluginDesc
{
	StringView name;
	StringView author;
	u32        version;
	u32        lhmVersion;
};

struct PluginContext;

#endif

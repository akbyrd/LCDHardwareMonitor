#pragma once
#pragma unmanaged

enum struct PluginKind
{
	Null,
	Native,
	Managed,
};

struct PluginInfo
{
	b32        isLoaded;
	PluginKind kind;
	c16*       directory;
	c16*       name;
};

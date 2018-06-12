#pragma once

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

	void* appDomain;
	void* pluginLoader;
	//ComPtr<_AppDomain> appDomain;
	//ComPtr<ILHMPluginLoader> pluginLoader;
};

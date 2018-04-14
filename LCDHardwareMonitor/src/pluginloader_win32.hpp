struct Plugin
{
	HMODULE              module;
	DLL_DIRECTORY_COOKIE cookie;
	c16*                 directory;
	c16*                 name;
};

struct PluginLoaderState
{
	List<Plugin> plugins;
};

b32
PluginLoader_Initialize(PluginLoaderState* s)
{
	CLR_Initialize();

	b32 success = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	LOG_LAST_ERROR_IF(!success, L"SetDefaultDllDirectories failed", Severity::Warning, return false);

	s->plugins = {};
	List_Reserve(s->plugins, 16);
	LOG_IF(!s->plugins, L"Plugin allocation failed", Severity::Error, return false);

	return true;
}

void
PluginLoader_Teardown(PluginLoaderState* s)
{
	List_Free(s->plugins);

	CLR_Teardown();
}

Plugin*
PluginLoader_LoadPlugin(PluginLoaderState* s, c16* directory, c16* name)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	Plugin plugin = {};
	plugin.directory = directory;
	plugin.name = name;

	c16 path[MAX_PATH];
	i32 cwdLen = GetCurrentDirectoryW(ArrayLength(path), path);
	LOG_LAST_ERROR_IF(cwdLen == 0, L"GetCurrentDirectory failed", Severity::Warning, return nullptr);
	path[cwdLen++] = '\\';
	errno_t err = wcscpy_s(path + cwdLen, ArrayLength(path) - cwdLen, directory);
	LOG_IF(err, L"wcscpy_s failed", Severity::Warning, return {});

	//Add the plugin directory to the DLL search path so its dependencies will be found.
	plugin.cookie = AddDllDirectory(path);
	LOG_LAST_ERROR_IF(!plugin.cookie, L"AddDllDirectory failed", Severity::Warning, return nullptr);

	plugin.module = LoadLibraryW(name);
	CLR_PluginLoaded(directory, name);

	//Reuse any empty spots left by unloading plugins
	for (i32 i = 0; i < s->plugins.length; i++)
	{
		Plugin* pluginSlot = &s->plugins[i];
		if (!pluginSlot->module)
		{
			*pluginSlot = plugin;
			return pluginSlot;
		}
	}
	return List_Append(s->plugins, plugin);
}

b32
PluginLoader_UnloadPlugin(PluginLoaderState* s, Plugin* plugin)
{
	b32 loaded = List_Contains(s->plugins, plugin);
	LOG_IF(!loaded, L"Attempting to unload a plugin that is not loaded", Severity::Warning, return false);

	b32 success = FreeLibrary(plugin->module);
	LOG_LAST_ERROR_IF(!success, L"FreeLibrary failed", Severity::Warning, return false);

	CLR_PluginUnloaded(plugin->directory, plugin->name);

	success = RemoveDllDirectory(plugin->cookie);
	LOG_LAST_ERROR_IF(!success, L"RemoveDllDirectory failed", Severity::Warning);

	*plugin = {};
	return true;
}

void*
PluginLoader_GetPluginSymbol(PluginLoaderState* s, Plugin* plugin, c8* symbol)
{
	return GetProcAddress(plugin->module, symbol);
}

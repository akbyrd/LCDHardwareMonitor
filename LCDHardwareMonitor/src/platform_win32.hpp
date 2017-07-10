//TODO: Rename to win32_platform

//
// Logging
//

inline void
ConsolePrint(c16* message)
{
	OutputDebugStringW(message);
}

#define LOG_HRESULT(message, severity, hr) LogHRESULT(message, severity, hr, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
void
LogHRESULT(c16* message, Severity severity, HRESULT hr, c16* file, i32 line, c16* function)
{
	Log(message, severity, file, line, function);
}

#define LOG_LAST_ERROR(message, severity) LogLastError(message, severity, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
#define LOG_LAST_ERROR_IF(expression, message, severity, ...) IF(expression, LOG_LAST_ERROR(message, severity); __VA_ARGS__)
b32
LogLastError(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	i32 lastError = GetLastError();

	c16 windowsMessage[256];
	u32 uResult = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		GetLastError(),
		0,
		windowsMessage,
		ArrayCount(windowsMessage),
		nullptr
	);

	//TODO: Check for swprintf failures (overflow).
	c16 combinedMessage[256];
	if (uResult == 0)
	{
		LOG_LAST_ERROR(L"FormatMessage failed", Severity::Warning);
		swprintf(combinedMessage, ArrayCount(combinedMessage), L"%s: %i", message, lastError);
	}
	else
	{
		windowsMessage[uResult - 1] = '\0';
		swprintf(combinedMessage, ArrayCount(combinedMessage), L"%s: %s", message, windowsMessage);
	}

	Log(combinedMessage, severity, file, line, function);

	return true;
}

//
// Init and Teardown
//

struct PlatformState
{
	List<Plugin> plugins;
};

b32
InitializePlatform(PlatformState* s)
{
	b32 success = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	//TODO: Does this varargs return actually work?
	LOG_LAST_ERROR_IF(!success, L"SetDefaultDllDirectories failed", Severity::Warning, return false);

	s->plugins = List_Create<Plugin>(16);
	LOG_IF(!s->plugins.items, L"Plugin allocation failed", Severity::Error, return false);

	/* TODO: Seeing some exceptions here when using the Native/Mixed debugger.
	* They seem like first chance exceptions, but it's hard to say.
	*/
	PluginHelper_Initialize();

	return true;
}

void
TeardownPlatform(PlatformState* s)
{
	PluginHelper_Teardown();

	List_Free(s->plugins);
}


//
// File handling
//

#include <fstream>

b32
LoadFile(c16* fileName, unique_ptr<c8[]>& data, size& dataSize)
{
	//TODO: Add cwd to error
	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	if (!inFile.is_open())
	{
		//LOG(L"Failed to open file: " + fileName, Severity::Error);
		LOG(L"Failed to open file: ", Severity::Error);
		return false;
	}

	dataSize = (size) inFile.tellg();
	data = std::make_unique<char[]>(dataSize);

	inFile.seekg(0);
	inFile.read(data.get(), dataSize);

	if (inFile.bad())
	{
		dataSize = 0;
		data = nullptr;

		//LOG(L"Failed to read file: " + fileName, Severity::Error);
		LOG(L"Failed to read file: ", Severity::Error);
		return false;
	}

	return true;
}

//
// Plugins
//

struct Plugin
{
	HMODULE              module;
	DLL_DIRECTORY_COOKIE cookie;
	c16*                 directory;
	c16*                 name;
};

Plugin*
LoadPlugin(PlatformState* s, c16* directory, c16* name)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	Plugin plugin = {};
	plugin.directory = directory;
	plugin.name = name;

	c16 path[MAX_PATH];
	i32 cwdLen = GetCurrentDirectoryW(ArrayCount(path), path);
	LOG_LAST_ERROR_IF(cwdLen == 0, L"GetCurrentDirectory failed", Severity::Warning, return {});
	path[cwdLen++] = '\\';
	errno_t err = wcscpy_s(path + cwdLen, ArrayCount(path) - cwdLen, directory);
	LOG_IF(err, L"wcscpy_s failed", Severity::Warning, return {});

	//Add the plugin directory to the DLL search path so its dependencies will be found.
	plugin.cookie = AddDllDirectory(path);
	LOG_LAST_ERROR_IF(!plugin.cookie, L"AddDllDirectory failed", Severity::Warning, return {});

	//TODO: Does this load dependencies?
	plugin.module = LoadLibraryW(name);
	PluginHelper_PluginLoaded(directory, name);

	//Reuse any empty spots left by unloading plugins
	for (i32 i = 0; i < s->plugins.count; i++)
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

void*
GetPluginSymbol(Plugin* plugin, c8* symbol)
{
	return GetProcAddress(plugin->module, symbol);
}

b32
UnloadPlugin(PlatformState* s, Plugin* plugin)
{
	LOG_IF(!List_Contains(s->plugins, plugin), L"Attempting to unload a plugin that is not loaded", Severity::Warning, return false);

	b32 success = FreeLibrary(plugin->module);
	LOG_LAST_ERROR_IF(!success, L"FreeLibrary failed", Severity::Warning, return false);

	PluginHelper_PluginUnloaded(plugin->directory, plugin->name);

	success = RemoveDllDirectory(plugin->cookie);
	LOG_LAST_ERROR_IF(!success, L"RemoveDllDirectory failed", Severity::Warning);

	*plugin = {};
	return true;
}
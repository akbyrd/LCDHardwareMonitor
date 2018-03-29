//
// Logging
//

inline void
ConsolePrint(c16* message)
{
	OutputDebugStringW(message);
}

void
LogFormatMessage(c16* message, Severity severity, u32 messageID, c16* file, i32 line, c16* function)
{
	c16 windowsMessage[256];
	u32 uResult = FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM,
		nullptr,
		messageID,
		0,
		windowsMessage,
		ArrayLength(windowsMessage) - 1,
		nullptr
	);

	//NOTE: If we fail this far down, just...fuck it.
	c16 combinedMessage[256];
	if (uResult == 0)
	{
		i32 iResult = swprintf(combinedMessage, ArrayLength(combinedMessage), L"%s: %i", message, messageID);
		if (iResult < 0)
		{
			Log(L"swprintf failed", Severity::Warning, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__));
			Log(message, severity, file, line, function);
		}
	}
	else
	{
		windowsMessage[uResult - 1] = '\0';
		i32 iResult = swprintf(combinedMessage, ArrayLength(combinedMessage), L"%s: %s", message, windowsMessage);
		if (iResult < 0)
		{
			Log(L"swprintf failed", Severity::Warning, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__));
			Log(message, severity, file, line, function);
		}
	}

	Log(combinedMessage, severity, file, line, function);
}

#define LOG_HRESULT(message, severity, hr) LogHRESULT(message, severity, hr, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
void
LogHRESULT(c16* message, Severity severity, HRESULT hr, c16* file, i32 line, c16* function)
{
	LogFormatMessage(message, severity, hr, file, line, function);
}

#define LOG_LAST_ERROR(message, severity) LogLastError(message, severity, WIDE(__FILE__), __LINE__, WIDE(__FUNCTION__))
#define LOG_LAST_ERROR_IF(expression, message, severity, ...) IF(expression, LOG_LAST_ERROR(message, severity); __VA_ARGS__)
void
LogLastError(c16* message, Severity severity, c16* file, i32 line, c16* function)
{
	u32 lastError = GetLastError();
	LogFormatMessage(message, severity, lastError, file, line, function);
}

//
// Init and Teardown
//

struct PlatformState
{
	List<Plugin> plugins;
};

b32
Platform_Initialize(PlatformState* s)
{
	b32 success = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
	LOG_LAST_ERROR_IF(!success, L"SetDefaultDllDirectories failed", Severity::Warning, return false);

	s->plugins = {};
	List_Reserve(s->plugins, 16);
	LOG_IF(!s->plugins, L"Plugin allocation failed", Severity::Error, return false);

	PluginHelper_Initialize();

	return true;
}

void
Platform_Teardown(PlatformState* s)
{
	PluginHelper_Teardown();

	List_Free(s->plugins);
}


//
// File handling
//

#include <fstream>

static Bytes
LoadFile(c16* fileName, i32 padding = 0)
{
	Assert(padding >= 0);

	//TODO: Add cwd to errors
	//TODO: Handle files larger than 4 GB
	//TODO: Handle c8/c16/void

	Bytes result = {};

	std::ifstream inFile(fileName, std::ios::binary | std::ios::ate);
	LOG_IF(!inFile.is_open(), L"Failed to open file: <file>. Working Directory: <cwd>", Severity::Error, goto Cleanup);

	result.length   = (i32) inFile.tellg();
	result.capacity = result.length + padding;
	LOG_IF(i32Max - result.length < padding, L"File is too big to fit requested padding when loading: <file>", Severity::Error, goto Cleanup);

	result.data = (u8*) malloc(sizeof(u8) * result.capacity);

	inFile.seekg(0);
	LOG_IF(inFile.fail(), L"Failed to seek file: <file>", Severity::Error, goto Cleanup);
	inFile.read((c8*) result.data, result.length);
	LOG_IF(inFile.fail(), L"Failed to read file: <file>", Severity::Error, goto Cleanup);

	return result;

Cleanup:
	List_Free(result);
	return result;
}

Bytes
LoadFileBytes(c16* fileName)
{
	return LoadFile(fileName, 0);
}

String
LoadFileString(c16* fileName)
{
	String result = {};

	Bytes bytes = LoadFile(fileName, 1);
	if (!bytes) return result;

	result.length   = bytes.length;
	result.capacity = bytes.capacity;
	result.data     = (c8*) bytes.data;

	result.data[result.length++] = '\0';

	return result;
}

WString
LoadFileWString(c16* fileName)
{
	WString result = {};
	Bytes bytes = LoadFile(fileName, 1);

	result.length   = bytes.length;
	result.capacity = bytes.capacity;
	result.data     = (c16*) bytes.data;

	result.data[result.length++] = '\0';

	return result;
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
	i32 cwdLen = GetCurrentDirectoryW(ArrayLength(path), path);
	LOG_LAST_ERROR_IF(cwdLen == 0, L"GetCurrentDirectory failed", Severity::Warning, return {});
	path[cwdLen++] = '\\';
	errno_t err = wcscpy_s(path + cwdLen, ArrayLength(path) - cwdLen, directory);
	LOG_IF(err, L"wcscpy_s failed", Severity::Warning, return {});

	//Add the plugin directory to the DLL search path so its dependencies will be found.
	plugin.cookie = AddDllDirectory(path);
	LOG_LAST_ERROR_IF(!plugin.cookie, L"AddDllDirectory failed", Severity::Warning, return {});

	plugin.module = LoadLibraryW(name);
	PluginHelper_PluginLoaded(directory, name);

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

void*
GetPluginSymbol(Plugin* plugin, c8* symbol)
{
	return GetProcAddress(plugin->module, symbol);
}

b32
UnloadPlugin(PlatformState* s, Plugin* plugin)
{
	b32 loaded = List_Contains(s->plugins, plugin);
	LOG_IF(!loaded, L"Attempting to unload a plugin that is not loaded", Severity::Warning, return false);

	b32 success = FreeLibrary(plugin->module);
	LOG_LAST_ERROR_IF(!success, L"FreeLibrary failed", Severity::Warning, return false);

	PluginHelper_PluginUnloaded(plugin->directory, plugin->name);

	success = RemoveDllDirectory(plugin->cookie);
	LOG_LAST_ERROR_IF(!success, L"RemoveDllDirectory failed", Severity::Warning);

	*plugin = {};
	return true;
}

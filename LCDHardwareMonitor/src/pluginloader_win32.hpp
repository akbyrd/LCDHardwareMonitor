#include <metahost.h>

#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#import "../Run Tree/LCDHardwareMonitor AppDomainManager.tlb" no_namespace

class CLRHostControl : public IHostControl
{
public:
	HRESULT __stdcall GetHostManager(REFIID riid, void** ppObject)
	{
		*ppObject = nullptr;
		return E_NOINTERFACE;
	}

	HRESULT __stdcall SetAppDomainManager(DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager)
	{
		HRESULT hr;
		hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&lhmAppDomainManager));
		return hr;
	}

	//TODO: Should this be done though QueryInterface?
	ILHMAppDomainManager* GetAppDomainManager() { return lhmAppDomainManager.Get(); }

	HRESULT __stdcall QueryInterface(REFIID riid, void **ppObject)
	{
		if (!ppObject) return E_POINTER;
		*ppObject = nullptr;

		//riid == __uuidof(lhmAppDomainManager)
		if (riid == IID_IUnknown
		 || riid == IID_IHostControl)
		{
			*ppObject = this;
			AddRef();
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	ULONG __stdcall AddRef()  { return InterlockedIncrement(&refCount); }
	ULONG __stdcall Release() { return InterlockedDecrement(&refCount); }

private:
	ComPtr<ILHMAppDomainManager> lhmAppDomainManager;
	ULONG                        refCount;
};

enum struct PluginKind
{
	Null,
	Native,
	Managed,
};

struct Plugin
{
	//Managed Old
	HMODULE              module;
	DLL_DIRECTORY_COOKIE cookie;

	//Managed New
	//TODO: Is this stable across unloads and loads?
	u32                  appDomainID;

	b32                  isLoaded;
	PluginKind           kind;
	c16*                 directory;
	c16*                 name;
};

struct PluginLoaderState
{
	ComPtr<ICLRRuntimeHost>      clrHost;
	CLRHostControl               clrHostControl;
	ComPtr<ILHMAppDomainManager> lhmAppDomainManager;
	List<Plugin>                 plugins;
};

b32
PluginLoader_Initialize(PluginLoaderState* s)
{
	//CLR_Initialize();
	//TODO: Ensure this fails gracefully if the CLR isn't installed.
	{
		HRESULT hr;

		ComPtr<ICLRMetaHost> clrMetaHost;
		hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&clrMetaHost));
		LOG_HRESULT_IF_FAILED(hr, L"CLRCreateInstance failed", Severity::Error, return false);

		//TODO: Enumerate installed versions and give a helpful error message.
		//NOTE: Relying on this version should be safe. Use clrver to check installed versions.
		ComPtr<ICLRRuntimeInfo> clrInfo;
		hr = clrMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&clrInfo));
		LOG_HRESULT_IF_FAILED(hr, L"ICLRMetaHost->GetRuntime failed", Severity::Error, return false);

		hr = clrInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&s->clrHost));
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeInfo->GetInterface failed", Severity::Error, return false);

		ComPtr<ICLRControl> clrControl;
		hr = s->clrHost->GetCLRControl(&clrControl);
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->GetCLRControl failed", Severity::Error, return false);

		hr = clrControl->SetAppDomainManagerType(L"LCDHardwareMonitor AppDomainManager", L"LHMAppDomainManager");
		LOG_HRESULT_IF_FAILED(hr, L"ICLRControl->SetAppDomainManagerType failed", Severity::Error, return false);

		hr = s->clrHost->SetHostControl(&s->clrHostControl);
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->SetHostControl failed", Severity::Error, return false);

		hr = s->clrHost->Start();
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->Start failed", Severity::Error, return false);

		s->lhmAppDomainManager = s->clrHostControl.GetAppDomainManager();
	}

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

	//CLR_Teardown();
	{
		HRESULT hr;

		hr = s->clrHost->Stop();
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->Stop failed", Severity::Error);

		s->clrHost.Reset();
	}
}

//TODO: Inline this
PluginKind
GetPluginKind(c16* directory, c16* name)
{
	//TODO: Actually check if the DLL is native or managed
	return PluginKind::Managed;
}

b32
LoadNativePlugin(Plugin* plugin)
{
	//TODO: Implement LoadNativePlugin
	Assert(false);
	return false;
}

b32
UnloadNativePlugin(Plugin* plugin)
{
	//TODO: Implement UnloadNativePlugin
	Assert(false);
	return false;
}

b32
LoadManagedPluginOLD(Plugin* plugin)
{
	c16 path[MAX_PATH];
	i32 cwdLen = GetCurrentDirectoryW(ArrayLength(path), path);
	LOG_LAST_ERROR_IF(cwdLen == 0, L"GetCurrentDirectory failed", Severity::Warning, return false);
	path[cwdLen++] = '\\';
	errno_t err = wcscpy_s(path + cwdLen, ArrayLength(path) - cwdLen, plugin->directory);
	LOG_IF(err, L"wcscpy_s failed", Severity::Warning, return false);

	//Add the plugin directory to the DLL search path so its dependencies will be found.
	plugin->cookie = AddDllDirectory(path);
	LOG_LAST_ERROR_IF(!plugin->cookie, L"AddDllDirectory failed", Severity::Warning, return false);

	plugin->module = LoadLibraryW(plugin->name);
	CLR_PluginLoaded(plugin->directory, plugin->name);

	return true;
}

b32
UnloadManagedPluginOLD(Plugin* plugin)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	b32 success;

	success = FreeLibrary(plugin->module);
	LOG_LAST_ERROR_IF(!success, L"FreeLibrary failed", Severity::Warning, return false);

	CLR_PluginUnloaded(plugin->directory, plugin->name);

	success = RemoveDllDirectory(plugin->cookie);
	LOG_LAST_ERROR_IF(!success, L"RemoveDllDirectory failed", Severity::Warning, return false);

	return true;
}

b32
LoadManagedPlugin(PluginLoaderState* s, Plugin* plugin)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	//TODO: Can this be done in native? Is that better?
	plugin->appDomainID = s->lhmAppDomainManager->LoadAppDomain(plugin->name, plugin->directory);
	//s->lhmAppDomainManager->LoadPlugin(plugin->name, plugin->directory);

	return true;
}

b32
UnloadManagedPlugin(PluginLoaderState* s, Plugin* plugin)
{
	HRESULT hr;
	hr = s->clrHost->UnloadAppDomain(plugin->appDomainID, true);
	LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->UnloadAppDomain failed", Severity::Error, return false);

	return true;
}

Plugin*
PluginLoader_LoadPlugin(PluginLoaderState* s, c16* directory, c16* name)
{
	Plugin plugin = {};
	plugin.kind      = GetPluginKind(directory, name);
	plugin.isLoaded  = true;
	plugin.directory = directory;
	plugin.name      = name;

	b32 success = false;
	switch (plugin.kind) {
		case PluginKind::Null:    success = false;                         break;
		case PluginKind::Native:  success = LoadNativePlugin(&plugin);     break;
		case PluginKind::Managed: success = LoadManagedPlugin(s, &plugin); break;
	}
	if (!success) return nullptr;

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

	b32 success;
	switch (plugin->kind) {
		case PluginKind::Null:    success = false;                          break;
		case PluginKind::Native:  success = UnloadNativePlugin(plugin);     break;
		case PluginKind::Managed: success = UnloadManagedPlugin(s, plugin); break;
	}
	if (!success) return false;

	List_RemoveFast(s->plugins, *plugin);
	return true;
}

void*
PluginLoader_GetPluginSymbol(PluginLoaderState* s, Plugin* plugin, c8* symbol)
{
	return GetProcAddress(plugin->module, symbol);
}

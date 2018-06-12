#include <metahost.h>
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

/* TODO: We don't really need this in run/. Want to reach into the
 * project output folder directly, but we need to know the correct config
 * subfolder. */
#import "../run/LCDHardwareMonitor PluginLoader CLR.tlb" no_namespace

class LHMHostControl : public IHostControl
{
public:
	//TODO: Double check that this doesn't get called for each new AppDomain
	HRESULT __stdcall SetAppDomainManager(DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager)
	{
		HRESULT hr;
		hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&lhmPluginLoader));
		return hr;
	}

	ILHMPluginLoader* GetAppDomainManager() { return lhmPluginLoader.Get(); }

	HRESULT __stdcall GetHostManager(REFIID, void**) { return E_NOTIMPL; }
	HRESULT __stdcall QueryInterface(REFIID, void**) { return E_NOTIMPL; }
	ULONG   __stdcall AddRef()  { return 0; }
	ULONG   __stdcall Release() { return 0; }

private:
	ComPtr<ILHMPluginLoader> lhmPluginLoader;
};

struct PluginLoaderState
{
	ComPtr<ICLRRuntimeHost>  clrHost;
	LHMHostControl           lhmHostControl;
	ComPtr<ILHMPluginLoader> lhmPluginLoader;
};

b32
PluginLoader_Initialize(PluginLoaderState* s)
{
	//Managed
	{
		HRESULT hr;

		ComPtr<ICLRMetaHost> clrMetaHost;
		hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&clrMetaHost));
		LOG_HRESULT_IF_FAILED(hr, L"CLRCreateInstance failed", Severity::Error, return false);

		//TODO: Ensure this fails gracefully if the CLR isn't installed.
		//TODO: Enumerate installed versions and give a helpful error message.
		//NOTE: Relying on this version should be safe. Use clrver to check installed versions.
		ComPtr<ICLRRuntimeInfo> clrInfo;
		hr = clrMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&clrInfo));
		LOG_HRESULT_IF_FAILED(hr, L"ICLRMetaHost->GetRuntime failed", Severity::Error, return false);

		u32 startupFlags = STARTUP_LOADER_OPTIMIZATION_MASK | STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST;
		hr = clrInfo->SetDefaultStartupFlags(startupFlags, nullptr);
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeInfo->SetDefaultStartupFlags failed", Severity::Error, return false);

		hr = clrInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&s->clrHost));
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeInfo->GetInterface failed", Severity::Error, return false);

		ComPtr<ICLRControl> clrControl;
		hr = s->clrHost->GetCLRControl(&clrControl);
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->GetCLRControl failed", Severity::Error, return false);

		hr = clrControl->SetAppDomainManagerType(L"LCDHardwareMonitor PluginLoader CLR", L"LHMPluginLoader");
		LOG_HRESULT_IF_FAILED(hr, L"ICLRControl->SetAppDomainManagerType failed", Severity::Error, return false);

		hr = s->clrHost->SetHostControl(&s->lhmHostControl);
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->SetHostControl failed", Severity::Error, return false);

		hr = s->clrHost->Start();
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->Start failed", Severity::Error, return false);

		s->lhmPluginLoader = s->lhmHostControl.GetAppDomainManager();
	}


	//Unmanaged
	{
		b32 success = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		LOG_LAST_ERROR_IF(!success, L"SetDefaultDllDirectories failed", Severity::Warning, return false);
	}

	return true;
}

void
PluginLoader_Teardown(PluginLoaderState* s)
{
	//Managed
	{
		HRESULT hr;

		hr = s->clrHost->Stop();
		LOG_HRESULT_IF_FAILED(hr, L"ICLRRuntimeHost->Stop failed", Severity::Error);

		s->clrHost.Reset();
	}
}

b32
LoadNativeSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	//TODO: Implement LoadNativeSensorPlugin
	Assert(false);
	return false;
}

b32
UnloadNativeSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	//TODO: Implement UnloadNativeSensorPlugin
	Assert(false);
	return false;
}

b32
LoadManagedSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	//TODO: Do we need to try/catch the managed code?
	b32 success;
	success = s->lhmPluginLoader->LoadSensorPlugin(pluginHeader, sensorPlugin);
	LOG_IF(!success, L"Failed to load managed sensor plugin", Severity::Warning, return false);

	return true;
}

b32
UnloadManagedSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	b32 success;
	success = s->lhmPluginLoader->UnloadSensorPlugin(pluginHeader, sensorPlugin);
	LOG_IF(!success, L"Failed to unload managed sensor plugin", Severity::Warning, return false);

	return true;
}

b32
LoadNativeWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
{
	//TODO: Implement LoadNativeWidgetPlugin
	Assert(false);
	return false;
}

b32
UnloadNativeWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* WidgetPlugin)
{
	//TODO: Implement UnloadNativeWidgetPlugin
	Assert(false);
	return false;
}

b32
LoadManagedWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* WidgetPlugin)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	//TODO: Do we need to try/catch the managed code?
	b32 success;
	success = s->lhmPluginLoader->LoadWidgetPlugin(pluginHeader, WidgetPlugin);
	LOG_IF(!success, L"Failed to load managed Widget plugin", Severity::Warning, return false);

	return true;
}

b32
UnloadManagedWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* WidgetPlugin)
{
	b32 success;
	success = s->lhmPluginLoader->UnloadWidgetPlugin(pluginHeader, WidgetPlugin);
	LOG_IF(!success, L"Failed to unload managed Widget plugin", Severity::Warning, return false);

	return true;
}

b32
PluginLoader_LoadSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	//TODO: Actually check if the DLL is native or managed
	pluginHeader->language = PluginLanguage::Managed;

	b32 success = false;
	switch (pluginHeader->language)
	{
		case PluginLanguage::Null:    success = false;                                                  break;
		case PluginLanguage::Native:  success = LoadNativeSensorPlugin(s, pluginHeader, sensorPlugin);  break;
		case PluginLanguage::Managed: success = LoadManagedSensorPlugin(s, pluginHeader, sensorPlugin); break;
	}
	if (!success) return false;

	pluginHeader->isLoaded = true;
	return true;
}

b32
PluginLoader_UnloadSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	b32 success = false;
	switch (pluginHeader->language)
	{
		case PluginLanguage::Null:    success = false;                                                    break;
		case PluginLanguage::Native:  success = UnloadNativeSensorPlugin(s, pluginHeader, sensorPlugin);  break;
		case PluginLanguage::Managed: success = UnloadManagedSensorPlugin(s, pluginHeader, sensorPlugin); break;
	}
	if (!success) return false;

	return true;
}

b32
PluginLoader_LoadWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
{
	//TODO: Actually check if the DLL is native or managed
	pluginHeader->language = PluginLanguage::Managed;

	b32 success = false;
	switch (pluginHeader->language)
	{
		case PluginLanguage::Null:    success = false;                                                  break;
		case PluginLanguage::Native:  success = LoadNativeWidgetPlugin(s, pluginHeader, widgetPlugin);  break;
		case PluginLanguage::Managed: success = LoadManagedWidgetPlugin(s, pluginHeader, widgetPlugin); break;
	}
	if (!success) return false;

	pluginHeader->isLoaded = true;
	return true;
}

b32
PluginLoader_UnloadWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
{
	b32 success = false;
	switch (pluginHeader->language)
	{
		case PluginLanguage::Null:    success = false;                                                    break;
		case PluginLanguage::Native:  success = UnloadNativeWidgetPlugin(s, pluginHeader, widgetPlugin);  break;
		case PluginLanguage::Managed: success = UnloadManagedWidgetPlugin(s, pluginHeader, widgetPlugin); break;
	}
	if (!success) return false;

	return true;
}

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
LoadNativeDataSource(PluginLoaderState* s, DataSource* dataSource)
{
	//TODO: Implement LoadNativeDataSource
	Assert(false);
	return false;
}

b32
UnloadNativeDataSource(PluginLoaderState* s, DataSource* dataSource)
{
	//TODO: Implement UnloadNativeDataSource
	Assert(false);
	return false;
}

b32
LoadManagedDataSource(PluginLoaderState* s, DataSource* dataSource)
{
	//NOTE: fuslogvw is great for debugging managed assembly loading.

	//TODO: Do we need to try/catch the managed code?
	b32 success;
	success = s->lhmPluginLoader->LoadDataSource(dataSource);
	LOG_IF(!success, L"Failed to load managed data source", Severity::Warning, return false);

	return true;
}

b32
UnloadManagedDataSource(PluginLoaderState* s, DataSource* dataSource)
{
	b32 success;
	success = s->lhmPluginLoader->UnloadDataSource(dataSource);
	LOG_IF(!success, L"Failed to unload managed data source", Severity::Warning, return false);

	return true;
}

b32
PluginLoader_LoadDataSource(PluginLoaderState* s, DataSource* dataSource)
{
	PluginInfo* pluginInfo = dataSource->pluginInfo;

	//TODO: Actually check if the DLL is native or managed
	pluginInfo->kind = PluginKind::Managed;

	b32 success = false;
	switch (pluginInfo->kind)
	{
		case PluginKind::Null:    success = false;                                break;
		case PluginKind::Native:  success = LoadNativeDataSource(s, dataSource);  break;
		case PluginKind::Managed: success = LoadManagedDataSource(s, dataSource); break;
	}
	if (!success) return false;

	pluginInfo->isLoaded = true;
	return true;
}

b32
PluginLoader_UnloadDataSource(PluginLoaderState* s, DataSource* dataSource)
{
	PluginInfo* pluginInfo = dataSource->pluginInfo;

	b32 success = false;
	switch (pluginInfo->kind)
	{
		case PluginKind::Null:    success = false;                                  break;
		case PluginKind::Native:  success = UnloadNativeDataSource(s, dataSource);  break;
		case PluginKind::Managed: success = UnloadManagedDataSource(s, dataSource); break;
	}
	if (!success) return false;

	return true;
}

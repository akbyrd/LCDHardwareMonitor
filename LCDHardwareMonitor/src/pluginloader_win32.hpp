#pragma warning(push, 0)
#include <winnt.h>
#include <metahost.h>
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#include <CorHdr.h>
#pragma warning(pop)

// TODO: Don't really need this in run/. Want to reach into the project output
// folder directly, but we need to know the correct config subfolder.
#import "..\\run\\LCDHardwareMonitor PluginLoader CLR.tlb" no_namespace

class LHMHostControl final : public IHostControl
{
public:
	// TODO: Double check that this doesn't get called for each new AppDomain
	HRESULT __stdcall SetAppDomainManager(DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager)
	{
		UNUSED(dwAppDomainID);
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
	// Managed
	{
		HRESULT hr;

		ComPtr<ICLRMetaHost> clrMetaHost;
		hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&clrMetaHost));
		LOG_HRESULT_IF_FAILED(hr, "CLRCreateInstance failed", Severity::Error, return false);

		// TODO: Ensure this fails gracefully if the CLR isn't installed.
		// TODO: Enumerate installed versions and give a helpful error message.
		// NOTE: Relying on this version should be safe. Use clrver to check
		// installed versions.
		ComPtr<ICLRRuntimeInfo> clrInfo;
		hr = clrMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&clrInfo));
		LOG_HRESULT_IF_FAILED(hr, "ICLRMetaHost->GetRuntime failed", Severity::Error, return false);

		u32 startupFlags = STARTUP_LOADER_OPTIMIZATION_MASK | STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST;
		hr = clrInfo->SetDefaultStartupFlags(startupFlags, nullptr);
		LOG_HRESULT_IF_FAILED(hr, "ICLRRuntimeInfo->SetDefaultStartupFlags failed", Severity::Error, return false);

		hr = clrInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&s->clrHost));
		LOG_HRESULT_IF_FAILED(hr, "ICLRRuntimeInfo->GetInterface failed", Severity::Error, return false);

		ComPtr<ICLRControl> clrControl;
		hr = s->clrHost->GetCLRControl(&clrControl);
		LOG_HRESULT_IF_FAILED(hr, "ICLRRuntimeHost->GetCLRControl failed", Severity::Error, return false);

		hr = clrControl->SetAppDomainManagerType(L"LCDHardwareMonitor PluginLoader CLR", L"LHMPluginLoader");
		LOG_HRESULT_IF_FAILED(hr, "ICLRControl->SetAppDomainManagerType failed", Severity::Error, return false);

		hr = s->clrHost->SetHostControl(&s->lhmHostControl);
		LOG_HRESULT_IF_FAILED(hr, "ICLRRuntimeHost->SetHostControl failed", Severity::Error, return false);

		hr = s->clrHost->Start();
		LOG_HRESULT_IF_FAILED(hr, "ICLRRuntimeHost->Start failed", Severity::Error, return false);

		s->lhmPluginLoader = s->lhmHostControl.GetAppDomainManager();
	}


	// Unmanaged
	{
		b32 success = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		LOG_LAST_ERROR_IF(!success, "SetDefaultDllDirectories failed", Severity::Warning, return false);
	}

	return true;
}

void
PluginLoader_Teardown(PluginLoaderState* s)
{
	// Managed
	{
		HRESULT hr;

		hr = s->clrHost->Stop();
		LOG_HRESULT_IF_FAILED(hr, "ICLRRuntimeHost->Stop failed", Severity::Error);

		s->clrHost.Reset();
	}
}

static b32
DetectPluginLanguage(PluginHeader* pluginHeader)
{
	// TODO: Move memory mapping to platform API?

	String pluginPath = {};
	b32 success = String_Format(pluginPath, "%s\\%s.dll", pluginHeader->directory, pluginHeader->name);
	if (!success) return false;
	defer { List_Free(pluginPath); };

	HANDLE pluginFile = CreateFileA(
		pluginPath,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	LOG_LAST_ERROR_IF(pluginFile == INVALID_HANDLE_VALUE, "Failed to open plugin file.", Severity::Warning, return false);
	defer { CloseHandle(pluginFile); };

	HANDLE pluginFileMap = CreateFileMappingA(
		pluginFile,
		nullptr,
		PAGE_READONLY,
		0, 0,
		nullptr
	);
	LOG_LAST_ERROR_IF(!pluginFileMap, "Failed to create plugin file mapping.", Severity::Warning, return false);
	defer { CloseHandle(pluginFileMap); };

	// TODO: We could map the DOS header, get the PE offset, then remap starting there
	// NOTE: Can't map a fixed number of bytes because the DOS stub is of unknown length
	void* pluginFileMemory = MapViewOfFile(pluginFileMap, FILE_MAP_READ, 0, 0, 0);
	LOG_LAST_ERROR_IF(!pluginFileMemory, "Failed to map view of plugin file.", Severity::Warning, return false);
	defer {
		if (pluginFileMemory)
		{
			// BUG: Failures here will not propagate back to the calling function
			// TODO: What happens if Unmap is called with null? Can we drop the check?
			b32 success = UnmapViewOfFile(pluginFileMemory);
			LOG_IF(!success, "Failed to unmap view of plugin file.", Severity::Error);
		}
	};

	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*) pluginFileMemory;
	LOG_IF(dosHeader->e_magic != IMAGE_DOS_SIGNATURE, "Plugin file does not have a proper DOS header.", Severity::Warning, return false);

	IMAGE_NT_HEADERS* ntHeader = (IMAGE_NT_HEADERS*) ((u8*) pluginFileMemory + dosHeader->e_lfanew);
	LOG_IF(ntHeader->Signature != IMAGE_NT_SIGNATURE, "Plugin file does not have a proper NT header.", Severity::Warning, return false);

	IMAGE_FILE_HEADER* fileHeader = &ntHeader->FileHeader;
	LOG_IF(fileHeader->SizeOfOptionalHeader == 0, "Plugin file does not have an optional header.", Severity::Warning, return false);
	LOG_IF(!HAS_FLAG(fileHeader->Characteristics, IMAGE_FILE_DLL), "Plugin file is not a dynamically linked library.", Severity::Warning, return false);

	IMAGE_OPTIONAL_HEADER* optionalHeader = &ntHeader->OptionalHeader;
	LOG_IF(optionalHeader->Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC, "Plugin file does not have a proper optional header.", Severity::Warning, return false);

	if (optionalHeader->NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COMHEADER)
	{
		pluginHeader->language = PluginLanguage::Native;
	}
	else
	{
		IMAGE_DATA_DIRECTORY* clrDirectory = &optionalHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
		pluginHeader->language = clrDirectory->Size == 0 ? PluginLanguage::Native : PluginLanguage::Managed;
	}

	return true;
}

b32
PluginLoader_LoadSensorPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, SensorPlugin* sensorPlugin)
{
	b32 success = false;

	success = DetectPluginLanguage(pluginHeader);
	if (!success) return false;

	switch (pluginHeader->language)
	{
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Native:
		{
			success = false;
			// TODO: Will this work with delay loaded dependencies?
			SetDllDirectoryA(pluginHeader->directory);
			pluginHeader->userData = LoadLibraryA(pluginHeader->name);
			LOG_IF(!pluginHeader->userData, "Failed to load unmanaged Sensor plugin", Severity::Warning, break);
			SetDllDirectoryA("");

			// TODO: Remove these casts
			sensorPlugin->initialize = (SensorPluginInitializeFn*) (void*) GetProcAddress((HMODULE) pluginHeader->userData, "Initialize");
			LOG_IF(!sensorPlugin->initialize, "Failed to find unmanaged Sensor Initialize function", Severity::Warning, break);

			sensorPlugin->update = (SensorPluginUpdateFn*) (void*) GetProcAddress((HMODULE) pluginHeader->userData, "Update");
			LOG_IF(!sensorPlugin->update, "Failed to find unmanaged Sensor Update function", Severity::Warning, break);

			sensorPlugin->teardown = (SensorPluginTeardownFn*) (void*) GetProcAddress((HMODULE) pluginHeader->userData, "Teardown");
			LOG_IF(!sensorPlugin->teardown, "Failed to find unmanaged Sensor Teardown function", Severity::Warning, break);
			success = true;
			break;
		}

		case PluginLanguage::Managed:
		{
			// NOTE: fuslogvw is great for debugging managed assembly loading.
			// TODO: Do we need to try/catch the managed code?
			success = s->lhmPluginLoader->LoadSensorPlugin(pluginHeader, sensorPlugin);
			LOG_IF(!success, "Failed to load managed Sensor plugin", Severity::Warning);
			break;
		}
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
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Native:
			// TODO: Remove this cast
			success = FreeLibrary((HMODULE) pluginHeader->userData);
			LOG_IF(!success, "Failed to unload unmanaged Sensor plugin", Severity::Warning, break);
			pluginHeader->userData = nullptr;
			break;

		case PluginLanguage::Managed:
			success = s->lhmPluginLoader->UnloadSensorPlugin(pluginHeader, sensorPlugin);
			LOG_IF(!success, "Failed to unload managed Sensor plugin", Severity::Warning);
			break;
	}
	if (!success) return false;

	pluginHeader->isLoaded = false;
	return true;
}

b32
PluginLoader_LoadWidgetPlugin(PluginLoaderState* s, PluginHeader* pluginHeader, WidgetPlugin* widgetPlugin)
{
	b32 success = false;

	success = DetectPluginLanguage(pluginHeader);
	if (!success) return false;

	switch (pluginHeader->language)
	{
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Native:
		{
			success = false;
			// TODO: Will this work with delay loaded dependencies?
			SetDllDirectoryA(pluginHeader->directory);
			pluginHeader->userData = LoadLibraryA(pluginHeader->name);
			LOG_IF(!pluginHeader->userData, "Failed to load unmanaged Widget plugin", Severity::Warning, break);
			SetDllDirectoryA("");

			// TODO: Remove these casts
			widgetPlugin->initialize = (WidgetPluginInitializeFn*) (void*) GetProcAddress((HMODULE) pluginHeader->userData, "Initialize");
			LOG_IF(!widgetPlugin->initialize, "Failed to find unmanaged Widget Initialize function", Severity::Warning, break);

			widgetPlugin->update = (WidgetPluginUpdateFn*) (void*) GetProcAddress((HMODULE) pluginHeader->userData, "Update");
			LOG_IF(!widgetPlugin->update, "Failed to find unmanaged Widget Update function", Severity::Warning, break);

			widgetPlugin->teardown = (WidgetPluginTeardownFn*) (void*) GetProcAddress((HMODULE) pluginHeader->userData, "Teardown");
			LOG_IF(!widgetPlugin->teardown, "Failed to find unmanaged Widget Teardown function", Severity::Warning, break);
			success = true;
			break;
		}

		case PluginLanguage::Managed:
		{
			// NOTE: fuslogvw is great for debugging managed assembly loading.
			// TODO: Do we need to try/catch the managed code?
			success = s->lhmPluginLoader->LoadWidgetPlugin(pluginHeader, widgetPlugin);
			LOG_IF(!success, "Failed to load managed Widget plugin", Severity::Warning);
			break;
		}
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
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Native:
			// TODO: Remove this cast
			success = FreeLibrary((HMODULE) pluginHeader->userData);
			LOG_IF(!success, "Failed to unload unmanaged Widget plugin", Severity::Warning, break);
			pluginHeader->userData = nullptr;
			break;

		case PluginLanguage::Managed:
			success = s->lhmPluginLoader->UnloadWidgetPlugin(pluginHeader, widgetPlugin);
			LOG_IF(!success, "Failed to unload managed Widget plugin", Severity::Warning);
			break;
	}
	if (!success) return false;

	pluginHeader->isLoaded = false;
	return true;
}

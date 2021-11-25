#pragma warning(push, 0)
#include <winnt.h>
#include <metahost.h>
#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#include <CorHdr.h>
#pragma warning(pop)

// NOTE: Required for CLRCreateInstance
#pragma comment(lib, "MSCorEE.lib")

#pragma warning(push)
// NOTE: The automatically included comdef.h in the tlh generates warnings
#pragma warning(disable: 5204) // non-virtual destructor
// NOTE: Added a hack "additional include directory" for this to work
#import "LCDHardwareMonitor.PluginLoader.CLR.Interface.tlb" \
	no_namespace \
	no_smart_pointers
#pragma warning(pop)

struct LHMHostControl final : public IHostControl
{
	// TODO: Double check that this doesn't get called for each new AppDomain
	HRESULT __stdcall SetAppDomainManager(DWORD dwAppDomainID, IUnknown* pUnkAppDomainManager)
	{
		Unused(dwAppDomainID);
		HRESULT hr;
		hr = pUnkAppDomainManager->QueryInterface(IID_PPV_ARGS(&lhmPluginLoader));
		return hr;
	}

	ILHMPluginLoader* GetAppDomainManager() { return lhmPluginLoader.Get(); }

	HRESULT __stdcall GetHostManager(REFIID, void**) { return E_NOTIMPL; }
	HRESULT __stdcall QueryInterface(REFIID, void**) { return E_NOTIMPL; }
	ULONG   __stdcall AddRef()  { return 0; }
	ULONG   __stdcall Release() { return 0; }

	ComPtr<ILHMPluginLoader> lhmPluginLoader;
};

struct PluginLoaderState
{
	ComPtr<ICLRRuntimeHost>  clrHost;
	LHMHostControl           lhmHostControl;
	ILHMPluginLoader*        lhmPluginLoader;
};

b8
PluginLoader_Initialize(PluginLoaderState& s)
{
	// Managed
	{
		HRESULT hr;

		ComPtr<ICLRMetaHost> clrMetaHost;
		hr = CLRCreateInstance(CLSID_CLRMetaHost, IID_PPV_ARGS(&clrMetaHost));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to create CLR instance");

		// TODO: Ensure this fails gracefully if the CLR isn't installed.
		// TODO: Enumerate installed versions and give a helpful error message.
		// NOTE: Relying on this version should be safe. Use clrver to check installed versions.
		ComPtr<ICLRRuntimeInfo> clrInfo;
		hr = clrMetaHost->GetRuntime(L"v4.0.30319", IID_PPV_ARGS(&clrInfo));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get desired CLR runtime");

		u32 startupFlags = STARTUP_LOADER_OPTIMIZATION_MASK | STARTUP_LOADER_OPTIMIZATION_MULTI_DOMAIN_HOST;
		hr = clrInfo->SetDefaultStartupFlags(startupFlags, nullptr);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to set CLR startup flags");

		hr = clrInfo->GetInterface(CLSID_CLRRuntimeHost, IID_PPV_ARGS(&s.clrHost));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get CLR runtime host interface");

		ComPtr<ICLRControl> clrControl;
		hr = s.clrHost->GetCLRControl(&clrControl);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to CLR control interface");

		hr = clrControl->SetAppDomainManagerType(L"LCDHardwareMonitor.PluginLoader.CLR", L"LHMPluginLoader");
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to set CLR AppDomain manager");

		hr = s.clrHost->SetHostControl(&s.lhmHostControl);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to set CLR host control");

		hr = s.clrHost->Start();
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to start CLR");

		s.lhmPluginLoader = s.lhmHostControl.GetAppDomainManager();
	}


	// Unmanaged
	{
		b8 success = SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
		LOG_LAST_ERROR_IF(!success, return false,
			Severity::Fatal, "Failed to set DLL directories");
	}

	return true;
}

void
PluginLoader_Teardown(PluginLoaderState& s)
{
	// Managed
	{
		// TODO: Placing a breakpoint on the following line when using the
		// mixed debugger and attempting to step over causes a crash. Placing
		// breakpoints on code that occurs after this point may also crash
		// (e.g. Renderer_Teardown).

		HRESULT hr = s.clrHost->Stop();
		LOG_HRESULT_IF_FAILED(hr, IGNORE,
			Severity::Fatal, "Failed to stop CLR");

		s.clrHost.Reset();
	}

	s = {};
}

static b8
DetectPluginLanguage(Plugin& plugin)
{
	// TODO: Move memory mapping to platform API?

	String pluginPath = String_Format("%\\%", plugin.directory, plugin.fileName);
	defer { String_Free(pluginPath); };

	HANDLE pluginFile = CreateFileA(
		pluginPath.data,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	defer { CloseHandle(pluginFile); };
	if (pluginFile == INVALID_HANDLE_VALUE)
	{
		String cwd = GetWorkingDirectory();
		defer { String_Free(cwd); };
		LOG_LAST_ERROR(Severity::Error, "Failed to open plugin file '%'; CWD: '%'", pluginPath, cwd);
		return false;
	}

	HANDLE pluginFileMap = CreateFileMappingA(
		pluginFile,
		nullptr,
		PAGE_READONLY,
		0, 0,
		nullptr
	);
	LOG_LAST_ERROR_IF(!pluginFileMap, return false,
		Severity::Error, "Failed to create plugin file mapping '%'", plugin.fileName);
	defer { CloseHandle(pluginFileMap); };

	// TODO: We could map the DOS header, get the PE offset, then remap starting there
	// NOTE: Can't map a fixed number of bytes because the DOS stub is of unknown length
	u8* pluginFileMemory = (u8*) MapViewOfFile(pluginFileMap, FILE_MAP_READ, 0, 0, 0);
	LOG_LAST_ERROR_IF(!pluginFileMemory, return false,
		Severity::Error, "Failed to map view of plugin file '%'", plugin.fileName);
	defer {
		if (pluginFileMemory)
		{
			// BUG: Failures here will not propagate back to the calling function
			// TODO: What happens if Unmap is called with null? Can we drop the check?
			b8 success = UnmapViewOfFile(pluginFileMemory);
			LOG_IF(!success, IGNORE,
				Severity::Error, "Failed to unmap view of plugin file '%'", plugin.fileName);
		}
	};

	IMAGE_DOS_HEADER& dosHeader = *(IMAGE_DOS_HEADER*) pluginFileMemory;
	LOG_IF(dosHeader.e_magic != IMAGE_DOS_SIGNATURE, return false,
		Severity::Error, "Plugin file does not have a proper DOS header '%'", plugin.fileName);

	IMAGE_NT_HEADERS& ntHeader = (IMAGE_NT_HEADERS&) pluginFileMemory[dosHeader.e_lfanew];
	LOG_IF(ntHeader.Signature != IMAGE_NT_SIGNATURE, return false,
		Severity::Error, "Plugin file does not have a proper NT header '%'", plugin.fileName);

	IMAGE_FILE_HEADER& fileHeader = ntHeader.FileHeader;
	LOG_IF(fileHeader.SizeOfOptionalHeader == 0, return false,
		Severity::Error, "Plugin file does not have an optional header '%'", plugin.fileName);
	LOG_IF(!HAS_FLAG(fileHeader.Characteristics, IMAGE_FILE_DLL), return false,
		Severity::Error, "Plugin file is not a DLL '%'", plugin.fileName);

	IMAGE_OPTIONAL_HEADER& optionalHeader = ntHeader.OptionalHeader;
	LOG_IF(optionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC, return false,
		Severity::Error, "Plugin file does not have a proper optional header '%'", plugin.fileName);

	if (optionalHeader.NumberOfRvaAndSizes <= IMAGE_DIRECTORY_ENTRY_COMHEADER)
	{
		plugin.language = PluginLanguage::Native;
	}
	else
	{
		IMAGE_DATA_DIRECTORY& clrDirectory = optionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER];
		plugin.language = clrDirectory.Size == 0 ? PluginLanguage::Native : PluginLanguage::Managed;
	}

	return true;
}

static b8
ValidatePluginDesc(PluginDesc& pluginDesc)
{
	b8 valid = true;
	valid = valid && pluginDesc.name.length > 0;
	valid = valid && pluginDesc.author.length > 0;
	valid = valid && pluginDesc.version;
	valid = valid && pluginDesc.lhmVersion == LHMVersion;
	return valid;
}

b8
PluginLoader_LoadSensorPlugin(PluginLoaderState& s, Plugin& plugin, SensorPlugin& sensorPlugin)
{
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	if (plugin.language != PluginLanguage::Builtin)
	{
		b8 success = DetectPluginLanguage(plugin);
		if (!success) return false;
	}

	PluginDesc pluginDesc = {};

	b8 success = false;
	switch (plugin.language)
	{
		default:
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Builtin:
		{
			sensorPlugin.functions.GetPluginInfo(pluginDesc, sensorPlugin.functions);
			success = true;
			break;
		}

		case PluginLanguage::Native:
		{
			success = false;

			// TODO: Will this work with delay loaded dependencies?
			success = SetDllDirectoryA(plugin.directory.data);
			LOG_IF(!success, break,
				Severity::Error, "Failed to set Sensor plugin DLL directory '%'", plugin.directory);

			plugin.loaderData = LoadLibraryA(plugin.fileName.data);
			LOG_IF(!plugin.loaderData, break,
				Severity::Error, "Failed to load unmanaged Sensor plugin '%'", plugin.fileName);

			success = SetDllDirectoryA("");
			LOG_IF(!success, break,
				Severity::Error, "Failed to unset Sensor plugin DLL directory '%'", plugin.directory);

			HMODULE pluginModule = (HMODULE) plugin.loaderData;
			sensorPlugin.functions.GetPluginInfo = (SensorPluginFunctions::GetPluginInfoFn*) (void*) GetProcAddress(pluginModule, "GetSensorPluginInfo");
			LOG_IF(!sensorPlugin.functions.GetPluginInfo, break,
				Severity::Error, "Failed to find unmanaged GetSensorPluginInfo '%'", plugin.fileName);

			sensorPlugin.functions.GetPluginInfo(pluginDesc, sensorPlugin.functions);

			success = true;
			break;
		}

		case PluginLanguage::Managed:
		{
			// NOTE: fuslogvw is great for debugging managed assembly loading.
			// TODO: Do we need to try/catch the managed code?
			success = (b8) s.lhmPluginLoader->LoadSensorPlugin(&plugin, &sensorPlugin, &pluginDesc);
			LOG_IF(!success, IGNORE,
				Severity::Error, "Failed to load managed Sensor plugin '%'", plugin.fileName);
			break;
		}
	}
	if (!success) return false;

	success = ValidatePluginDesc(pluginDesc);
	LOG_IF(!success, return false,
		Severity::Error, "Sensor plugin provided invalid description '%'", plugin.fileName);

	plugin.loadState       = PluginLoadState::Loaded;
	plugin.info.name       = String_FromView(pluginDesc.name);
	plugin.info.author     = String_FromView(pluginDesc.author);
	plugin.info.version    = pluginDesc.version;
	plugin.info.lhmVersion = pluginDesc.lhmVersion;

	pluginGuard.dismiss = true;
	return true;
}

b8
PluginLoader_UnloadSensorPlugin(PluginLoaderState& s, Plugin& plugin, SensorPlugin& sensorPlugin)
{
	b8 success = false;

	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	switch (plugin.language)
	{
		default:
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Builtin:
			success = true;
			break;

		case PluginLanguage::Native:
			success = FreeLibrary((HMODULE) plugin.loaderData);
			LOG_IF(!success, break,
				Severity::Error, "Failed to unload unmanaged Sensor plugin '%'", plugin.info.name);
			plugin.loaderData = nullptr;
			break;

		case PluginLanguage::Managed:
			success = (b8) s.lhmPluginLoader->UnloadSensorPlugin(&plugin, &sensorPlugin);
			LOG_IF(!success, IGNORE,
				Severity::Error, "Failed to unload managed Sensor plugin '%'", plugin.info.name);
			break;
	}
	if (!success) return false;

	pluginGuard.dismiss = true;
	plugin.loadState = PluginLoadState::Unloaded;
	return true;
}

b8
PluginLoader_LoadWidgetPlugin(PluginLoaderState& s, Plugin& plugin, WidgetPlugin& widgetPlugin)
{
	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	if (plugin.language != PluginLanguage::Builtin)
	{
		b8 success = DetectPluginLanguage(plugin);
		if (!success) return false;
	}

	PluginDesc pluginDesc = {};

	b8 success = false;
	switch (plugin.language)
	{
		default:
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Builtin:
		{
			widgetPlugin.functions.GetPluginInfo(pluginDesc, widgetPlugin.functions);
			success = true;
			break;
		}

		case PluginLanguage::Native:
		{
			success = false;

			// TODO: Will this work with delay loaded dependencies?
			success = SetDllDirectoryA(plugin.directory.data);
			LOG_IF(!success, break,
				Severity::Error, "Failed to set Widget plugin DLL directory '%'", plugin.directory);

			plugin.loaderData = LoadLibraryA(plugin.fileName.data);
			LOG_IF(!plugin.loaderData, break,
				Severity::Error, "Failed to load unmanaged Widget plugin '%'", plugin.fileName);

			success = SetDllDirectoryA("");
			LOG_IF(!success, break,
				Severity::Error, "Failed to set Widget plugin DLL directory '%'", plugin.directory);

			HMODULE pluginModule = (HMODULE) plugin.loaderData;
			widgetPlugin.functions.GetPluginInfo = (WidgetPluginFunctions::GetPluginInfoFn*) (void*) GetProcAddress(pluginModule, "GetWidgetPluginInfo");
			LOG_IF(!widgetPlugin.functions.GetPluginInfo, break,
				Severity::Error, "Failed to find unmanaged GetWidgetPluginInfo '%'", plugin.fileName);

			widgetPlugin.functions.GetPluginInfo(pluginDesc, widgetPlugin.functions);

			success = true;
			break;
		}

		case PluginLanguage::Managed:
		{
			// NOTE: fuslogvw is great for debugging managed assembly loading.
			// TODO: Do we need to try/catch the managed code?
			success = (b8) s.lhmPluginLoader->LoadWidgetPlugin(&plugin, &widgetPlugin, &pluginDesc);
			LOG_IF(!success, IGNORE,
				Severity::Error, "Failed to load managed Widget plugin '%'", plugin.fileName);
			break;
		}
	}
	if (!success) return false;

	success = ValidatePluginDesc(pluginDesc);
	LOG_IF(!success, return false,
		Severity::Error, "Widget plugin provided invalid description '%'", plugin.fileName);

	plugin.loadState       = PluginLoadState::Loaded;
	plugin.info.name       = String_FromView(pluginDesc.name);
	plugin.info.author     = String_FromView(pluginDesc.author);
	plugin.info.version    = pluginDesc.version;
	plugin.info.lhmVersion = pluginDesc.lhmVersion;

	pluginGuard.dismiss = true;
	return true;
}

b8
PluginLoader_UnloadWidgetPlugin(PluginLoaderState& s, Plugin& plugin, WidgetPlugin& widgetPlugin)
{
	b8 success = false;

	auto pluginGuard = guard { plugin.loadState = PluginLoadState::Broken; };

	switch (plugin.language)
	{
		default:
		case PluginLanguage::Null:
			success = false;
			break;

		case PluginLanguage::Builtin:
			success = true;
			break;

		case PluginLanguage::Native:
			success = FreeLibrary((HMODULE) plugin.loaderData);
			LOG_IF(!success, break,
				Severity::Error, "Failed to unload unmanaged Widget plugin '%'", plugin.info.name);
			plugin.loaderData = nullptr;
			break;

		case PluginLanguage::Managed:
			success = (b8) s.lhmPluginLoader->UnloadWidgetPlugin(&plugin, &widgetPlugin);
			LOG_IF(!success, IGNORE,
				Severity::Error, "Failed to unload managed Widget plugin '%'", plugin.info.name);
			break;
	}
	if (!success) return false;

	pluginGuard.dismiss = true;
	plugin.loadState = PluginLoadState::Unloaded;
	return true;
}

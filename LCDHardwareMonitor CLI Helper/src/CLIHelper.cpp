#pragma unmanaged
#define EXPORTING 1
#include "CLIHelper.h"
//#include "LHMDataSource.h"

#pragma managed

/* NOTE: This is in a separate project because it's going to get loaded into
 * the AppDomain for every plugin. This happens because we create a new domain
 * then run code defined in this file to lead the actual plugin assembly.
 */

/* TODO:
 *  - Do we support calling native functions in the plugin assembly? What about exported managed functions?
 */

#include <wtypes.h>
#include <msclr/gcroot.h>

using namespace msclr;
using namespace System;
using namespace System::Reflection;

struct CLIHelperState
{
	gcroot<AppDomain^> appDomain = nullptr;
	Plugin plugin;
};
CLIHelperState cliHelperState;

ref struct DomainProxy : MarshalByRefObject
{
	Plugin
	LoadAssembly(c16* assemblyDirectory, c16* assemblyName)
	{
		Plugin plugin = {};

		SetDllDirectoryW(assemblyDirectory);
		HMODULE module = LoadLibraryW(assemblyName);

		plugin.initialize = (InitializePtr) GetProcAddress(module, "ManagedInitialize");
		plugin.update     = (UpdatePtr)     GetProcAddress(module, "ManagedUpdate");
		plugin.teardown   = (TeardownPtr)   GetProcAddress(module, "ManagedTeardown");

		return plugin;
	}
};

Plugin
ManagedPlugin_LoadImpl(c16* assemblyDirectory, c16* assemblyName)
{
	String^ assemblyNameString = gcnew String(assemblyName);

	AppDomainSetup^ appDomainSetup = gcnew AppDomainSetup();
	appDomainSetup->ApplicationName    = assemblyNameString;
	appDomainSetup->PrivateBinPath     = gcnew String(assemblyDirectory);
	appDomainSetup->LoaderOptimization = LoaderOptimization::MultiDomainHost;

	auto appDomain = AppDomain::CreateDomain("Plugin AppDomain", nullptr, appDomainSetup);

	auto proxy = (DomainProxy^) appDomain->CreateInstanceAndUnwrap("LCDHardwareMonitor CLI Helper", nameof(DomainProxy));
	Plugin plugin = proxy->LoadAssembly(assemblyDirectory, assemblyName);

	cliHelperState.appDomain = appDomain;
	cliHelperState.plugin    = plugin;
	return plugin;
}

void
ManagedPlugin_UnloadImpl(Plugin plugin)
{
	if (plugin.initialize == cliHelperState.plugin.initialize)
	{
		AppDomain::Unload(cliHelperState.appDomain);
		cliHelperState = {};
	}
}

#pragma unmanaged
Plugin
_cdecl
ManagedPlugin_Load(c16* assemblyDirectory, c16* assemblyName)
{
	return ManagedPlugin_LoadImpl(assemblyDirectory, assemblyName);
}

void
_cdecl
ManagedPlugin_Unload(Plugin plugin)
{
	ManagedPlugin_UnloadImpl(plugin);
}
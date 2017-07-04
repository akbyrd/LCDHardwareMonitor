#pragma managed

#define EXPORTING 1
#include "CLIHelper.h"

#include <wtypes.h>

using namespace System;
using namespace System::Reflection;

value struct State
{
	static ResolveEventHandler^ resolveDelegate;
	static String^ pluginDirectory;
	static String^ pluginName;
};

Assembly^
AssemblyResolveHandler(Object^ sender, ResolveEventArgs^ args)
{
	//TODO: RequestingAssembly is null. Maybe because we're still loading the plugin?
	//AssemblyName^ loadingAssembly = args->RequestingAssembly->GetName();
	//if (loadingAssembly->Name == State::pluginName)
	{
		/* NOTE: Looks like we have to use LoadFrom here. Load caching causes a
		 * Load here to insta-fail. The docs also warn about recursive AssemblyResolve
		 * events, though that doesn't seem to happen in practice because of the
		 * caching. This doesn't feel very robust, but evenutally we'll be moving
		 * to separate AppDomains with appropriate bin paths anyway. Not worth
		 * spending more time on right now.
		 */
		AssemblyName^ dependencyAssembly = gcnew AssemblyName(args->Name);
		String^ path = String::Format("{0}\\{1}.dll", State::pluginDirectory, dependencyAssembly->Name);
		return Assembly::LoadFrom(path);
	}
	return nullptr;
}

void
PluginHelper_Initialize()
{
	State::resolveDelegate = gcnew ResolveEventHandler(AssemblyResolveHandler);
	AppDomain::CurrentDomain->AssemblyResolve += State::resolveDelegate;
}

void
PluginHelper_PluginLoaded(c16* pluginDirectory, c16* pluginName)
{
	State::pluginDirectory = gcnew String(pluginDirectory);
	State::pluginName = gcnew String(pluginName);
}

void
PluginHelper_PluginUnloaded(c16* pluginDirectory, c16* pluginName)
{
	if (State::pluginName == gcnew String(pluginName))
	{
		State::pluginDirectory = nullptr;
		State::pluginName = nullptr;
	}
}

void
PluginHelper_Teardown()
{
	//TODO: Unload the current AppDomain
	AppDomain::CurrentDomain->AssemblyResolve -= State::resolveDelegate;
	//TODO: Reset State
}
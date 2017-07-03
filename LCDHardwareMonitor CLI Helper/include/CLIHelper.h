#pragma once

#include "shared.hpp"

typedef void (*InitializePtr)();
typedef void (*UpdatePtr)();
typedef void (*TeardownPtr)();

//@TODO: Using field initializers causes C4190
struct Plugin
{
	InitializePtr initialize;
	UpdatePtr     update;
	TeardownPtr   teardown;
};

LHM_API Plugin ManagedPlugin_Load(c16* assemblyDirectory, c16* assemblyName);
LHM_API void   ManagedPlugin_UpdateAllPlugins();
LHM_API void   ManagedPlugin_Unload(Plugin plugin);
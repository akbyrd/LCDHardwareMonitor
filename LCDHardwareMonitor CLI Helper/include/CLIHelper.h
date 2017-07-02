#pragma once

#include "shared.hpp"

typedef void (_cdecl *InitializePtr)();
typedef void (_cdecl *UpdatePtr)();
typedef void (_cdecl *TeardownPtr)();

//@TODO: Using field initializers causes C4190
struct Plugin
{
	InitializePtr initialize;
	UpdatePtr     update;
	TeardownPtr   teardown;
};

LHM_API Plugin ManagedPlugin_Load(c16* assemblyDirectory, c16* assemblyName);
LHM_API void   ManagedPlugin_Unload(Plugin plugin);
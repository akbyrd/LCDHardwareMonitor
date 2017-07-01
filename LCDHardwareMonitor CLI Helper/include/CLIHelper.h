#pragma once

#include "shared.hpp"

typedef void (__stdcall *InitializePtr)();
typedef void (__stdcall *UpdatePtr)();
typedef void (__stdcall *TeardownPtr)();

struct Plugin
{
	InitializePtr initialize = nullptr;
	UpdatePtr     update     = nullptr;
	TeardownPtr   teardown   = nullptr;
};

LHM_API Plugin ManagedPlugin_Load(c16* assemblyDirectory, c16* assemblyName);
LHM_API void   ManagedPlugin_Unload(Plugin plugin);
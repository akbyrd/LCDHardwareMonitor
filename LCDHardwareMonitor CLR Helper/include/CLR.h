#pragma once

#include "LHMAPI.h"

LHM_API void _cdecl CLR_Initialize     ();
LHM_API void _cdecl CLR_PluginLoaded   (c16* pluginDirectory, c16* pluginName);
LHM_API void _cdecl CLR_PluginUnloaded (c16* pluginDirectory, c16* pluginName);
LHM_API void _cdecl CLR_Teardown       ();

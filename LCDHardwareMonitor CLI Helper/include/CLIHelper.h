#pragma once

#include "LHMAPI.h"

LHM_API void _cdecl PluginHelper_Initialize();
LHM_API void _cdecl PluginHelper_PluginLoaded(c16* pluginDirectory, c16* pluginName);
LHM_API void _cdecl PluginHelper_PluginUnloaded(c16* pluginDirectory, c16* pluginName);
LHM_API void _cdecl PluginHelper_Teardown();
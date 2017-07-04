#pragma once

#include "shared.hpp"

LHM_API void PluginHelper_Initialize();
LHM_API void PluginHelper_PluginLoaded(c16* pluginDirectory, c16* pluginName);
LHM_API void PluginHelper_PluginUnloaded(c16* pluginDirectory, c16* pluginName);
LHM_API void PluginHelper_Teardown();
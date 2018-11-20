#ifndef LHM_APICLR
#define LHM_APICLR

#pragma unmanaged
struct PluginContext {};

#include "LHMAPI.h"

#pragma managed
#pragma make_public(PluginContext)
#pragma make_public(PluginInfo)
#pragma make_public(SensorPluginAPI)
#pragma make_public(WidgetPluginAPI)
#pragma make_public(WidgetInstanceAPI)

#endif

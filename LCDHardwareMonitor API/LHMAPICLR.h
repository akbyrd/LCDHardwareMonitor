#ifndef LHM_APICLR
#define LHM_APICLR

#pragma unmanaged
public struct PluginContext {};

#include "LHMAPI.h"

#pragma managed
#pragma make_public(PluginHeader)
#pragma make_public(PluginContext)
#pragma make_public(SensorPlugin)
#pragma make_public(WidgetPlugin)

#endif

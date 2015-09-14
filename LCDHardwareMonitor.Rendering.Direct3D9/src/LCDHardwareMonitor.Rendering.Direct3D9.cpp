// LCDHardwareMonitor.Rendering.Direct3D9.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "LCDHardwareMonitor.Rendering.Direct3D9.h"


// This is an example of an exported variable
LHM_D3D9_API int nLCDHardwareMonitorRenderingDirect3D9=0;

// This is an example of an exported function.
LHM_D3D9_API int fnLCDHardwareMonitorRenderingDirect3D9(void)
{
	return 42;
}

// This is the constructor of a class that has been exported.
// see LCDHardwareMonitor.Rendering.Direct3D9.h for the class definition
CLCDHardwareMonitorRenderingDirect3D9::CLCDHardwareMonitorRenderingDirect3D9()
{
	return;
}

// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LCDHARDWAREMONITORRENDERINGDIRECT3D9_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LCDHARDWAREMONITORRENDERINGDIRECT3D9_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LHM_D3D9_EXPORTS
#define LHM_D3D9_API __declspec(dllexport)
#else
#define LHM_D3D9_API __declspec(dllimport)
#endif

// This class is exported from the LCDHardwareMonitor.Rendering.Direct3D9.dll
class LHM_D3D9_API CLCDHardwareMonitorRenderingDirect3D9
{
public:
	CLCDHardwareMonitorRenderingDirect3D9(void);
	// TODO: add your methods here.
};

extern LHM_D3D9_API int nLCDHardwareMonitorRenderingDirect3D9;

LHM_D3D9_API int fnLCDHardwareMonitorRenderingDirect3D9(void);

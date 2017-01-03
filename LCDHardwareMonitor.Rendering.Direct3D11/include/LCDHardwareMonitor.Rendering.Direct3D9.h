// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the LHM_D3D9_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// LHM_D3D9_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef LHM_D3D9_EXPORTS
#define LHM_D3D9_API __declspec(dllexport)
#else
#define LHM_D3D9_API __declspec(dllimport)
#endif

extern "C"
{
	LHM_D3D9_API bool Initialize();
}
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "Core.h"
#include "FileAPI.hpp"
#include "Renderer.hpp"
#include "LCDHardwareMonitor.Rendering.Direct3D9.h"

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	switch (ulReasonForCall)
	{
		case DLL_PROCESS_ATTACH:
		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

LHM_D3D9_API int
Render(void)
{
	return 42;
}
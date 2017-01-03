#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "Core.h"
#include "FileAPI.hpp"
#include "Renderer.hpp"
#include "LCDHardwareMonitor.Rendering.Direct3D9.h"

//TODO: Init and render
//TODO: Teardown
//TODO: Logging

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ulReasonForCall, LPVOID lpReserved)
{
	BOOL success;

	switch (ulReasonForCall)
	{
		//Microsoft recommended optimization
		//Disables DLL_THREAD_ATTACH and DLL_THREAD_DETACH
		//https://msdn.microsoft.com/en-us/library/ms682579(v=vs.85).aspx
		case DLL_PROCESS_ATTACH:
			success = DisableThreadLibraryCalls(hModule);
			if (!success)
			{
				LOG_LASTERROR();
				return FALSE;
			}
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	//NOTE: Can't use Direct3D or DXGI from DllMain
	//https://msdn.microsoft.com/en-us/library/windows/desktop/bb205075(v=vs.85).aspx#DXGI_Responses_From_DLLMain

	return TRUE;
}

LHM_D3D9_API bool
Initialize()
{
	bool success;

	//TODO: Real memory
	D3DRendererState rendererState = {};
	rendererState.renderSize = {320, 240};

	success = InitializeRenderer(&rendererState);
	if (!success) return false;

	return true;
}
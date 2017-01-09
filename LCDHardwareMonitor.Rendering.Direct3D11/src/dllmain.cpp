#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include "Core.h"
#include "FileAPI.hpp"
#include "Renderer.hpp"
#include "LCDHardwareMonitor.Rendering.Direct3D11.h"

//TODO: Logging

D3DRendererState rendererState;

BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD fdwReason, LPVOID lpvReserved)
{
	bool success;

	switch (fdwReason)
	{
		//Microsoft recommended optimization
		//Disables DLL_THREAD_ATTACH and DLL_THREAD_DETACH
		//https://msdn.microsoft.com/en-us/library/ms682579(v=vs.85).aspx
		case DLL_PROCESS_ATTACH:
			success = DisableThreadLibraryCalls(hInstance);
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

	return true;
}

LHM_D3D11_API bool
Initialize(u16 width, u16 height, IDirect3DSurface9** renderSurface)
{
	bool success;

	*renderSurface = nullptr;

	//TODO: Real memory
	rendererState = {};
	rendererState.renderSize = {width, height};

	success = InitializeRenderer(&rendererState);
	if (!success) return false;

	*renderSurface = rendererState.d3d9RenderSurface0.Get();

	return true;
}

LHM_D3D11_API bool
Render()
{
	bool success;

	success = Render(&rendererState);
	if (!success) return false;

	return true;
}

LHM_D3D11_API void
Teardown()
{
	TeardownRenderer(&rendererState);
}
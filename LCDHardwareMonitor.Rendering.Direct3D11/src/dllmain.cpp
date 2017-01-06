#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"

#include <d3d9.h>

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

D3DRendererState rendererState;

LHM_D3D9_API bool
Initialize()
{
	bool success;

	//TODO: Real memory
	rendererState = {};
	rendererState.renderSize = {320, 240};

	success = InitializeRenderer(&rendererState);
	if (!success) return false;

	return true;
}

LHM_D3D9_API bool
Render()
{
	bool success;

	success = Render(&rendererState);
	if (!success) return false;

	return true;
}

LHM_D3D9_API void
Teardown()
{
	TeardownRenderer(&rendererState);
}

LHM_D3D9_API ID3D11Resource*
GetD3D9RenderTexture()
{
	#if false
	return rendererState.d3dRenderTexture.Get();
	#elif false
	HRESULT hr;

	ID3D11Texture2D* renderTexture = rendererState.d3dRenderTexture.Get();
	IDirect3DBaseTexture9* d3d9RenderTexture;
	hr = rendererState.d3dDevice->OpenSharedResource(
		renderTexture,
		__uuidof(d3d9RenderTexture),
		(void**) &d3d9RenderTexture
	);
	if (LOG_HRESULT(hr)) return nullptr;

	return d3d9RenderTexture;
	#else
	//TODO: QueryInterface IDXGIResource from ID3D11Texture2D
	//TODO: GetSharedHandle from IDXGIResource
	//TODO: Init D3D9 device
	//TODO: CreateTexture IDirect3DTexture9
	//TODO: GetSurfaceLevel 0 (IDirect3DSurface9) from IDirect3DTexture9
	//TODO: SetBackBuffer IDirect3DSurface9
	return nullptr;
	#endif
}
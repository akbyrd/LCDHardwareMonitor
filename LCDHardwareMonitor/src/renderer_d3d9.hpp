#pragma comment(lib, "D3D9.lib")
#pragma comment(lib, "DXGI.lib")

// For some GUID magic in the DirectX/DXGI headers. Must be included before them.
#include <InitGuid.h>

#ifdef DEBUG
	#define D3D_DEBUG_INFO
#endif

#include <d3d9.h>

// TODO: To provide synchronization, use event queries or lock the texture.
// https://docs.microsoft.com/en-us/windows/desktop/direct3darticles/surface-sharing-between-windows-graphics-apis

b32
D3D9_Initialize(
	HWND hwnd,
	IDirect3D9Ex** d3d9,
	IDirect3DDevice9Ex** d3d9Device)
{
	HRESULT hr;

	hr = Direct3DCreate9Ex(D3D_SDK_VERSION, d3d9);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to initialize D3D9");

	D3DPRESENT_PARAMETERS presentParams = {};
	presentParams.Windowed             = true;
	presentParams.SwapEffect           = D3DSWAPEFFECT_DISCARD;
	presentParams.hDeviceWindow        = nullptr;
	presentParams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

	hr = (*d3d9)->CreateDeviceEx(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hwnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_NOWINDOWCHANGES,
		&presentParams,
		nullptr,
		d3d9Device
	);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to create D3D9 device");

	return true;
}

void
D3D9_Teardown(
	IDirect3D9Ex*       d3d9,
	IDirect3DDevice9Ex* d3d9Device,
	IDirect3DTexture9*  d3d9RenderTexture,
	IDirect3DSurface9*  d3d9RenderSurface0)
{
	d3d9              ->Release();
	d3d9Device        ->Release();
	d3d9RenderTexture ->Release();
	d3d9RenderSurface0->Release();
}

b32
D3D9_CreateSharedSurface(
	IDirect3DDevice9Ex* d3d9Device,
	IDirect3DTexture9** d3d9RenderTexture,
	IDirect3DSurface9** d3d9RenderSurface0,
	HANDLE              d3dRenderTextureSharedHandle,
	v2u                 renderSize)
{
	HRESULT hr;

	hr = d3d9Device->CreateTexture(
		renderSize.x,
		renderSize.y,
		1,
		D3DUSAGE_RENDERTARGET,
		D3DFMT_A8R8G8B8,
		D3DPOOL_DEFAULT,
		d3d9RenderTexture,
		&d3dRenderTextureSharedHandle
	);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to create D3D9 render texture");

	hr = (*d3d9RenderTexture)->GetSurfaceLevel(0, d3d9RenderSurface0);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to get D3D9 render surface");

	return true;
}

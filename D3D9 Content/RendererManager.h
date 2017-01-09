#pragma once

class Renderer
{
public:
	HRESULT Initialize();
	HRESULT Render();
	HRESULT Teardown();

	HWND                    m_hwnd                = nullptr;
	IDirect3D9Ex*           m_pD3DEx              = nullptr;
	IDirect3DDevice9Ex*     m_pd3dDeviceEx        = nullptr;
	IDirect3DSurface9*      m_pd3dRTS             = nullptr;

	ID3D11Device*           d3d11Device           = nullptr;
	ID3D11DeviceContext*    d3d11Context          = nullptr;
	ID3D11RenderTargetView* d3d11RenderTargetView = nullptr;

	IDirect3DSurface9*      d3d9RenderSurface0    = nullptr;


	UINT m_uWidth    = 320;
	UINT m_uHeight   = 240;
	bool m_fUseAlpha = true;
};
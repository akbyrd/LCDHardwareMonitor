#include "StdAfx.h"

const static TCHAR szAppName[] = TEXT("D3DImageSample");

HRESULT
Renderer::Initialize()
{
	HRESULT hr = S_OK;
	D3DPRESENT_PARAMETERS d3dpp = {};
	D3D11_TEXTURE2D_DESC renderTextureDesc = {};
	ComPtr<ID3D11Texture2D> d3d11RenderTexture;

	//Create window
	{
		WNDCLASS wndclass = {};
		wndclass.style         = CS_HREDRAW | CS_VREDRAW;
		wndclass.lpfnWndProc   = DefWindowProc;
		wndclass.cbClsExtra    = 0;
		wndclass.cbWndExtra    = 0;
		wndclass.hInstance     = NULL;
		wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
		wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
		wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
		wndclass.lpszMenuName  = NULL;
		wndclass.lpszClassName = szAppName;

		if (!RegisterClass(&wndclass))
		{
			IFC(E_FAIL);
		}

		m_hwnd = CreateWindow(
			szAppName,
			szAppName,
			WS_OVERLAPPEDWINDOW,
			0, 0,
			0, 0,
			NULL,
			NULL,
			NULL,
			NULL
		);
	}


	//Initialize Direct3D11
	{
		//Create device
		{
			D3D_FEATURE_LEVEL featureLevel = {};

			hr = D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_HARDWARE,
				nullptr,
				D3D11_CREATE_DEVICE_SINGLETHREADED,
				nullptr, 0, //NOTE: 11_1 will never be created by default
				D3D11_SDK_VERSION,
				&d3d11Device,
				&featureLevel,
				&d3d11Context
			);
			Assert(SUCCEEDED(hr));
		}


		//Create render texture
		{
			renderTextureDesc.Width              = m_uWidth;
			renderTextureDesc.Height             = m_uHeight;
			renderTextureDesc.MipLevels          = 1;
			renderTextureDesc.ArraySize          = 1;
			renderTextureDesc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
			renderTextureDesc.SampleDesc.Count   = 1;
			renderTextureDesc.SampleDesc.Quality = 0;
			renderTextureDesc.Usage              = D3D11_USAGE_DEFAULT;
			renderTextureDesc.BindFlags          = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			renderTextureDesc.CPUAccessFlags     = 0;
			renderTextureDesc.MiscFlags          = D3D11_RESOURCE_MISC_SHARED;

			hr = d3d11Device->CreateTexture2D(&renderTextureDesc, nullptr, &d3d11RenderTexture);
			Assert(SUCCEEDED(hr));

			hr = d3d11Device->CreateRenderTargetView(d3d11RenderTexture.Get(), nullptr, &d3d11RenderTargetView);
			Assert(SUCCEEDED(hr));

			//Initialize output merger
			d3d11Context->OMSetRenderTargets(1, &d3d11RenderTargetView, nullptr);
		}
	}


	//Initalize Direct3D9
	{
		IFC(Direct3DCreate9Ex(D3D_SDK_VERSION, &m_pD3DEx));

		d3dpp.Windowed         = TRUE;
		d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
		d3dpp.BackBufferHeight = 1;
		d3dpp.BackBufferWidth  = 1;
		d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
		/*
		UINT                BackBufferWidth;
		UINT                BackBufferHeight;
		D3DFORMAT           BackBufferFormat;
		UINT                BackBufferCount;

		D3DMULTISAMPLE_TYPE MultiSampleType;
		DWORD               MultiSampleQuality;

		D3DSWAPEFFECT       SwapEffect;
		HWND                hDeviceWindow;
		BOOL                Windowed;
		BOOL                EnableAutoDepthStencil;
		D3DFORMAT           AutoDepthStencilFormat;
		DWORD               Flags;

		//FullScreen_RefreshRateInHz must be zero for Windowed mode
		UINT                FullScreen_RefreshRateInHz;
		UINT                PresentationInterval;
		*/

		IFC(m_pD3DEx->CreateDeviceEx(
			D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			m_hwnd, //nullptr shouldn't be allowed, but it still works. Hmm
			D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE,
			&d3dpp,
			nullptr,
			&m_pd3dDeviceEx
		));

		#if false
		IFC(m_pd3dDeviceEx->CreateRenderTarget(
			m_uWidth,
			m_uHeight,
			m_fUseAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
			(D3DMULTISAMPLE_TYPE) 1,
			0,
			m_pd3dDeviceEx ? FALSE : TRUE,  // Lockable RT required for good XP perf
			&m_pd3dRTS,
			NULL
		));
		#else
		ComPtr<IDirect3DTexture9> d3d9RenderTexture;
		IFC(m_pd3dDeviceEx->CreateTexture(
			m_uWidth,
			m_uHeight,
			1,
			D3DUSAGE_RENDERTARGET,
			m_fUseAlpha ? D3DFMT_A8R8G8B8 : D3DFMT_X8R8G8B8,
			D3DPOOL_DEFAULT,
			&d3d9RenderTexture,
			nullptr
		));
		IFC(d3d9RenderTexture->GetSurfaceLevel(0, &m_pd3dRTS));
		#endif

		IFC(m_pd3dDeviceEx->SetRenderTarget(0, m_pd3dRTS));
	}


	//Share Direct3D11 back buffer
	{
		//Shared surface
		ComPtr<IDXGIResource> dxgiRenderTexture;
		hr = d3d11RenderTexture.As(&dxgiRenderTexture);
		Assert(SUCCEEDED(hr));

		HANDLE renderTextureSharedHandle;
		hr = dxgiRenderTexture->GetSharedHandle(&renderTextureSharedHandle);
		Assert(SUCCEEDED(hr));

		Assert(renderTextureDesc.Format == DXGI_FORMAT_B8G8R8A8_UNORM);

		ComPtr<IDirect3DTexture9> d3d9RenderTexture;
		hr = m_pd3dDeviceEx->CreateTexture(
			renderTextureDesc.Width,
			renderTextureDesc.Height,
			renderTextureDesc.MipLevels,
			D3DUSAGE_RENDERTARGET,
			D3DFMT_A8R8G8B8,
			D3DPOOL_DEFAULT,
			&d3d9RenderTexture,
			//TODO: Fix
			&renderTextureSharedHandle
			//nullptr
		);
		Assert(SUCCEEDED(hr));

		hr = d3d9RenderTexture->GetSurfaceLevel(0, &d3d9RenderSurface0);
		Assert(SUCCEEDED(hr));
	}

	Cleanup:
	return hr;
}

HRESULT
Renderer::Render()
{
	HRESULT hr = S_OK;

	d3d11Context->ClearRenderTargetView(d3d11RenderTargetView, DirectX::Colors::Red);

	//IFC(m_pd3dDeviceEx->BeginScene());
	IFC(m_pd3dDeviceEx->Clear(
		0,
		NULL,
		D3DCLEAR_TARGET,
		D3DCOLOR_ARGB(128, 0, 0, 128),  // NOTE: Premultiplied alpha!
		1.0f,
		0
	));
	//IFC(m_pd3dDeviceEx->EndScene());

	Cleanup:
	return hr;
}

HRESULT
Renderer::Teardown()
{
	SAFE_RELEASE(m_pd3dDeviceEx);
	SAFE_RELEASE(m_pd3dRTS);
	SAFE_RELEASE(m_pD3DEx);

	SAFE_RELEASE(d3d11RenderTargetView);
	SAFE_RELEASE(d3d11Context);
	SAFE_RELEASE(d3d11Device);

	if (m_hwnd)
	{
		DestroyWindow(m_hwnd);
		UnregisterClass(szAppName, NULL);
	}

	return S_OK;
}
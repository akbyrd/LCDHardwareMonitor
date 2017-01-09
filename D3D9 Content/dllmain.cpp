// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"

static Renderer *pManager = NULL;

extern "C" HRESULT WINAPI
Initialize()
{
	HRESULT hr = S_OK;

	pManager = new Renderer();
	IFCOOM(pManager);
	pManager->Initialize();

	Cleanup:
	return hr;
}

extern "C" HRESULT WINAPI
Render()
{
	HRESULT hr = S_OK;

	hr = pManager->Render();

	return hr;
}

extern "C" HRESULT WINAPI
Teardown()
{
	HRESULT hr = S_OK;

	hr = pManager->Teardown();

	return hr;
}

extern "C" HRESULT WINAPI
GetD3D9BackBuffer(IDirect3DSurface9 **ppSurface)
{
	*ppSurface = pManager->m_pd3dRTS;

	return S_OK;
}

extern "C" HRESULT WINAPI
GetD3D11BackBuffer(IDirect3DSurface9 **ppSurface)
{
	*ppSurface = pManager->d3d9RenderSurface0;

	return S_OK;
}
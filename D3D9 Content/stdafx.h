// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

#pragma comment(lib, "D3D9.lib")
#include <d3d9.h>
#include <DirectXMath.h>
using namespace DirectX;

#pragma comment(lib, "D3D11.lib")
#include <d3d11.h>
#include <DirectXColors.h>

#include <wrl\client.h>
using Microsoft::WRL::ComPtr;

#define Assert(condition) if (!(condition)) { __debugbreak(); *((int *) 0) = 0; }

#include "RendererManager.h"

#define IFC(x) { hr = (x); if (FAILED(hr)) goto Cleanup; }
#define IFCOOM(x) { if ((x) == NULL) { hr = E_OUTOFMEMORY; IFC(hr); } }
#define SAFE_RELEASE(x) { if (x) { x->Release(); x = NULL; } }
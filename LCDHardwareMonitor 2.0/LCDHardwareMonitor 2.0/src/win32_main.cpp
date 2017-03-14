#include "shared.hpp"
#include "math.hpp"
#include "platform.h"
#include "renderer.h"
#include "simulation.hpp"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "platform_win32.hpp"
#include "renderer_d3d11.hpp"

//TODO: Can an existing texture be associated with a swap chain?
//TODO: Fix the following so hr gets stringified
//      LOG_IF(FAILED(hr), L"", return false);
//TODO: Probably want to use this as some point so frame debugger works without the preview window
//      https://msdn.microsoft.com/en-us/library/hh780905.aspx

//TODO: Move to PreviewWindowState if it doesn't increase the struct size
const c16 PreviewWindowClass[] = L"LCDHardwareMonitor Preview Class";
const u32 WM_PREVIEWWINDOWCLOSED = WM_USER + 0;
const i32 togglePreviewWindowID = 0;

LRESULT CALLBACK
PreviewWndProc(HWND, u32, WPARAM, LPARAM);

b32 CreatePreviewWindow(PreviewWindowState*, D3DRendererState*, V2i, HINSTANCE, i32);
b32 DestroyPreviewWindow(PreviewWindowState*, D3DRendererState*, HINSTANCE);

i32 CALLBACK
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, i32 nCmdShow)
{
	SimulationState simulationState = {};

	//Renderer
	D3DRendererState rendererState = {};
	{
		b32 success = InitializeRenderer(&rendererState, simulationState.renderSize);
		if (!success) LOG(L"InitializeRenderer failed to initialize", Severity::Error);
	}


	//Misc
	PreviewWindowState previewState = {};
	{
		b32 success = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		if (!success) LOG_LAST_ERROR(L"SetPriorityClass failed", Severity::Warning);
	}


	//Debug
	{
		//TODO: Do this through a system tray icon and command line
		CreatePreviewWindow(&previewState, &rendererState, simulationState.renderSize, hInstance, nCmdShow);
		b32 success = RegisterHotKey(nullptr, togglePreviewWindowID, MOD_NOREPEAT, VK_F1);
		if (!success) LOG_LAST_ERROR(L"RegisterHotKey failed", Severity::Warning);
	}


	//Main loop
	int returnValue = -1;
	{
		b32 quit = false;
		while (!quit)
		{
			//Message loop
			MSG msg = {};
			while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessageW(&msg);

				switch (msg.message)
				{
					case WM_PREVIEWWINDOWCLOSED:
						DestroyPreviewWindow(&previewState, &rendererState, hInstance);
						break;

					case WM_HOTKEY:
					{
						if (msg.wParam == togglePreviewWindowID)
						{
							if (!previewState.hwnd)
							{
								CreatePreviewWindow(&previewState, &rendererState, simulationState.renderSize, hInstance, nCmdShow);
							}
							else
							{
								DestroyPreviewWindow(&previewState, &rendererState, hInstance);
							}
						}
						break;
					}

					case WM_QUIT:
					{
						quit = true;
						returnValue = (i32) msg.wParam;
						break;
					}
				}
				if (quit) break;
			}
			if (quit) break;


			//Simulate and render!
			b32 success = Render(&rendererState, &previewState);
			if (!success) LOG(L"Render failed", Severity::Error);

			Sleep(10);
		}
	}


	//Cleanup
	{
		//Leak all the things!
		//(Windows destroys everything automatically)

		return returnValue;
	}
}

b32
CreatePreviewWindow(PreviewWindowState* s, D3DRendererState* rendererState, V2i renderSize, HINSTANCE hInstance, i32 nCmdShow)
{
	//TODO: Eat beep when using alt enter
	//TODO: Snap resizing (add WS_THICKFRAME and handle the message)
	//TODO: Handle partial creation

	WNDCLASSW windowClass = {};
	windowClass.style         = 0;
	windowClass.lpfnWndProc   = PreviewWndProc;
	windowClass.cbClsExtra    = 0;
	windowClass.cbWndExtra    = sizeof(s);
	windowClass.hInstance     = hInstance;
	windowClass.hIcon         = nullptr;
	windowClass.hCursor       = nullptr;
	windowClass.hbrBackground = nullptr;
	windowClass.lpszMenuName  = nullptr;
	windowClass.lpszClassName = PreviewWindowClass;

	ATOM classAtom = RegisterClassW(&windowClass);
	LOG_LAST_ERROR_IF(classAtom == INVALID_ATOM, L"RegisterClass failed", Severity::Error, return false);

	s->hwnd = CreateWindowW(
		windowClass.lpszClassName,
		L"LCDHardwareMonitor Preview Window",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		//TODO: Center of screen
		CW_USEDEFAULT, CW_USEDEFAULT,
		renderSize.x,
		renderSize.y,
		nullptr,
		nullptr,
		hInstance,
		s
	);
	if (s->hwnd == INVALID_HANDLE_VALUE)
	{
		s->hwnd = nullptr;
		LOG_LAST_ERROR(L"CreateWindowEx failed", Severity::Error);
		return false;
	}

	b32 success = AttachPreviewWindow(rendererState, s);
	if (!success)
	{
		LOG(L"Failed to create a preview window", Severity::Error);
		DestroyPreviewWindow(s, rendererState, hInstance);
		return false;
	}

	ShowWindow(s->hwnd, nCmdShow);

	return true;
}

b32
DestroyPreviewWindow(PreviewWindowState* s, D3DRendererState* rendererState, HINSTANCE hInstance)
{
	b32 success;

	success = DestroyWindow(s->hwnd);
	LOG_LAST_ERROR_IF(!success, L"DestroyWindow failed", Severity::Error, return false);

	success = UnregisterClassW(PreviewWindowClass, hInstance);
	LOG_LAST_ERROR_IF(!success, L"UnregisterClass failed", Severity::Warning);

	*s = {};

	return true;
}

LRESULT CALLBACK
PreviewWndProc(HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
	auto state = (PreviewWindowState*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	switch (uMsg)
	{
		case WM_NCCREATE:
		{
			auto createStruct = (CREATESTRUCT*) lParam;
			state = (PreviewWindowState*) createStruct->lpCreateParams;

			//NOTE: Because Windows is dumb. See Return value section:
			//https://msdn.microsoft.com/en-us/library/windows/desktop/ms644898.aspx
			SetLastError(0);

			i64 iResult = SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM) state);
			if (iResult == 0 && GetLastError() != 0)
			{
				LOG_LAST_ERROR(L"SetWindowLongPtr failed", Severity::Error);
				return false;
			}
			break;
		}

		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				b32 success = PostMessageW(nullptr, WM_PREVIEWWINDOWCLOSED, 0, 0);
				LOG_LAST_ERROR_IF(!success, L"PostMessage failed", Severity::Warning);
				return 0;
			}
			break;
		}

		case WM_CLOSE:
		{
			b32 success = PostMessageW(nullptr, WM_PREVIEWWINDOWCLOSED, 0, 0);
			LOG_LAST_ERROR_IF(!success, L"PostMessage failed", Severity::Warning);
			return 0;
		}
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
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
//TODO: Research hosting a CLR instance in-process (CorBindToRuntime is deprecated).
/* TODO: Interop options so far:
 *  - Reverse P/Invoke (has to start from .NET delegate passed as callback, so this is only good if the “action” begins in your .NET code)
 *    https://blogs.msdn.microsoft.com/junfeng/2008/01/28/reverse-pinvoke-and-exception/
 *  - COM interop (every .NET class can also be a COM object, with or without explicit interfaces)
 *  - C++/CLI wrapper
 */

//TODO: Move to PreviewWindowState if it doesn't increase the struct size
const c16 PreviewWindowClass[] = L"LCDHardwareMonitor Preview Class";
const u32 WM_PREVIEWWINDOWCLOSED = WM_USER + 0;
const i32 togglePreviewWindowID = 0;

LRESULT CALLBACK
PreviewWndProc(HWND, u32, WPARAM, LPARAM);

b32 CreatePreviewWindow(PreviewWindowState*, D3DRendererState*, HINSTANCE, i32);
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
		CreatePreviewWindow(&previewState, &rendererState, hInstance, nCmdShow);
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
								CreatePreviewWindow(&previewState, &rendererState, hInstance, nCmdShow);
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
CreatePreviewWindow(PreviewWindowState* s, D3DRendererState* rendererState, HINSTANCE hInstance, i32 nCmdShow)
{
	//TODO: If the window didn't have focus the last time it was destroyed, it won't have focus when created
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

	auto windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	b32 success;
	RECT windowRect = { 0, 0, rendererState->renderSize.x, rendererState->renderSize.y };
	success = AdjustWindowRect(&windowRect, windowStyle, false);
	LOG_LAST_ERROR_IF(!success, L"AdjustWindowRect failed", Severity::Warning);

	s->renderSize = rendererState->renderSize;

	s->nonClientSize.x = (windowRect.right - windowRect.left) - s->renderSize.x;
	s->nonClientSize.y = (windowRect.bottom - windowRect.top) - s->renderSize.y;

	s->hwnd = CreateWindowW(
		windowClass.lpszClassName,
		L"LCDHardwareMonitor Preview Window",
		windowStyle,
		//TODO: Center of screen (or remember last position?)
		CW_USEDEFAULT, CW_USEDEFAULT,
		s->renderSize.x + s->nonClientSize.x,
		s->renderSize.y + s->nonClientSize.y,
		nullptr,
		nullptr,
		hInstance,
		s
	);
	if (s->hwnd == INVALID_HANDLE_VALUE)
	{
		LOG_LAST_ERROR(L"CreateWindowEx failed", Severity::Error);
		s->hwnd = nullptr;
		return false;
	}

	success = AttachPreviewWindow(rendererState, s);
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
	auto s = (PreviewWindowState*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);

	static POINT cursorStartPos;

	switch (uMsg)
	{
		case WM_NCCREATE:
		{
			auto createStruct = (CREATESTRUCT*) lParam;
			s = (PreviewWindowState*) createStruct->lpCreateParams;

			//NOTE: Because Windows is dumb. See Return value section:
			//https://msdn.microsoft.com/en-us/library/windows/desktop/ms644898.aspx
			SetLastError(0);

			i64 iResult = SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM) s);
			if (iResult == 0 && GetLastError() != 0)
			{
				LOG_LAST_ERROR(L"SetWindowLongPtr failed", Severity::Error);
				return false;
			}
			break;
		}

		case WM_MOUSEWHEEL:
		{
			//TODO: Is it worth trying to make things portable?
			s->mouseWheelAccumulator += GET_WHEEL_DELTA_WPARAM(wParam);
			i16 newZoomFactor = s->zoomFactor;
			while (s->mouseWheelAccumulator >= WHEEL_DELTA)
			{
				s->mouseWheelAccumulator -= WHEEL_DELTA;
				newZoomFactor++;
			}
			while (s->mouseWheelAccumulator <= -WHEEL_DELTA)
			{
				s->mouseWheelAccumulator += WHEEL_DELTA;
				newZoomFactor--;
			}
			if (newZoomFactor < 1) newZoomFactor = 1;

			if (newZoomFactor != s->zoomFactor)
			{
				RECT windowRect;
				b32 success = GetWindowRect(s->hwnd, &windowRect);
				LOG_LAST_ERROR_IF(!success, L"GetWindowRect failed", Severity::Warning, break);

				V2i windowCenter;
				windowCenter.x = (windowRect.right + windowRect.left) / 2;
				windowCenter.y = (windowRect.bottom + windowRect.top) / 2;

				V2i newClientSize = newZoomFactor * s->renderSize;

				RECT usableDesktopRect;
				success = SystemParametersInfoW(SPI_GETWORKAREA, 0, &usableDesktopRect, 0);
				LOG_LAST_ERROR_IF(!success, L"SystemParametersInfo failed", Severity::Warning, break);

				V2i usableDesktopSize;
				usableDesktopSize.x = usableDesktopRect.right - usableDesktopRect.left;
				usableDesktopSize.y = usableDesktopRect.bottom - usableDesktopRect.top;

				V2i newWindowSize = newClientSize + s->nonClientSize;
				if (newWindowSize.x > usableDesktopSize.x || newWindowSize.y > usableDesktopSize.y)
				{
					if (newZoomFactor > s->zoomFactor)
						break;
				}

				V2i newWindowTopLeft;
				newWindowTopLeft.x = windowCenter.x - (newClientSize.x / 2);
				newWindowTopLeft.y = windowCenter.y - (newClientSize.y / 2);

				if (newWindowTopLeft.x < 0) newWindowTopLeft.x = 0;
				if (newWindowTopLeft.y < 0) newWindowTopLeft.y = 0;

				success = SetWindowPos(
					s->hwnd,
					nullptr,
					newWindowTopLeft.x,
					newWindowTopLeft.y,
					newClientSize.x,
					newClientSize.y,
					0
				);
				LOG_LAST_ERROR_IF(!success, L"SetWindowPos failed", Severity::Warning, break);

				s->zoomFactor = newZoomFactor;
			}

			return 0;
		}

		case WM_KEYDOWN:
		{
			if (wParam == VK_ESCAPE)
			{
				if (IsDebuggerPresent())
				{
					PostQuitMessage(0);
					return 0;
				}
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
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

//TODO: Fix the following so hr gets stringified
//LOG_IF(FAILED(hr), L"", return false);

LRESULT CALLBACK
PreviewWndProc(HWND, u32, WPARAM, LPARAM);

b32
CreatePreviewWindow(PreviewWindowState*, D3DRendererState*, V2i, HINSTANCE, i32);

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

		CreatePreviewWindow(&previewState, &rendererState, simulationState.renderSize, hInstance, nCmdShow);
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
					case WM_KEYDOWN:
					{
						if (msg.wParam == VK_ESCAPE)
							PostQuitMessage(0);
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
	//TODO: Handle partial creation
	//TODO: Handle window being closed
	//TODO: Key to open and close window

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
	windowClass.lpszClassName = L"LCDHardwareMonitor Preview Class";

	ATOM classAtom = RegisterClassW(&windowClass);
	if (classAtom == INVALID_ATOM)
	{
		LOG_LAST_ERROR(L"RegisterClassW failed", Severity::Error);
		return false;
	}

	s->hwnd = CreateWindowExW(
		WS_EX_TOPMOST | WS_EX_TRANSPARENT,
		windowClass.lpszClassName,
		L"LCDHardwareMonitor Preview Window",
		WS_OVERLAPPEDWINDOW,
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
		LOG_LAST_ERROR(L"CreateWindowExW failed", Severity::Error);
		return false;
	}

	b32 success = AttachPreviewWindow(rendererState, s);
	LOG_IF(!success, L"Failed to create a preview window", return false);

	ShowWindow(s->hwnd, nCmdShow);

	return true;
}

b32 DestroyPreviewWindow()
{
	return false;
}

LRESULT CALLBACK
PreviewWndProc(HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
	auto state = (PreviewWindowState*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);


	switch (uMsg)
	{
		case WM_NCCREATE:
		{
			//TODO: Getting this message twice

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
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
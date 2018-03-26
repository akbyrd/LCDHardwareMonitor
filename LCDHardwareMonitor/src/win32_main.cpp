#include "LHMAPI.h"
#include "LHMString.h"

//Named varargs sure would be nice...
#define IF(expression, ...) \
if (expression)             \
{                           \
	__VA_ARGS__;            \
}

/* NOTE: Raise a compiler error when switching over
 * an enum and any enum values are missing a case.
 * https://msdn.microsoft.com/en-us/library/fdt9w8tf.aspx
 */
#pragma warning (error: 4062)

#include "math.hpp"
#include "CLIHelper.h"
#include "platform.h"
#include "renderer.h"
#include "widget_filledbar.hpp"
#include "simulation.hpp"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "platform_win32.hpp"
#include "renderer_d3d11.hpp"

//TODO: Remove winow clamping
//TODO: Think about having a fixed size for plugins, widgets, etc
//TODO: Support x86 and x64
//TODO: Reduce link dependencies
/* TODO: Fix the following so hr gets stringified
 * LOG_IF(FAILED(hr), L"", return false); */
/* TODO: Probably want to use this as some point so frame debugger works without
 * the preview window
 * https://msdn.microsoft.com/en-us/library/hh780905.aspx */
//TODO: Decide what we're really doing with wide characters/i18n

/* NOTE: Plugins
 * Ok, here's (part of) the deal. Managed plugins bring in a lot of complexity.
 * Ultimately, I'm leaning toward Option 3. I don't like having to call into
 * managed code, so maybe there's a way around that. However, for now I'm
 * sticking with the simplicity of Option 1. Once we're up and running I'll
 * revisit the issue.
 *
 * Option 1: Call managed plugins through GetProcAddress.
 *  - This will always call into the default AppDomain. And the Fusion Loader
 *    will fail to find other managed dependencies (that aren't in the GAC)
 *    because they aren't in the bin path. We'll need to use AssemblyResolve to
 *    handle this. Will need a CLI Helper for this.
 * *- The catch here is that assemblies are loaded on-demand. So we can't know
 *    which folder to search in based on *when* the load occurs. However, we can
 *    make a reasonable guess at which folder to search based on the name of the
 *    requesting assembly. We'll need to keep a cache of loads that have occured
 *    because Assembly A and load B which then loads C and we need to know to
 *    use A's folder.
 *  - It's very appealing to be able to simply call every plugin through a
 *    uniform C interface. It means we don't have to care whether a plugin is
 *    managed or unmanaged.
 *  - However, it also leaves the onus of doing good interop on the plugin
 *    author (e.g. using a delegate to skirt the x64 performance hit and an
 *    instance method for a bit more speed). Can maybe provide a 'template' or
 *    some such to simplify it.
 *  - Enabling or disabling plugins would require unloading and reloading the
 *    entire CLR. This doesn't look possible.
 *  - Plugin dependencies could end up loading out of some other folder in the
 *    bin path. Probably not a big issue. Plugin authors can use strong naming.
 *  - ~NOTE~ Specifying the full path won't work for indirectly loaded
 *    assemblies (e.g. dependencies).
 *  - ~NOTE~ Could use a .config file for resolving dependencies, but
 *    AssemblyResolve is simpler.
 *
 * Option 2: Load plugins into a separate AppDomain
 *  - Means we can load and unload plugins independently of one another.
 *  - We'll have to call into the managed side of each plugin. If we call into
 *    native and the plugin calls into managed it's going to use the default
 *    AppDomain.
 *  - We take a 10x performance hit for calling across AppDomains.
 *  - I think we can return a function pointer back to the native side to hide
 *    the 'have to call into managed code' issue from native side.
 *
 * Option 3: Host the CLR from native
 *  - I haven't looked into this heavily, but currently it looks like it might
 *    be the right choice.
 *  - Basically it's Option 2 without a separate C++/CLI assembly. It should
 *    streamline things a bit.
 *  - What is perfomrmance like in this case?
 */

//TODO: Move
const c16 previewWindowClass[] = L"LCDHardwareMonitor Preview Class";
const u32 WM_PREVIEWWINDOWCLOSED = WM_USER + 0;
const i32 togglePreviewWindowID = 0;

b32 CreatePreviewWindow(PreviewWindowState*, RendererState*, HINSTANCE);
b32 DestroyPreviewWindow(PreviewWindowState*, RendererState*, HINSTANCE);

i32 CALLBACK
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, c16* pCmdLine, i32 nCmdShow)
{
	//TODO: Deal with goto's skipping variable initialization
	int returnValue = -1;
	SimulationState    simulationState = {};
	PlatformState      platformState   = {};
	RendererState      rendererState   = {};
	PreviewWindowState previewState    = {};

	//Platform
	InitializePlatform(&platformState);


	//Renderer
	{
		b32 success = InitializeRenderer(&rendererState, simulationState.renderSize);
		LOG_IF(!success, L"InitializeRenderer failed to initialize", Severity::Error, goto Cleanup);
	}


	//Simulation
	{
		simulationState.platform = &platformState;
		simulationState.renderer = &rendererState;
		b32 success = Simulation_Initialize(&simulationState);
		LOG_IF(!success, L"Failed to initialize simulation", Severity::Error, goto Cleanup);
	}


	//Misc
	{
		b32 success = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		LOG_LAST_ERROR_IF(!success, L"SetPriorityClass failed", Severity::Warning);
	}


	//Debug
	{
		//TODO: Do this through a system tray icon and command line
		CreatePreviewWindow(&previewState, &rendererState, hInstance);
		b32 success = RegisterHotKey(nullptr, togglePreviewWindowID, MOD_NOREPEAT, VK_F1);
		LOG_LAST_ERROR_IF(!success, L"RegisterHotKey failed", Severity::Warning);
	}


	//Main loop
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
								CreatePreviewWindow(&previewState, &rendererState, hInstance);
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


			//Simulate
			Simulation_Update(&simulationState);

			//Render
			b32 success = Render(&rendererState, &previewState);
			LOG_IF(!success, L"Render failed", Severity::Error);

			Sleep(10);
		}
	}


	Cleanup:
	{
		PluginHelper_Teardown();

		//Leak all the things!
		//(Windows destroys everything automatically)

		return returnValue;
	}
}

b32
CreatePreviewWindow(PreviewWindowState* s, RendererState* rendererState, HINSTANCE hInstance)
{
	//TODO: Handle partial creation

	LRESULT CALLBACK
	PreviewWndProc(HWND, u32, WPARAM, LPARAM);

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
	windowClass.lpszClassName = previewWindowClass;

	ATOM classAtom = RegisterClassW(&windowClass);
	LOG_LAST_ERROR_IF(classAtom == INVALID_ATOM, L"RegisterClass failed", Severity::Error, return false);

	auto windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

	b32 success;
	RECT windowRect = { 0, 0, rendererState->renderSize.x, rendererState->renderSize.y };
	success = AdjustWindowRect(&windowRect, windowStyle, false);
	LOG_LAST_ERROR_IF(!success, L"AdjustWindowRect failed", Severity::Warning);

	s->renderSize = rendererState->renderSize;

	s->nonClientSize.x = (windowRect.right - windowRect.left) - s->renderSize.x;
	s->nonClientSize.y = (windowRect.bottom - windowRect.top) - s->renderSize.y;

	s->hwnd = CreateWindowW(
		windowClass.lpszClassName,
		L"LHM Preview",
		windowStyle,
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

	success = SetForegroundWindow(s->hwnd);
	LOG_LAST_ERROR_IF(!success, L"SetForegroundWindow failed", Severity::Warning);

	return true;
}

b32
DestroyPreviewWindow(PreviewWindowState* s, RendererState* rendererState, HINSTANCE hInstance)
{
	b32 success;

	success = DestroyWindow(s->hwnd);
	LOG_LAST_ERROR_IF(!success, L"DestroyWindow failed", Severity::Error, return false);

	success = UnregisterClassW(previewWindowClass, hInstance);
	LOG_LAST_ERROR_IF(!success, L"UnregisterClass failed", Severity::Warning);

	*s = {};

	return true;
}

LRESULT CALLBACK
PreviewWndProc(HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
	auto s = (PreviewWindowState*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);


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
			//TODO: Fix mouse cursor
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
				b32 success;

				RECT windowRect;
				success = GetWindowRect(s->hwnd, &windowRect);
				LOG_LAST_ERROR_IF(!success, L"GetWindowRect failed", Severity::Warning, return 0);

				V2i windowCenter;
				windowCenter.x = (windowRect.right + windowRect.left) / 2;
				windowCenter.y = (windowRect.bottom + windowRect.top) / 2;

				V2i newClientSize = newZoomFactor * s->renderSize;

				RECT usableDesktopRect;
				success = SystemParametersInfoW(SPI_GETWORKAREA, 0, &usableDesktopRect, 0);
				LOG_LAST_ERROR_IF(!success, L"SystemParametersInfo failed", Severity::Warning, return 0);

				V2i usableDesktopSize;
				usableDesktopSize.x = usableDesktopRect.right - usableDesktopRect.left;
				usableDesktopSize.y = usableDesktopRect.bottom - usableDesktopRect.top;

				V2i newWindowSize = newClientSize + s->nonClientSize;
				if (newWindowSize.x > usableDesktopSize.x || newWindowSize.y > usableDesktopSize.y)
				{
					if (newZoomFactor > s->zoomFactor)
						break;
				}

				//TODO: Try expanding from top left only
				//TODO: Maybe account for the shadow? (window - client - border)
				// or DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &extendedRect, sizeof(extendedRect));
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
				LOG_LAST_ERROR_IF(!success, L"SetWindowPos failed", Severity::Warning, return 0);

				s->zoomFactor = newZoomFactor;

				success = SetForegroundWindow(s->hwnd);
				LOG_LAST_ERROR_IF(!success, L"SetForegroundWindow failed", Severity::Warning);
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

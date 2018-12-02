#include "LHMAPI.h"
#include "LHMString.hpp"

#include "platform.h"
#include "pluginloader.h"
#include "renderer.h"
#include "plugin_shared.h"
#include "simulation.hpp"

// Fuck you, Microsoft
#pragma push_macro("IGNORE")
#undef IGNORE
#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#pragma pop_macro("IGNORE")

#include "platform_win32.hpp"
#include "pluginloader_win32.hpp"
#include "renderer_d3d11.hpp"
#include "previewwindow_win32_d3d11.hpp"

// TODO: Remove window clamping
// TODO: Support x86 and x64
// TODO: Reduce link dependencies

static const i32 togglePreviewWindowID = 0;

i32 CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, c8* pCmdLine, i32 nCmdShow)
{
	UNUSED(hPrevInstance); UNUSED(pCmdLine); UNUSED(nCmdShow);

	// NOTE: Normally, we can skip the majority of teardown code. Windows will
	// reclaim resources so there's no real point in us wasting time on it and
	// delaying the application from closing. We only enable teardown occasionally
	// to check for bugs in e.g. D3D resource handling.
	#define DO_TEARDOWN true
	#if DO_TEARDOWN
		#define DEFER_TEARDOWN defer
	#else
		#define DEFER_TEARDOWN [&]
	#endif

	b32 success;
	RendererState      rendererState     = {};
	SimulationState    simulationState   = {};
	PluginLoaderState  pluginLoaderState = {};
	PreviewWindowState previewState      = {};


	// Renderer
	success = Renderer_Initialize(&rendererState, simulationState.renderSize);
	LOG_IF(!success, return -1, Severity::Fatal, "Failed to initialize the renderer");
	DEFER_TEARDOWN { Renderer_Teardown(&rendererState); };

	success = Renderer_CreateSharedD3D9RenderTexture(&rendererState);
	LOG_IF(!success, return -1, Severity::Fatal, "Failed to create a shared render texture");
	DEFER_TEARDOWN { Renderer_DestroySharedD3D9RenderTarget(&rendererState); };


	// Simulation
	success = Simulation_Initialize(&simulationState, &pluginLoaderState, &rendererState);
	LOG_IF(!success, return -1, Severity::Fatal, "Failed to initialize the simulation");
	DEFER_TEARDOWN { Simulation_Teardown(&simulationState); };


	// Misc
	success = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to set process priority");


	// Debug
	PreviewWindow_Initialize(&previewState, &rendererState, hInstance);
	auto previewGuard = guard { PreviewWindow_Teardown(&previewState, &rendererState, hInstance); };
	success = RegisterHotKey(nullptr, togglePreviewWindowID, MOD_NOREPEAT, VK_F1);
	LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to register hotkeys");


	// Main loop
	while (true)
	{
		// Message loop
		MSG msg = {};
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			switch (msg.message)
			{
				case WM_PREVIEWWINDOWCLOSED:
					PreviewWindow_Teardown(&previewState, &rendererState, hInstance);
					break;

				case WM_HOTKEY:
				{
					if (msg.wParam == togglePreviewWindowID)
					{
						if (!previewState.hwnd)
						{
							PreviewWindow_Initialize(&previewState, &rendererState, hInstance);
							previewGuard.dismiss = false;
						}
						else
						{
							PreviewWindow_Teardown(&previewState, &rendererState, hInstance);
							previewGuard.dismiss = true;
						}
					}
					break;
				}

				case WM_QUIT:
					return (i32) msg.wParam;
			}
		}


		// Tick
		Simulation_Update(&simulationState);
		Renderer_Render(&rendererState);
		PreviewWindow_Render(&previewState, &rendererState);

		Sleep(10);
	}

	return 0;
}

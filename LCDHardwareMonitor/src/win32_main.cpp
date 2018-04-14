#include "LHMAPI.h"
#include "LHMString.h"

#include "platformdefs_win32.hpp"
#include "platform.h"
#include "math.hpp"
#include "pluginloader.h"
#include "renderer.h"
#include "widget_filledbar.hpp"
#include "simulation.hpp"

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "CLR.h"
#include "platform_win32.hpp"
#include "pluginloader_win32.hpp"
#include "renderer_d3d11.hpp"
#include "previewwindow_win32_d3d11.hpp"

//TODO: Remove window clamping
//TODO: Think about having a fixed size for plugins, widgets, etc
//TODO: Support x86 and x64
//TODO: Reduce link dependencies
/* TODO: Probably want to use this as some point so frame debugger works without
 * the preview window
 * https://msdn.microsoft.com/en-us/library/hh780905.aspx */

const i32 togglePreviewWindowID = 0;

i32 CALLBACK
wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, c16* pCmdLine, i32 nCmdShow)
{
	//TODO: Deal with goto's skipping variable initialization
	int returnValue = -1;
	SimulationState    simulationState   = {};
	PluginLoaderState  pluginLoaderState = {};
	RendererState      rendererState     = {};
	PreviewWindowState previewState      = {};


	//Renderer
	{
		b32 success;

		success = Renderer_Initialize(&rendererState, simulationState.renderSize);
		if (!success) goto Cleanup;

		success = Renderer_CreateSharedD3D9RenderTexture(&rendererState);
		if (!success) goto Cleanup;
	}


	//Simulation
	{
		b32 success = Simulation_Initialize(&simulationState, &pluginLoaderState, &rendererState);
		LOG_IF(!success, L"Failed to initialize simulation", Severity::Error, goto Cleanup);
	}


	//Misc
	{
		b32 success = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
		LOG_LAST_ERROR_IF(!success, L"SetPriorityClass failed", Severity::Warning);
	}


	//Debug
	{
		PreviewWindow_Initialize(&previewState, &rendererState, hInstance);
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
						PreviewWindow_Teardown(&previewState, &rendererState, hInstance);
						break;

					case WM_HOTKEY:
					{
						if (msg.wParam == togglePreviewWindowID)
						{
							if (!previewState.hwnd)
							{
								PreviewWindow_Initialize(&previewState, &rendererState, hInstance);
							}
							else
							{
								PreviewWindow_Teardown(&previewState, &rendererState, hInstance);
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
			b32 success = Renderer_Render(&rendererState);
			LOG_IF(!success, L"Render failed", Severity::Error);

			PreviewWindow_Render(&previewState, &rendererState);

			Sleep(10);
		}
	}


	Cleanup:
	{
		Simulation_Teardown(&simulationState);
		//NOTE: Only tearing these things down so we can check for D3D leaks.
		PreviewWindow_Teardown(&previewState, &rendererState, hInstance);
		Renderer_DestroySharedD3D9RenderTarget(&rendererState);
		Renderer_Teardown(&rendererState);

		//Leak most of the things!
		//(Windows destroys everything automatically)

		return returnValue;
	}
}

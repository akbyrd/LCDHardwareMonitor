#include "LHMAPI.h"

#include <stdio.h>

#include "platform.h"
#include "pluginloader.h"
#include "renderer.h"
#include "plugin_shared.h"
#include "gui_protocol.hpp"
#include "Solid Colored.ps.h"
#include "Outline.ps.h"
#include "ft232h.h"
#include "ili9341.hpp"
#include "simulation.hpp"

#include "platform_win32.hpp"
#include "ft232h_win32.hpp"
#include "pluginloader_win32.hpp"
#include "renderer_d3d11.hpp"
#include "previewwindow_win32_d3d11.hpp"

// TODO: Need a proper shutdown implementation (clean vs error)
// TODO: Need to handle multiple instance more gracefully

static const i32 togglePreviewWindowID = 0;

struct MessagePumpContext
{
	MSG*  msg;
	void* mainFiber;
};

i32 CALLBACK
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, c8* pCmdLine, i32 nCmdShow)
{
	// TODO: The CLR will throw structured exceptions when a debugger is
	// attached. Not catching these will silently crash the program. D3D will
	// throw structured exceptions when bad API calls are made. Catching these
	// ruins the ability to inspect the stack. So we have a bit of a catch 22.
	//__try
	{
		i32 WinMainImpl(HINSTANCE hInstance, HINSTANCE hPrevInstance, c8* pCmdLine, i32 nCmdShow);
		return WinMainImpl(hInstance, hPrevInstance, pCmdLine, nCmdShow);
	}
	//__except (true)
	//{
	//	LOG(Severity::Fatal, "A structured exception has occurred");
	//	return (i32) GetExceptionCode();
	//}
}

i32
WinMainImpl(HINSTANCE hInstance, HINSTANCE hPrevInstance, c8* pCmdLine, i32 nCmdShow)
{
	Unused(hPrevInstance, pCmdLine, nCmdShow);

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

	// TODO: Do plugin loader, ft232h, and ili9341 belong in the simulation?
	FT232HState        ft232hState       = {};
	ILI9341State       ili9341State      = {};
	RendererState      rendererState     = {};
	SimulationState    simulationState   = {};
	PluginLoaderState  pluginLoaderState = {};
	PreviewWindowState previewState      = {};


	// Renderer
	b8 success = Renderer_Initialize(rendererState);
	LOG_IF(!success, return -1, Severity::Fatal, "Failed to initialize the renderer");
	DEFER_TEARDOWN { Renderer_Teardown(rendererState); };


	// Simulation
	success = Simulation_Initialize(simulationState, pluginLoaderState, rendererState, ft232hState, ili9341State);
	LOG_IF(!success, return -1, Severity::Fatal, "Failed to initialize the simulation");
	DEFER_TEARDOWN { Simulation_Teardown(simulationState); };


	// Misc
	// TODO: Set a core affinity
	success = SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
	LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to set process priority");


	// Debug
	auto previewGuard = guard { PreviewWindow_Teardown(previewState); };
	#if true
	PreviewWindow_Initialize(previewState, simulationState, rendererState, hInstance, nullptr);
	#else
	previewGuard.dismiss = true;
	#endif

	success = RegisterHotKey(nullptr, togglePreviewWindowID, MOD_NOREPEAT, VK_F1);
	LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to register hotkeys");


	// Fibers
	void* mainFiber = ConvertThreadToFiber(nullptr);
	LOG_LAST_ERROR_IF(!mainFiber, return -1, Severity::Warning, "Failed to convert main thread to a fiber");
	previewState.mainFiber = mainFiber;

	MessagePumpContext msgPumpContext = {};
	msgPumpContext.mainFiber = previewState.mainFiber;

	void CALLBACK MessagePump(MessagePumpContext*) noexcept;
	void* messageFiber = CreateFiber(
		0,
		(LPFIBER_START_ROUTINE) MessagePump,
		&msgPumpContext
	);
	LOG_LAST_ERROR_IF(!messageFiber, return -1, Severity::Warning, "Failed to create message fiber");


	// Main loop
	for(;;)
	{
		// Pump messages
		SwitchToFiber(messageFiber);
		while (msgPumpContext.msg)
		{
			MSG& msg = *msgPumpContext.msg;
			switch (msg.message)
			{
				case WM_PREVIEWWINDOWCLOSED:
					PreviewWindow_Teardown(previewState);
					previewGuard.dismiss = true;
					break;

				case WM_HOTKEY:
				{
					if (msg.wParam == togglePreviewWindowID)
					{
						if (!previewState.hwnd)
						{
							PreviewWindow_Initialize(previewState, simulationState, rendererState, hInstance, mainFiber);
							previewGuard.dismiss = false;
						}
						else
						{
							PreviewWindow_Teardown(previewState);
							previewGuard.dismiss = true;
						}
					}
					break;
				}

				case WM_QUIT:
					return (i32) msg.wParam;
			}
			SwitchToFiber(messageFiber);
		}

		// Tick
		Simulation_Update(simulationState);

		// BUG: Looks like it's possible to get WM_PREVIEWWINDOWCLOSED without WM_QUIT
		PreviewWindow_Render(previewState);

		// TODO: Probably not a good idea when using fibers
		// BUG: This seems to lock up and causes VS to crash
		//Sleep(10);
	}

	return 0;
}

void CALLBACK
MessagePump(MessagePumpContext* context) noexcept
{
	for (;;)
	{
		MSG msg = {};
		while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);

			context->msg = &msg;
			SwitchToFiber(context->mainFiber);
			context->msg = nullptr;
		}
		SwitchToFiber(context->mainFiber);
	}
}

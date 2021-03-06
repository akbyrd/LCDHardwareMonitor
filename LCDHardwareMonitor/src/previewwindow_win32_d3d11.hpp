#include <Windowsx.h>

const c8  previewWindowClass[] = "LCDHardwareMonitor Preview Class";
const u32 WM_PREVIEWWINDOWCLOSED = WM_USER + 0;

struct PreviewWindowState
{
	HWND                    hwnd;
	void*                   mainFiber;
	HINSTANCE               hInstance;
	ComPtr<IDXGISwapChain>  swapChain;
	ComPtr<ID3D11Texture2D> backBuffer;
	v2u                     renderSize;
	v2u                     nonClientSize;
	v2i                     mousePos;
	u16                     zoomFactor;
	i16                     mouseWheelAccumulator;
	SimulationState*        simulationState;
	RendererState*          rendererState;
	i8                      mouseCaptureCount;
};

b8
PreviewWindow_Initialize(
	PreviewWindowState& s,
	SimulationState&    simulationState,
	RendererState&      rendererState,
	HINSTANCE           hInstance,
	void*               mainFiber)
{
	s.mainFiber       = mainFiber;
	s.hInstance       = hInstance;
	s.zoomFactor      = 1;
	s.simulationState = &simulationState;
	s.rendererState   = &rendererState;

	// Create Window
	{
		LRESULT CALLBACK
		PreviewWndProc(HWND, u32, WPARAM, LPARAM);

		WNDCLASSA windowClass = {};
		windowClass.style         = 0;
		windowClass.lpfnWndProc   = PreviewWndProc;
		windowClass.cbClsExtra    = 0;
		windowClass.cbWndExtra    = sizeof(s);
		windowClass.hInstance     = hInstance;
		windowClass.hIcon         = nullptr;
		windowClass.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName  = nullptr;
		windowClass.lpszClassName = previewWindowClass;

		ATOM classAtom = RegisterClassA(&windowClass);
		LOG_LAST_ERROR_IF(classAtom == INVALID_ATOM, return false,
			Severity::Error, "Failed to register preview window class");

		u32 windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

		RECT windowRect = { 0, 0, (i32) rendererState.renderSize.x, (i32) rendererState.renderSize.y };
		b8 success = AdjustWindowRect(&windowRect, (u32) windowStyle, false);
		LOG_LAST_ERROR_IF(!success, IGNORE,
			Severity::Error, "Failed to set preview window size or style");

		s.renderSize = rendererState.renderSize;

		s.nonClientSize.x = (windowRect.right - windowRect.left) - s.renderSize.x;
		s.nonClientSize.y = (windowRect.bottom - windowRect.top) - s.renderSize.y;

		s.hwnd = CreateWindowA(
			windowClass.lpszClassName,
			"LHM Preview",
			(u32) windowStyle,
			CW_USEDEFAULT, CW_USEDEFAULT,
			(i32) (s.renderSize.x + s.nonClientSize.x),
			(i32) (s.renderSize.y + s.nonClientSize.y),
			nullptr,
			nullptr,
			hInstance,
			&s
		);
		if (s.hwnd == INVALID_HANDLE_VALUE)
		{
			LOG_LAST_ERROR(Severity::Error, "Failed to create preview window");
			s.hwnd = nullptr;
			return false;
		}

		success = SetForegroundWindow(s.hwnd);
		LOG_LAST_ERROR_IF(!success, IGNORE,
			Severity::Warning, "Failed to bring preview window to the foreground");
	}


	// Attach Renderer
	{
		HRESULT hr;

		// TODO: When using fullscreen, the display mode should be chosen by
		// enumerating supported modes. If a mode is chosen that isn't supported,
		// a performance penalty will be incurred due to Present performing a blit
		// instead of a swap (does this apply to incorrect refresh rates or only
		// incorrect resolutions?).

		// Set swap chain properties
		// NOTE: If the DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH flag is used, the
		// display mode that most closely matches the back buffer will be used
		// when entering fullscreen. If this happens to be the same size as the
		// back buffer, no WM_SIZE event is sent to the application (I'm only
		// assuming it *does* get sent if the size changes, I haven't tested it).
		// If the flag is not
		// used, the display mode will be changed to match that of the desktop
		// (usually the monitors native display mode). This generally results in a
		// WM_SIZE event (again, I'm only assuming one will not be sent if the
		// window happens to already be the same size as the desktop). For now, I
		// think it makes the most sense to use the native display mode when
		// entering fullscreen, so I'm removing the flag.
		//
		// If we use a flip presentation swap chain, explicitly force destruction
		// of any previous swap chains before creating a new one.
		// https://msdn.microsoft.com/en-us/library/windows/desktop/ff476425(v=vs.85).aspx#Defer_Issues_with_Flip
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		swapChainDesc.BufferDesc.Width                   = rendererState.renderSize.x;
		swapChainDesc.BufferDesc.Height                  = rendererState.renderSize.y;
		// TODO: Get values from system (match desktop. what happens if it changes?)
		swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.Format                  = rendererState.sharedFormat;
		swapChainDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count                   = rendererState.multisampleCount;
		swapChainDesc.SampleDesc.Quality                 = rendererState.qualityCountShared - 1;
		swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount                        = 1;
		swapChainDesc.OutputWindow                       = s.hwnd;
		swapChainDesc.Windowed                           = true;
		swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags                              = 0;//DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// NOTE: IDXGISwapChain::ResizeTarget to resize window if render target changes

		// Create the swap chain
		ComPtr<IDXGIDevice1> dxgiDevice;
		hr = rendererState.d3dDevice.As(&dxgiDevice);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI device");

		ComPtr<IDXGIAdapter> dxgiAdapter;
		hr = dxgiDevice->GetAdapter(&dxgiAdapter);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI adapter");

		ComPtr<IDXGIFactory1> dxgiFactory;
		hr = dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Fatal, "Failed to get DXGI factory");

		hr = dxgiFactory->CreateSwapChain(rendererState.d3dDevice.Get(), &swapChainDesc, &s.swapChain);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Error, "Failed to create preview window swap chain");
		SetDebugObjectName(s.swapChain, "Preview Swap Chain");

		// Get the back buffer
		hr = s.swapChain->GetBuffer(0, IID_PPV_ARGS(&s.backBuffer));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Error, "Failed to preview window create back buffer");
		SetDebugObjectName(s.backBuffer, "Preview Back Buffer");

		// Associate the window
		hr = dxgiFactory->MakeWindowAssociation(s.hwnd, DXGI_MWA_NO_ALT_ENTER);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Error, "Failed to associate DXGI and preview window");
	}

	s.simulationState->previewWindow = true;
	return true;
}

b8
PreviewWindow_Teardown(PreviewWindowState& s)
{
	s.simulationState->previewWindow = false;

	// Detach Renderer
	{
		s.backBuffer.Reset();
		s.swapChain .Reset();
	}


	// Destroy Window
	{
		b8 success = DestroyWindow(s.hwnd);
		LOG_LAST_ERROR_IF(!success, return false,
			Severity::Error, "Failed to destroy preview window");

		success = UnregisterClassA(previewWindowClass, s.hInstance);
		LOG_LAST_ERROR_IF(!success, IGNORE,
			Severity::Error, "Failed to unregister preview window class");

		s = {};
	}

	return true;
}

b8
PreviewWindow_Render(PreviewWindowState& s)
{
	if (!s.hwnd) return true;

	SimulationState& simulationState = *s.simulationState;
	RendererState&   rendererState   = *s.rendererState;

	// TODO: Want to label this, but it doesn't go through the normal render queue...
	//Renderer_PushEvent(rendererState, "Update Preview Window");
	RenderTargetData mainRTGUI = rendererState.renderTargets[simulationState.renderTargetGUICopy];
	rendererState.d3dContext->CopyResource(s.backBuffer.Get(), mainRTGUI.d3dRenderTarget.Get());
	//Renderer_PopEvent(rendererState);

	// TODO: Handle DXGI_ERROR_DEVICE_RESET and DXGI_ERROR_DEVICE_REMOVED
	// Developer Command Prompt for Visual Studio as an administrator, and
	// typing dxcap -forcetdr which will immediately cause all currently
	// running Direct3D apps to get a DXGI_ERROR_DEVICE_REMOVED event.
	HRESULT hr = s.swapChain->Present(0, 0);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to present preview window");

	return true;
}

static v2i
GetMousePosition(LPARAM lParam, u16 zoomFactor, v2u renderSize)
{
	v2i pos = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
	pos /= (i32) zoomFactor;
	pos.y = (i32) renderSize.y - 1 - pos.y;
	return pos;
}

LRESULT CALLBACK
PreviewWndProc(HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
	PreviewWindowState* s = (PreviewWindowState*) GetWindowLongPtrA(hwnd, GWLP_USERDATA);


	switch (uMsg)
	{
		case WM_NCCREATE:
		{
			CREATESTRUCT& createStruct = *(CREATESTRUCT*) lParam;
			s = (PreviewWindowState*) createStruct.lpCreateParams;

			// NOTE: Because Windows is dumb. See Return value section:
			// https://msdn.microsoft.com/en-us/library/windows/desktop/ms644898.aspx
			SetLastError(0);

			i64 iResult = SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LPARAM) s);
			if (iResult == 0 && GetLastError() != 0)
			{
				LOG_LAST_ERROR(Severity::Error, "Failed to stash preview window state pointer");
				return false;
			}
			break;
		}

		case WM_ENTERMENULOOP:
		case WM_ENTERSIZEMOVE:
		{
			u64 result = SetTimer(s->hwnd, 1, 1, 0);
			LOG_LAST_ERROR_IF(result == 0, IGNORE, Severity::Warning, "Failed to set modal timer");
			break;
		}

		case WM_EXITMENULOOP:
		case WM_EXITSIZEMOVE:
		{
			b8 success = KillTimer(s->hwnd, 1);
			LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to kill modal timer");
			break;
		}

		case WM_PAINT:
		case WM_TIMER:
		{
			SwitchToFiber(s->mainFiber);
			break;
		}

		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE)
			{
				FromGUI::MouseMove mouseMove = {};
				mouseMove.pos = s->mousePos;
				FromGUI_MouseMove(*s->simulationState, mouseMove);
			}
			break;
		}

		case WM_MOUSEMOVE:
		{
			s->mousePos = GetMousePosition(lParam, s->zoomFactor, s->renderSize);
			FromGUI::MouseMove mouseMove = {};
			mouseMove.pos = s->mousePos;
			FromGUI_MouseMove(*s->simulationState, mouseMove);
			break;
		}

		case WM_LBUTTONDOWN:
		{
			// TODO: If you move the mouse quickly and click it's possible to get this without a
			// corresponding mouse move. Maybe they come out of order? This probably applies to all
			// mouse buttons. Maybe also keyboard? Ugh.
			// TODO: Assertion fires occasionally if GUI and preview are open. They're fighting to set
			// mouse the position.
			v2i pos = GetMousePosition(lParam, s->zoomFactor, s->renderSize);
			Assert(pos == s->simulationState->mousePos);

			if (s->simulationState->guiInteraction != GUIInteraction::Null) break;

			if (s->mouseCaptureCount++ == 0)
				SetCapture(s->hwnd);

			FromGUI::SelectHovered selectHovered = {};
			FromGUI_SelectHovered(*s->simulationState, selectHovered);

			FromGUI::BeginDragSelection beginDrag = {};
			FromGUI_BeginDragSelection(*s->simulationState, beginDrag);
			break;
		}

		case WM_LBUTTONUP:
		{
			if (s->simulationState->guiInteraction != GUIInteraction::DragSelection) break;

			if (--s->mouseCaptureCount == 0)
			{
				b8 success = ReleaseCapture();
				LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to release mouse capture");
			}

			FromGUI::EndDragSelection endDrag = {};
			FromGUI_EndDragSelection(*s->simulationState, endDrag);
			break;
		}

		case WM_RBUTTONDOWN:
		{
			v2i pos = GetMousePosition(lParam, s->zoomFactor, s->renderSize);
			Assert(pos == s->simulationState->mousePos);

			if (s->simulationState->guiInteraction != GUIInteraction::Null) break;

			if (s->mouseCaptureCount++ == 0)
				SetCapture(s->hwnd);

			FromGUI::BeginMouseLook beginMouseLook = {};
			FromGUI_BeginMouseLook(*s->simulationState, beginMouseLook);
			break;
		}

		case WM_RBUTTONUP:
		{
			if (s->simulationState->guiInteraction != GUIInteraction::MouseLook) break;

			if (--s->mouseCaptureCount == 0)
			{
				b8 success = ReleaseCapture();
				LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to release mouse capture");
			}

			v2i pos = GetMousePosition(lParam, s->zoomFactor, s->renderSize);
			Assert(pos == s->simulationState->mousePos);
			FromGUI::EndMouseLook endMouseLook = {};
			FromGUI_EndMouseLook(*s->simulationState, endMouseLook);
			break;
		}

		case WM_MBUTTONDOWN:
		{
			v2i pos = GetMousePosition(lParam, s->zoomFactor, s->renderSize);
			Assert(pos == s->simulationState->mousePos);

			if (s->simulationState->guiInteraction != GUIInteraction::Null) break;

			ResetCamera(*s->simulationState);
			break;
		}

		case WM_MOUSEWHEEL:
		{
			s->mouseWheelAccumulator += GET_WHEEL_DELTA_WPARAM(wParam);
			u16 newZoomFactor = s->zoomFactor;
			while (s->mouseWheelAccumulator >= WHEEL_DELTA)
			{
				s->mouseWheelAccumulator -= WHEEL_DELTA;
				newZoomFactor++;
			}
			while (s->mouseWheelAccumulator <= -WHEEL_DELTA)
			{
				s->mouseWheelAccumulator += WHEEL_DELTA;
				if (newZoomFactor > 1) newZoomFactor--;
			}

			if (newZoomFactor != s->zoomFactor)
			{
				RECT windowRect;
				b8 success = GetWindowRect(s->hwnd, &windowRect);
				LOG_LAST_ERROR_IF(!success, return 0,
					Severity::Warning, "Failed to get preview window rect");

				v2i windowCenter;
				windowCenter.x = (windowRect.right + windowRect.left) / 2;
				windowCenter.y = (windowRect.bottom + windowRect.top) / 2;

				v2u newClientSize = (u32) newZoomFactor * s->renderSize;

				RECT usableDesktopRect;
				success = SystemParametersInfoA(SPI_GETWORKAREA, 0, &usableDesktopRect, 0);
				LOG_LAST_ERROR_IF(!success, return 0,
					Severity::Warning, "Failed to get usable desktop area");

				v2u usableDesktopSize;
				usableDesktopSize.x = (u32) (usableDesktopRect.right - usableDesktopRect.left);
				usableDesktopSize.y = (u32) (usableDesktopRect.bottom - usableDesktopRect.top);

				v2u newWindowSize = newClientSize + s->nonClientSize;
				if (newWindowSize.x > usableDesktopSize.x || newWindowSize.y > usableDesktopSize.y)
				{
					if (newZoomFactor > s->zoomFactor)
						break;
				}

				// TODO: Try expanding from top left only
				// TODO: Maybe account for the shadow? (window - client - border)
				// or DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &extendedRect, sizeof(extendedRect));
				v2i newWindowTopLeft;
				newWindowTopLeft.x = windowCenter.x - ((i32) newWindowSize.x / 2);
				newWindowTopLeft.y = windowCenter.y - ((i32) newWindowSize.y / 2);

				success = SetWindowPos(
					s->hwnd,
					nullptr,
					newWindowTopLeft.x,
					newWindowTopLeft.y,
					(i32) newWindowSize.x,
					(i32) newWindowSize.y,
					0
				);
				LOG_LAST_ERROR_IF(!success, return 0,
					Severity::Warning, "Failed to change preview window position during zoom");

				s->zoomFactor = newZoomFactor;

				success = SetForegroundWindow(s->hwnd);
				LOG_LAST_ERROR_IF(!success, IGNORE,
					Severity::Warning, "Failed to bring preview window to the foreground during zoom");
			}

			return 0;
		}

		case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_ESCAPE:
					if (s->simulationState->guiInteraction != GUIInteraction::Null) break;

					PostQuitMessage(0);
					return 0;

				case VK_DELETE:
					if (s->simulationState->guiInteraction != GUIInteraction::Null) break;

					RemoveSelectedWidgets(*s->simulationState);
					return 0;
			}

			break;
		}

		case WM_CLOSE:
		{
			// TODO: If we hold onto the necessary pointers, we could just teardown directly
			b8 success = PostMessageA(nullptr, WM_PREVIEWWINDOWCLOSED, 0, 0);
			LOG_LAST_ERROR_IF(!success, IGNORE,
				Severity::Error, "Failed to post preview window close message");
			return 0;
		}
	}

	return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

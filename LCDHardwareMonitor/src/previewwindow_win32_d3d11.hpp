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
	u16                     zoomFactor;
	i16                     mouseWheelAccumulator;
	SimulationState*        simulationState;
	RendererState*          rendererState;
	b32                     mouseLook;
	v2i                     mousePosStart;
	v2                      cameraRotStart;
	v2                      cameraRot;
};

b32
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
		b32 success;

		LRESULT CALLBACK
		PreviewWndProc(HWND, u32, WPARAM, LPARAM);

		WNDCLASSA windowClass = {};
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

		ATOM classAtom = RegisterClassA(&windowClass);
		LOG_LAST_ERROR_IF(classAtom == INVALID_ATOM, return false,
			Severity::Error, "Failed to register preview window class");

		u32 windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

		RECT windowRect = { 0, 0, (i32) rendererState.renderSize.x, (i32) rendererState.renderSize.y };
		success = AdjustWindowRect(&windowRect, (u32) windowStyle, false);
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
		swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count                   = rendererState.multisampleCount;
		swapChainDesc.SampleDesc.Quality                 = rendererState.qualityLevelCount - 1;
		swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount                        = 1;
		swapChainDesc.OutputWindow                       = s.hwnd;
		swapChainDesc.Windowed                           = true;
		swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags                              = 0;//DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// NOTE: IDXGISwapChain::ResizeTarget to resize window if render target changes

		// Create the swap chain
		HRESULT hr;
		hr = rendererState.dxgiFactory->CreateSwapChain(rendererState.d3dDevice.Get(), &swapChainDesc, &s.swapChain);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Error, "Failed to create preview window swap chain");
		SetDebugObjectName(s.swapChain, "Preview Swap Chain");

		// Get the back buffer
		hr = s.swapChain->GetBuffer(0, IID_PPV_ARGS(&s.backBuffer));
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Error, "Failed to preview window create back buffer");
		SetDebugObjectName(s.backBuffer, "Preview Back Buffer");

		// Create a render target view to the back buffer
		//ComPtr<ID3D11RenderTargetView> renderTargetView;
		//hr = rendererState.d3dDevice->CreateRenderTargetView(s.backBuffer.Get(), nullptr, &renderTargetView);
		//LOG_HRESULT_IF_FAILED(hr, return false,
		//	Severity::Error, "Failed to create preview window render target view");
		//SetDebugObjectName(renderTargetView, "Preview Render Target View");

		// Associate the window
		hr = rendererState.dxgiFactory->MakeWindowAssociation(s.hwnd, DXGI_MWA_NO_ALT_ENTER);
		LOG_HRESULT_IF_FAILED(hr, return false,
			Severity::Error, "Failed to associate DXGI and preview window");
	}

	return true;
}

b32
PreviewWindow_Teardown(PreviewWindowState& s)
{
	// Detach Renderer
	{
		s.backBuffer.Reset();
		s.swapChain .Reset();
	}


	// Destroy Window
	{
		b32 success;

		success = DestroyWindow(s.hwnd);
		LOG_LAST_ERROR_IF(!success, return false,
			Severity::Error, "Failed to destroy preview window");

		success = UnregisterClassA(previewWindowClass, s.hInstance);
		LOG_LAST_ERROR_IF(!success, IGNORE,
			Severity::Error, "Failed to unregister preview window class");

		s = {};
	}

	return true;
}

b32
PreviewWindow_Render(PreviewWindowState& s)
{
	if (!s.hwnd) return true;

	RendererState& rendererState = *s.rendererState;

	// TODO: Handle DXGI_ERROR_DEVICE_RESET and DXGI_ERROR_DEVICE_REMOVED
	// Developer Command Prompt for Visual Studio as an administrator, and
	// typing dxcap -forcetdr which will immediately cause all currently
	// running Direct3D apps to get a DXGI_ERROR_DEVICE_REMOVED event.
	rendererState.d3dContext->CopyResource(s.backBuffer.Get(), rendererState.d3dRenderTexture.Get());
	HRESULT hr = s.swapChain->Present(0, 0);
	LOG_HRESULT_IF_FAILED(hr, return false,
		Severity::Error, "Failed to present preview window");

	return true;
}

static void
UpdateCameraPosition(PreviewWindowState& s, LPARAM lParam)
{
	SimulationState& simulationState = *s.simulationState;

	v2i mousePos = { GET_X_LPARAM(lParam), -GET_Y_LPARAM(lParam) };
	v2i deltaPos = mousePos - s.mousePosStart;
	s.cameraRot.yaw   = 0.0005f * deltaPos.x * 2*r32Pi;
	s.cameraRot.pitch = 0.0005f * deltaPos.y * 2*r32Pi;

	v3 rot = (v3) (s.cameraRotStart + s.cameraRot);
	rot.pitch = Clamp(rot.pitch, -0.49f*r32Pi, 0.49f*r32Pi);
	rot.roll  = 500;

	v3 target = { 160.0f, 120.0f, 0.0f };
	v3 pos    = GetOrbitPos(target, rot);

	simulationState.cameraPos = pos;
	simulationState.view      = LookAt(pos, target);
	simulationState.vp        = simulationState.view * simulationState.proj;
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
			u32 result = SetTimer(s->hwnd, 1, 1, 0);
			LOG_LAST_ERROR_IF(result == 0, IGNORE, Severity::Warning, "Failed to set modal timer");
			break;
		}

		case WM_EXITMENULOOP:
		case WM_EXITSIZEMOVE:
		{
			b32 success = KillTimer(s->hwnd, 1);
			LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to kill modal timer");
			break;
		}

		case WM_PAINT:
		case WM_TIMER:
		{
			SwitchToFiber(s->mainFiber);
			break;
		}

		case WM_LBUTTONDOWN:
		{
			SetCapture(s->hwnd);

			s->mouseLook      = true;
			s->mousePosStart  = { GET_X_LPARAM(lParam), -GET_Y_LPARAM(lParam) };
			s->cameraRotStart = s->cameraRot;
			break;
		}

		case WM_MOUSEMOVE:
		{
			if (!s->mouseLook) break;
			UpdateCameraPosition(*s, lParam);
			break;
		}

		case WM_LBUTTONUP:
		{
			b32 success = ReleaseCapture();
			LOG_LAST_ERROR_IF(!success, IGNORE, Severity::Warning, "Failed to release mouse capture");

			s->mouseLook       = false;
			s->mousePosStart   = {};
			s->cameraRot      += s->cameraRotStart;
			s->cameraRotStart  = {};
			break;
		}

		case WM_MBUTTONDOWN:
		{
			s->mousePosStart = { GET_X_LPARAM(lParam), -GET_Y_LPARAM(lParam) };
			UpdateCameraPosition(*s, lParam);
			break;
		}

		case WM_MOUSEWHEEL:
		{
			// TODO: Fix mouse cursor
			// TODO: Is it worth trying to make things portable?
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
				b32 success;

				RECT windowRect;
				success = GetWindowRect(s->hwnd, &windowRect);
				LOG_LAST_ERROR_IF(!success, return 0,
					Severity::Warning, "Failed to get preview window rect");

				v2i windowCenter;
				windowCenter.x = (windowRect.right + windowRect.left) / 2;
				windowCenter.y = (windowRect.bottom + windowRect.top) / 2;

				v2u newClientSize = newZoomFactor * s->renderSize;

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
			if (wParam == VK_ESCAPE)
			{
				PostQuitMessage(0);
				return 0;
			}
			break;
		}

		case WM_CLOSE:
		{
			// TODO: If we hold onto the necessary pointers, we could just teardown directly
			b32 success = PostMessageA(nullptr, WM_PREVIEWWINDOWCLOSED, 0, 0);
			LOG_LAST_ERROR_IF(!success, IGNORE,
				Severity::Error, "Failed to post preview window close message");
			return 0;
		}
	}

	return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

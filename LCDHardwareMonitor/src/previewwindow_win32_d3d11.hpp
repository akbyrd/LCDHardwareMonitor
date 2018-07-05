const c8  previewWindowClass[] = "LCDHardwareMonitor Preview Class";
const u32 WM_PREVIEWWINDOWCLOSED = WM_USER + 0;

struct PreviewWindowState
{
	HWND                    hwnd                  = nullptr;
	ComPtr<IDXGISwapChain>  swapChain             = nullptr;
	ComPtr<ID3D11Texture2D> backBuffer            = nullptr;
	V2i                     renderSize            = {};
	V2i                     nonClientSize         = {};
	u16                     zoomFactor            = 1;
	i16                     mouseWheelAccumulator = 0;
};

b32 PreviewWindow_Initialize (PreviewWindowState*, RendererState*, HINSTANCE);
b32 PreviewWindow_Teardown   (PreviewWindowState*, RendererState*, HINSTANCE);
b32 PreviewWindow_Render     (RendererState*, PreviewWindowState*);

b32
PreviewWindow_Initialize(PreviewWindowState* s, RendererState* rendererState, HINSTANCE hInstance)
{
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
		windowClass.hCursor       = nullptr;
		windowClass.hbrBackground = nullptr;
		windowClass.lpszMenuName  = nullptr;
		windowClass.lpszClassName = previewWindowClass;

		ATOM classAtom = RegisterClassA(&windowClass);
		LOG_LAST_ERROR_IF(classAtom == INVALID_ATOM, "RegisterClass failed", Severity::Error, return false);

		auto windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VISIBLE;

		b32 success;
		RECT windowRect = { 0, 0, rendererState->renderSize.x, rendererState->renderSize.y };
		success = AdjustWindowRect(&windowRect, (u32) windowStyle, false);
		LOG_LAST_ERROR_IF(!success, "AdjustWindowRect failed", Severity::Warning);

		s->renderSize = rendererState->renderSize;

		s->nonClientSize.x = (windowRect.right - windowRect.left) - s->renderSize.x;
		s->nonClientSize.y = (windowRect.bottom - windowRect.top) - s->renderSize.y;

		s->hwnd = CreateWindowA(
			windowClass.lpszClassName,
			"LHM Preview",
			(u32) windowStyle,
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
			LOG_LAST_ERROR("CreateWindowEx failed", Severity::Error);
			s->hwnd = nullptr;
			return false;
		}

		success = SetForegroundWindow(s->hwnd);
		LOG_LAST_ERROR_IF(!success, "SetForegroundWindow failed", Severity::Warning);
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
		swapChainDesc.BufferDesc.Width                   = (u32) rendererState->renderSize.x;
		swapChainDesc.BufferDesc.Height                  = (u32) rendererState->renderSize.y;
		// TODO: Get values from system (match desktop. what happens if it changes?)
		swapChainDesc.BufferDesc.RefreshRate.Numerator   = 60;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.Format                  = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering        = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling                 = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count                   = rendererState->multisampleCount;
		swapChainDesc.SampleDesc.Quality                 = rendererState->qualityLevelCount - 1;
		swapChainDesc.BufferUsage                        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount                        = 1;
		swapChainDesc.OutputWindow                       = s->hwnd;
		swapChainDesc.Windowed                           = true;
		swapChainDesc.SwapEffect                         = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags                              = 0;//DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		// NOTE: IDXGISwapChain::ResizeTarget to resize window if render target changes

		// Create the swap chain
		HRESULT hr;
		hr = rendererState->dxgiFactory->CreateSwapChain(rendererState->d3dDevice.Get(), &swapChainDesc, &s->swapChain);
		LOG_HRESULT_IF_FAILED(hr, "", Severity::Error, return false);
		SetDebugObjectName(s->swapChain, "Swap Chain");

		// Get the back buffer
		hr = s->swapChain->GetBuffer(0, IID_PPV_ARGS(&s->backBuffer));
		LOG_HRESULT_IF_FAILED(hr, "", Severity::Error, return false);
		SetDebugObjectName(s->backBuffer, "Back Buffer");

		// Create a render target view to the back buffer
		//ComPtr<ID3D11RenderTargetView> renderTargetView;
		//hr = rendererState->d3dDevice->CreateRenderTargetView(s->backBuffer.Get(), nullptr, &renderTargetView);
		//LOG_HRESULT_IF_FAILED(hr, "", Severity::Error, return false);
		//SetDebugObjectName(renderTargetView, "Render Target View");

		// Associate the window
		hr = rendererState->dxgiFactory->MakeWindowAssociation(s->hwnd, DXGI_MWA_NO_ALT_ENTER);
		LOG_HRESULT_IF_FAILED(hr, "", Severity::Error, return false);
	}

	return true;
}

b32
PreviewWindow_Teardown(PreviewWindowState* s, RendererState* rendererState, HINSTANCE hInstance)
{
	UNUSED(rendererState);

	// Detach Renderer
	{
		s->backBuffer.Reset();
		s->swapChain .Reset();
	}


	// Destroy Window
	{
		b32 success;

		success = DestroyWindow(s->hwnd);
		LOG_LAST_ERROR_IF(!success, "DestroyWindow failed", Severity::Error, return false);

		success = UnregisterClassA(previewWindowClass, hInstance);
		LOG_LAST_ERROR_IF(!success, "UnregisterClass failed", Severity::Warning);

		*s = {};
	}

	return true;
}

b32
PreviewWindow_Render(PreviewWindowState* s, RendererState* rendererState)
{
	if (s->hwnd)
	{
		// TODO: Handle DXGI_ERROR_DEVICE_RESET and DXGI_ERROR_DEVICE_REMOVED
		// Developer Command Prompt for Visual Studio as an administrator, and
		// typing dxcap -forcetdr which will immediately cause all currently
		// running Direct3D apps to get a DXGI_ERROR_DEVICE_REMOVED event.
		rendererState->d3dContext->CopyResource(s->backBuffer.Get(), rendererState->d3dRenderTexture.Get());
		HRESULT hr = s->swapChain->Present(0, 0);
		LOG_HRESULT_IF_FAILED(hr, "", Severity::Error, return false);
	}

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

			// NOTE: Because Windows is dumb. See Return value section:
			// https://msdn.microsoft.com/en-us/library/windows/desktop/ms644898.aspx
			SetLastError(0);

			i64 iResult = SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LPARAM) s);
			if (iResult == 0 && GetLastError() != 0)
			{
				LOG_LAST_ERROR("SetWindowLongPtr failed", Severity::Error);
				return false;
			}
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
				LOG_LAST_ERROR_IF(!success, "GetWindowRect failed", Severity::Warning, return 0);

				V2i windowCenter;
				windowCenter.x = (windowRect.right + windowRect.left) / 2;
				windowCenter.y = (windowRect.bottom + windowRect.top) / 2;

				V2i newClientSize = newZoomFactor * s->renderSize;

				RECT usableDesktopRect;
				success = SystemParametersInfoW(SPI_GETWORKAREA, 0, &usableDesktopRect, 0);
				LOG_LAST_ERROR_IF(!success, "SystemParametersInfo failed", Severity::Warning, return 0);

				V2i usableDesktopSize;
				usableDesktopSize.x = usableDesktopRect.right - usableDesktopRect.left;
				usableDesktopSize.y = usableDesktopRect.bottom - usableDesktopRect.top;

				V2i newWindowSize = newClientSize + s->nonClientSize;
				if (newWindowSize.x > usableDesktopSize.x || newWindowSize.y > usableDesktopSize.y)
				{
					if (newZoomFactor > s->zoomFactor)
						break;
				}

				// TODO: Try expanding from top left only
				// TODO: Maybe account for the shadow? (window - client - border)
				// or DwmGetWindowAttribute(hWnd, DWMWA_EXTENDED_FRAME_BOUNDS, &extendedRect, sizeof(extendedRect));
				V2i newWindowTopLeft;
				newWindowTopLeft.x = windowCenter.x - (newWindowSize.x / 2);
				newWindowTopLeft.y = windowCenter.y - (newWindowSize.y / 2);

				if (newWindowTopLeft.x < 0) newWindowTopLeft.x = 0;
				if (newWindowTopLeft.y < 0) newWindowTopLeft.y = 0;

				success = SetWindowPos(
					s->hwnd,
					nullptr,
					newWindowTopLeft.x,
					newWindowTopLeft.y,
					newWindowSize.x,
					newWindowSize.y,
					0
				);
				LOG_LAST_ERROR_IF(!success, "SetWindowPos failed", Severity::Warning, return 0);

				s->zoomFactor = newZoomFactor;

				success = SetForegroundWindow(s->hwnd);
				LOG_LAST_ERROR_IF(!success, "SetForegroundWindow failed", Severity::Warning);
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
			b32 success = PostMessageW(nullptr, WM_PREVIEWWINDOWCLOSED, 0, 0);
			LOG_LAST_ERROR_IF(!success, "PostMessage failed", Severity::Warning);

			// NOTE: The window can be toggle at will. Don't stop the simulation
			// just because it's not open.
			//PostQuitMessage(0);
			return 0;
		}
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

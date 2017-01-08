#pragma once

/* Notes:
 * Functions bodies inside a type are implicitly inline.
 * Only function bodies marked inline or static are allowed in headers.
 * Static functions outside of a type are local to the translation unit.
 */

struct Win32State
{
	HWND      hwnd          = nullptr;
	HINSTANCE hInstance     = nullptr;
	V2i       minClientSize = {200, 200};
	V2i       clientSize    = {};
};

#pragma region Foward Declarations
bool ClientSizeToWindowSize(V2i, DWORD, V2i*);
LRESULT CALLBACK StaticWndProc(HWND, u32, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(Win32State*, HWND, u32, WPARAM, LPARAM);
#pragma endregion

bool
InitializeWindow(Win32State* win32State, LPCWSTR applicationName, V2i desiredClientSize)
{
	WNDCLASS wc      = {};
	wc.style         = 0;
	wc.lpfnWndProc   = StaticWndProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = sizeof(Win32State*);
	wc.hInstance     = win32State->hInstance;
	wc.hIcon         = nullptr;
	wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
	wc.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
	wc.lpszMenuName  = nullptr;
	wc.lpszClassName = applicationName;

	V2i windowPos  = {CW_USEDEFAULT, CW_USEDEFAULT};
	V2i windowSize = desiredClientSize;

	win32State->clientSize = desiredClientSize;

	DWORD windowStyle = WS_BORDER; //WS_POPUP;

	ClientSizeToWindowSize(win32State->minClientSize, windowStyle, &win32State->minClientSize);
	ClientSizeToWindowSize(desiredClientSize, windowStyle, &windowSize);

	//Clamp size and center on desktop
	RECT usableDesktopRect = {};
	if (SystemParametersInfoW(SPI_GETWORKAREA, 0, &usableDesktopRect, 0))
	{
		V2i usableDesktop = {
			usableDesktopRect.right - usableDesktopRect.left,
			usableDesktopRect.bottom - usableDesktopRect.top
		};

		Clamp(win32State->minClientSize, usableDesktop);

		i32 delta = windowSize.x - usableDesktop.x;
		if (delta > 0)
		{
			windowSize.x             -= delta;
			win32State->clientSize.x -= delta;
		}

		delta = windowSize.y - usableDesktop.y;
		if (delta > 0)
		{
			windowSize.y             -= delta;
			win32State->clientSize.y -= delta;
		}

		windowPos = (usableDesktop - windowSize) / 2;
	}
	else
	{
		LOG_LASTERROR();
	}

	ATOM atom = RegisterClassW(&wc);
	if (atom == INVALID_ATOM)
	{
		LOG_LASTERROR();
		return false;
	}

	win32State->hwnd = CreateWindowExW(
		WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
		applicationName, applicationName,
		windowStyle,
		windowPos.x, windowPos.y,
		windowSize.x, windowSize.y,
		nullptr, nullptr,
		win32State->hInstance,
		win32State
	);
	if (win32State->hwnd == nullptr)
	{
		LOG_LASTERROR();
		return false;
	}

	ShowWindow(win32State->hwnd, SW_SHOW);

	return true;
}

inline bool
ClientSizeToWindowSize(V2i clientSize, DWORD windowStyle, V2i* windowSize)
{
	// TODO: It's pretty easy to create style where this function returns
	// different numbers than CreateWindow will. This in turn can cause an
	// immediate resize/swap chain rebuild. Should place a strategic assert to
	// catch that case early.

	bool success;

	RECT windowRect = {0, 0, clientSize.x, clientSize.y};
	success = AdjustWindowRect(&windowRect, windowStyle, false);
	if (!success)
	{
		LOG_WARNING(L"Failed to translate client size to window size");
		return false;
	}

	windowSize->x = windowRect.right - windowRect.left;
	windowSize->y = windowRect.bottom - windowRect.top;

	return true;
}

inline bool
GetWindowClientSize(HWND hwnd, V2i* clientSize)
{
	RECT rect = {};
	if ( GetClientRect(hwnd, &rect) )
	{
		clientSize->x = rect.right - rect.left;
		clientSize->y = rect.bottom - rect.top;
	}
	else
	{
		HRESULT hr = GetLastError();
		if ( !LOG_HRESULT(hr) )
			LOG_WARNING(L"GetClientRect failed, but the last error passed a FAILED. HR = " + std::to_wstring(hr));

		return false;
	}

	return true;
}

LRESULT CALLBACK
StaticWndProc(HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
	Win32State* win32State = nullptr;

	//NOTE: This misses the very first WM_GETMINMAXINFO.
	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT *params = (CREATESTRUCT*) lParam;
		win32State = (Win32State*) params->lpCreateParams;

		win32State->hwnd = hwnd;
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG) win32State);
	}
	else
	{
		win32State = (Win32State*) GetWindowLongPtrW(hwnd, GWLP_USERDATA);
	}

	if (win32State)
	{
		return WndProc(win32State, hwnd, uMsg, wParam, lParam);
	}
	else
	{
		return DefWindowProcW(hwnd, uMsg, wParam, lParam);
	}
}

LRESULT CALLBACK
WndProc(Win32State* win32State, HWND hwnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_KEYDOWN:
		{
			if ( wParam == VK_ESCAPE )
				PostQuitMessage(0);
			return 0;
		}

		case WM_MENUCHAR:
		{
			//Recieved unhandled keystroke while a menu is active.
			UINT menuType = HIWORD(wParam);
			bool isSystem = menuType == MF_SYSMENU;

			UINT keyPressed = LOWORD(wParam);
			bool enterPressed = keyPressed == VK_RETURN;

			//Don't beep when exiting fullscreen with Alt+Enter
			if (isSystem && enterPressed)
				return MAKELRESULT(0, MNC_CLOSE);

			break;
		}

		//TODO: Exit codes?
		//TODO: How should the application exit? Quit message? Inform the sim? Just queue input?
		case WM_CLOSE:
		{
			/* NOTE: Received when the window's X button is clicked, Alt-F4,
			* SysMenu > Close clicked, SysMenu double clicked, Right click
			* Taskbar > Close, Task Manager Processes > End Task clicked
			*/
			PostQuitMessage(0);
			return 0;
		}

		case WM_ENDSESSION:
		{
			//NOTE: Application is shutting down, probably due to user logging off.
			//TODO: Perform an expedient shutdown here.
			return 0;
		}
	}

	return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}
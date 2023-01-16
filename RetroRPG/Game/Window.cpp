#include "Window.h"
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN 1
#	define WIN_32_EXTRA_LEAN 1
#	define NOMINMAX
#	define WINVER 0x0600
#	include <SDKDDKVer.h>
#	include <winapifamily.h>
#	include <Windows.h>
#	include <wingdi.h>
#endif // _WIN32

#if defined(__linux__)
#	include <X11/X.h>
#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#endif // __linux__

#if defined(__EMSCRIPTEN__)
#endif // __EMSCRIPTEN__
//-----------------------------------------------------------------------------
#if defined(_WIN32)
LRESULT CALLBACK windowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
static void decodeMouseButton(Window* window, uint32_t wParam, uint32_t lParam);
static void decodeKey(Window* window, uint32_t wParam, uint32_t lParam);
static void createWindow(Window* window, Handle& handle, const char* title, bool fullscreen);
static void destroyWindow(Window* window, Handle& handle);
static const char* windowClassName = "Game";
static DEVMODE previousDevMode;
static bool previousDevModeValid = false;
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(__linux__)
static Display * usedXDisplay = NULL;
static int usedXDisplayCount = 0;
static const int xEventMask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask | PointerMotionMask;
#endif // __linux__
//-----------------------------------------------------------------------------
Window::Window(const char* name, int width, int height, bool fullscreen)
	: Width(width)
	, Height(height)
{
	memset(&m_dc, 0, sizeof(DrawingContext));
	if( name ) m_title = _strdup(name);
	Running = true;
	Status = SYSTEM_ALRIGHT;

#if defined(_WIN32)
	// Create the window class
	WNDCLASSEXA wincl = { 0 };
	wincl.hInstance = NULL;
	wincl.lpszClassName = windowClassName;
	wincl.lpfnWndProc = windowProcedure;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof(WNDCLASSEXA);
	wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
	wincl.lpszMenuName = NULL;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 32;
	wincl.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	if( !RegisterClassExA(&wincl) ) return;

	// Open the window
	SetFullScreen(fullscreen);
#endif // _WIN32

#if defined(__linux__)
	if( !usedXDisplay )
	{
		usedXDisplay = XOpenDisplay(nulptr);
		if( !usedXDisplay )
		{
			printf("window: unable to connect to X Server\n");
			return;
	}
}
	usedXDisplayCount++;

	handle = (Handle)XCreateSimpleWindow(usedXDisplay, RootWindow(usedXDisplay, 0), 0, 0, Width, Height, 1, 0, 0);
	XStoreName(usedXDisplay, (Window)handle, m_title);

	XSelectInput(usedXDisplay, (Window)handle, ExposureMask | xEventMask);
	XMapWindow(usedXDisplay, (Window)handle);

	dc.display = (Handle)usedXDisplay;
	dc.window = handle;
	dc.gc = (Handle)DefaultGC(usedXDisplay, 0);

	Visible = true;
#endif // __linux__

#if defined(__EMSCRIPTEN__)
#endif // __EMSCRIPTEN__
}
//-----------------------------------------------------------------------------
Window::~Window()
{
	Running = false;
	if( m_title ) free(m_title);
#if defined(_WIN32)
	destroyWindow(this, m_handle);
#endif // _WIN32

#if defined(__linux__)
	if( handle ) XDestroyWindow(usedXDisplay, (Window)handle);
	usedXDisplayCount--;
	if( usedXDisplayCount ) return;
	if( usedXDisplay ) XCloseDisplay(usedXDisplay);
#endif // __linux__

#if defined(__EMSCRIPTEN__)
#endif // __EMSCRIPTEN__
}
//-----------------------------------------------------------------------------
void Window::Update()
{
#if defined(_WIN32)
	MSG msg;
	memset(&msg, 0, sizeof(MSG));
	while( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
	{
		if( msg.message == WM_QUIT )
		{
			Status = SYSTEM_EXIT_QUIT;
			Running = false;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
#endif // _WIN32
#if defined(__linux__)
	if( !handle ) return;

	XEvent event;
	while( XCheckWindowEvent((Display *)dc.display, (Window)dc.window, xEventMask, &event) > 0 )
	{
		if( event.type == KeyPress ||
			event.type == KeyRelease )
		{
			int uniState = 0;
			if( event.type == KeyPress )
				uniState = KEYSTATE_PRESSED;
			else uniState = KEYSTATE_RELEASED;

			if( event.xkey.state & ShiftMask ) uniState |= KEYSTATE_SHIFT;
			if( event.xkey.state & ControlMask ) uniState |= KEYSTATE_CTRL;

			sendKeyEvent(event.xkey.keycode, uniState);
		}
		else
			if( event.type == ButtonPress ||
				event.type == ButtonRelease )
			{

				int uniState = 0;
				if( event.xbutton.state & Button1Mask )
					uniState |= MOUSE_LEFT;
				if( event.xbutton.state & Button2Mask )
					uniState |= MOUSE_MIDDLE;
				if( event.xbutton.state & Button3Mask )
					uniState |= MOUSE_RIGHT;
				if( event.xbutton.state & Button4Mask )
					uniState |= MOUSE_WHEEL_UP;
				if( event.xbutton.state & Button5Mask )
					uniState |= MOUSE_WHEEL_DOWN;

				SendMouseEvent(event.xbutton.x, event.xbutton.y, uniState);
			}
			else
				if( event.type == MotionNotify )
				{

					int uniState = 0;
					if( event.xmotion.state & Button1Mask )
						uniState |= MOUSE_LEFT;
					if( event.xmotion.state & Button2Mask )
						uniState |= MOUSE_MIDDLE;
					if( event.xmotion.state & Button3Mask )
						uniState |= MOUSE_RIGHT;
					if( event.xmotion.state & Button4Mask )
						uniState |= MOUSE_WHEEL_UP;
					if( event.xmotion.state & Button5Mask )
						uniState |= MOUSE_WHEEL_DOWN;

					SendMouseEvent(event.xmotion.x, event.xmotion.y, uniState);
				}
				else
					if( event.type == DestroyNotify )
					{
						Visible = false;
					}

	}
#endif
}
//-----------------------------------------------------------------------------
void Window::SetFullScreen(bool fullscreen)
{
#if defined(_WIN32) // only supported for Windows
	if( fullscreen ) setFullScreen();
	else setWindowed();
#endif
}
//-----------------------------------------------------------------------------
Handle Window::GetHandle()
{
	return m_handle;
}
//-----------------------------------------------------------------------------
DrawingContext Window::GetContext()
{
	return m_dc;
}
//-----------------------------------------------------------------------------
void Window::RegisterKeyCallback(KeyCallback callback)
{
	m_keyCallback = callback;
}
//-----------------------------------------------------------------------------
void Window::RegisterMouseCallback(MouseCallback callback)
{
	m_mouseCallback = callback;
}
//-----------------------------------------------------------------------------
void Window::SendKeyEvent(int code, int state)
{
	if( !m_keyCallback ) return;
	m_keyCallback(code, state);
}
//-----------------------------------------------------------------------------
void Window::SendMouseEvent(int x, int y, int buttons)
{
	if( !m_mouseCallback ) return;
	m_mouseCallback(x, y, buttons);
}
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void createWindow(Window* window, Handle& handle, const char* title, bool fullscreen)
{
	// Compute the client size
	RECT size;
	size.top = 0;
	size.left = 0;
	size.right = window->Width;
	size.bottom = window->Height;

	DWORD style = WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX;
	if( fullscreen ) style = WS_POPUP | WS_VISIBLE | WS_SYSMENU;
	AdjustWindowRect(&size, style, 0);

	// Create and display window
	if( (handle = (Handle)CreateWindowExA(
		0,
		windowClassName,
		title, style,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		size.right - size.left,
		size.bottom - size.top,
		HWND_DESKTOP,
		NULL, NULL, NULL
	)) == 0 ) return;
	SetWindowLongPtrA((HWND)handle, 0, (long long)window);
}
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void destroyWindow(Window* window, Handle& handle)
{
	if( !handle ) return;
	DestroyWindow((HWND)handle);
	handle = 0;
	window->FullScreen = false;
	window->Visible = false;
}
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(_WIN32)
LRESULT CALLBACK windowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	Window* window = (Window *)GetWindowLongPtr(hwnd, 0);

	switch( msg )
	{
	case WM_DESTROY:
		window->Visible = false;
		break;

	case WM_KEYUP:
	case WM_SYSKEYUP:
	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
		decodeKey(window, wParam, lParam);
		break;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
		decodeMouseButton(window, wParam, lParam);
		break;
	}

	return DefWindowProcA(hwnd, msg, wParam, lParam);
}
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void decodeMouseButton(Window* window, uint32_t wParam, uint32_t lParam)
{
	int uniButtons = 0;
	int uniX = lParam & 0xFFFF;
	int uniY = lParam >> 16;

	if( wParam & MK_LBUTTON ) uniButtons |= MOUSE_LEFT;
	if( wParam & MK_RBUTTON ) uniButtons |= MOUSE_RIGHT;
	if( wParam & MK_MBUTTON ) uniButtons |= MOUSE_MIDDLE;

	window->SendMouseEvent(uniX, uniY, uniButtons);
}
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void decodeKey(Window* window, uint32_t wParam, uint32_t lParam)
{
	int uniState = 0;
	int uniCode = wParam;

	switch( uniCode )
	{
	case VK_UP:	uniCode = KEYCODE_UP; break;
	case VK_DOWN: uniCode = KEYCODE_DOWN; break;
	case VK_LEFT: uniCode = KEYCODE_LEFT; break;
	case VK_RIGHT: uniCode = KEYCODE_RIGHT; break;
	case VK_SHIFT: uniCode = KEYCODE_SHIFT; break;
	case VK_CONTROL: uniCode = KEYCODE_CTRL; break;
	case VK_MENU: uniCode = KEYCODE_ALT; break;
	case VK_TAB: uniCode = KEYCODE_TAB; break;
	case VK_ESCAPE: uniCode = KEYCODE_ESC; break;
	case VK_BACK: uniCode = KEYCODE_BACKSPACE; break;
	case VK_RETURN: uniCode = KEYCODE_ENTER; break;
	case VK_INSERT: uniCode = KEYCODE_INSERT; break;
	case VK_DELETE: uniCode = KEYCODE_DELETE; break;
	case VK_HOME: uniCode = KEYCODE_HOME; break;
	case VK_END: uniCode = KEYCODE_END; break;
	case VK_PRIOR: uniCode = KEYCODE_PAGEUP; break;
	case VK_NEXT: uniCode = KEYCODE_PAGEDOWN; break;

	default: break;
	}

	if( lParam & 0x80000000 )
		uniState = KEYSTATE_RELEASED;
	else
		uniState = KEYSTATE_PRESSED;

	if( GetKeyState(VK_SHIFT) & 0x8000 )
		uniState |= KEYSTATE_SHIFT;
	if( GetKeyState(VK_CONTROL) & 0x8000 )
		uniState |= KEYSTATE_CTRL;
	if( GetKeyState(VK_MENU) & 0x8000 )
		uniState |= KEYSTATE_ALT;

	window->SendKeyEvent(uniCode, uniState);
}
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void Window::setFullScreen()
{
	// Change display mode
	if( !previousDevModeValid )
	{
		EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &previousDevMode);
		previousDevModeValid = true;

		DEVMODE newDevMode;
		memcpy(&newDevMode, &previousDevMode, sizeof(DEVMODE));
		newDevMode.dmPelsWidth = Width;
		newDevMode.dmPelsHeight = Height;
		newDevMode.dmFields = DM_PELSHEIGHT | DM_PELSWIDTH;
		ChangeDisplaySettings(&newDevMode, CDS_FULLSCREEN);
	}

	// Create a new window
	destroyWindow(this, m_handle);
	createWindow(this, m_handle, m_title, true);
	m_dc.display = 0;
	m_dc.window = m_handle;
	m_dc.gc = (Handle)GetDC((HWND)m_handle);

	// Display the window
	SetWindowPos((HWND)m_handle, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	ShowWindow((HWND)m_handle, SW_MAXIMIZE);
	FullScreen = true;
	Visible = true;
}
#endif // _WIN32
//-----------------------------------------------------------------------------
#if defined(_WIN32)
void Window::setWindowed()
{
	// Revert display mode
	if( previousDevModeValid )
	{
		ChangeDisplaySettings(&previousDevMode, CDS_RESET);
		previousDevModeValid = false;
	}

	// Create a new window
	destroyWindow(this, m_handle);
	createWindow(this, m_handle, m_title, false);
	m_dc.display = 0;
	m_dc.window = m_handle;
	m_dc.gc = (Handle)GetDC((HWND)m_handle);

	// Display the window
	SetWindowPos((HWND)m_handle, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE);
	ShowWindow((HWND)m_handle, SW_NORMAL);
	FullScreen = false;
	Visible = true;
}
#endif // _WIN32
//-----------------------------------------------------------------------------
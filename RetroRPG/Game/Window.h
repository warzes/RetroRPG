#pragma once

#include "Core.h"

enum MOUSE_BUTTONS
{
	MOUSE_LEFT = 0x1,
	MOUSE_RIGHT = 0x2,
	MOUSE_MIDDLE = 0x4,
	MOUSE_WHEEL_UP = 0x8,
	MOUSE_WHEEL_DOWN = 0x8,
};

enum KEY_STATES
{
	KEYSTATE_PRESSED = 0x01,
	KEYSTATE_RELEASED = 0x02,
	KEYSTATE_SHIFT = 0x10,
	KEYSTATE_CTRL = 0x20,
	KEYSTATE_ALT = 0x40,
};

enum KEY_CODES
{
	KEYCODE_NONE = 0,
	KEYCODE_UP = 1,
	KEYCODE_DOWN = 2,
	KEYCODE_LEFT = 3,
	KEYCODE_RIGHT = 4,
	KEYCODE_SHIFT = 5,
	KEYCODE_CTRL = 6,
	KEYCODE_ALT = 7,
	KEYCODE_TAB = 8,
	KEYCODE_ESC = 9,
	KEYCODE_BACKSPACE = 10,
	KEYCODE_ENTER = 11,
	KEYCODE_INSERT = 12,
	KEYCODE_DELETE = 13,
	KEYCODE_HOME = 14,
	KEYCODE_END = 15,
	KEYCODE_PAGEUP = 16,
	KEYCODE_PAGEDOWN = 17,
};

struct DrawingContext
{
	Handle display; // Handle to a native display resource
	Handle window;  // Handle to a native window resource
	Handle gc;      // Handle to a native graphic context resource
};

typedef void(*KeyCallback)(int key, int state);
typedef void(*MouseCallback)(int x, int y, int buttons);

enum SYSTEM_STATUS
{
	SYSTEM_UNKNOWN = 0x0,     // The status is unknown
	SYSTEM_ALRIGHT = 0x01,    // The application is running
	SYSTEM_EXIT_QUIT = 0x02,  // The application is shutting down
	SYSTEM_EXIT_ABORT = 0x04, // The application has been aborted
};

class Window
{
public:
	Window(const char* name, int width, int height, bool fullscreen = false);
	~Window();

	void Update();

	void SetFullScreen(bool fullscreen);

	Handle GetHandle();
	DrawingContext GetContext();

	void RegisterKeyCallback(KeyCallback callback);
	void RegisterMouseCallback(MouseCallback callback);

	void SendKeyEvent(int code, int state);
	void SendMouseEvent(int x, int y, int buttons);

	int Width;                               // Width of window client region (in pixels)
	int Height;                              // Height of window client region (in pixels)
	bool FullScreen = false;                 // Fullscreen state of window
	bool Visible = false;                    // Is the window open or closed

	int Status = SYSTEM_UNKNOWN;
	bool Running = false;
private:
	void setFullScreen();
	void setWindowed();

	char* m_title = nullptr;                 // Window title in title bar
	Handle m_handle = 0;                     // OS native window handle
	DrawingContext m_dc;                     // OS native drawing context handle

	KeyCallback m_keyCallback = nullptr;     // Registrated callback for keyboard events
	MouseCallback m_mouseCallback = nullptr; // Registrated callback for mouse events
};
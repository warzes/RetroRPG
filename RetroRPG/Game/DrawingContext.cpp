#include "DrawingContext.h"
#if defined(_WIN32)
#	define WIN32_LEAN_AND_MEAN 1
#	define WIN_32_EXTRA_LEAN 1
#	define NOMINMAX
#	define WINVER 0x0600
#	include <SDKDDKVer.h>
#	include <winapifamily.h>
#	include <Windows.h>
#endif // _WIN32

#if defined(__linux__)
#	include <X11/X.h>
#	include <X11/Xlib.h>
#	include <X11/Xutil.h>
#endif // __linux__

#if defined(__EMSCRIPTEN__)
#endif // __EMSCRIPTEN__
//-----------------------------------------------------------------------------
Draw::Draw(DrawingContext context, int width, int heigth)
	: Width(width)
	, Height(heigth)
	, m_frontContext(context)
{
#if defined(_WIN32)
	if( !m_frontContext.gc )
	{
		m_frontContext.window = 0;
		m_frontContext.gc = (Handle)GetDC(nullptr);
	}
#endif // _WIN32

#if defined(__linux__)
	Visual* visual = DefaultVisual((Display*)context.display, 0);
	if( visual->c_class != TrueColor )
	{
		printf("Draw: can only draw on truecolor displays!\n");
		return;
	}
	bitmap = (Handle)XCreateImage((Display*)context.display, visual, 24, ZPixmap, 0, (char *)NULL, width, height, 32, 0);
#endif // __linux__
}
//-----------------------------------------------------------------------------
Draw::~Draw()
{
#if defined(__linux__)
	// interestingly XDestroyImage works differently than XCreateImage. XCreateImage will not allocate the data pointer but XDestroyImage will free it as the data has already been freed we need to prevent a double free here
	XImage* image = (XImage *)bitmap;
	if( image )
	{
		image->data = nullptr;
		XDestroyImage(image);
	}
#endif // __linux__
}
//-----------------------------------------------------------------------------
void Draw::SetContext(DrawingContext context)
{
	m_frontContext = context;
}
//-----------------------------------------------------------------------------
void Draw::SetPixels(const void* data)
{
#if defined(_WIN32)
	BITMAPV4HEADER info;
	info.bV4Size = sizeof(BITMAPV4HEADER);
	info.bV4Width = Width;
	info.bV4Height = -Height;
	info.bV4Planes = 1;
	info.bV4BitCount = 32;
	info.bV4V4Compression = BI_RGB;

	SetDIBitsToDevice((HDC)m_frontContext.gc, 0, 0, Width, Height, 0, 0, 0, Height, data, (BITMAPINFO*)&info, DIB_RGB_COLORS);
#endif // _WIN32

#if defined(__linux__)
	XImage* image = (XImage *)bitmap;
	image->data = (char*)data;
	XPutImage((Display*)m_frontContext.display, (Drawable)m_frontContext.window, (GC)m_frontContext.gc, image, 0, 0, 0, 0, Width, Height);
#endif // __linux__
}
//-----------------------------------------------------------------------------
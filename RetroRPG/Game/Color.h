#pragma once

#include <stdint.h>

class Color
{
public:
	Color() = default;
	Color(uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a) : b(_b), g(_g), r(_r), a(_a) {}
	explicit Color(int color) : b(color), g(color >> 8), r(color >> 16), a(color >> 24) {};
	
	Color& operator=(const Color& color)
	{
		r = color.r;
		g = color.g;
		b = color.b;
		a = color.a;
		return *this;
	}

	Color& operator=(int color)
	{
		r = color >> 16;
		g = color >> 8;
		b = color;
		a = color >> 24;

		return *this;
	}

	operator int()
	{
		return (r << 16) | (g << 8) | b | (a << 24);
	}

	static Color RGBA(uint32_t rgba)
	{
		return Color(rgba >> 16, rgba >> 8, rgba, rgba >> 24);
	}

	static Color RGB(uint32_t rgb)
	{
		return Color(rgb >> 16, rgb >> 8, rgb, 1);
	}

	uint8_t b = 0;
	uint8_t g = 0;
	uint8_t r = 0;
	uint8_t a = 0;
};
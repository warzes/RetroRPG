#pragma once

#include "Window.h"

class Draw
{
public:
	Draw(DrawingContext context, int width, int heigth);
	~Draw();

	void SetContext(DrawingContext context);
	void SetPixels(const void* data);

	int Width;  // Width of context (in pixels)
	int Height; // Height of context (in pixels)

private:
	DrawingContext m_frontContext;
	Handle m_bitmap = 0;
};
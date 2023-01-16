#pragma once

// Bitmap image container & manipulator	

#include "Core.h"
#include "Color.h"

enum BITMAP_FLAGS
{
	BITMAP_RGB = 0,          // Bitmap in 32bit RGB color format
	BITMAP_RGBA = 1,         // Bitmap in 32bit RGBA format
	BITMAP_PREMULTIPLIED = 2 // Bitmap in 32bit RGBA (alpha pre-multiplied) format
};

class BmpFont;

// Contain and manage a RGB or RGBA 32bit bitmap image
class Bitmap
{
public:
	Bitmap();
	~Bitmap();

	// Clear the image with the specified color
	void Clear(Color color);
	// Fill a rectangle with the specified color
	void Rect(int32_t x, int32_t y, int32_t w, int32_t h, Color color);
	// Copy an image portion to the image
	void Blit(int32_t xDst, int32_t yDst, const Bitmap* src, int32_t xSrc, int32_t ySrc, int32_t w, int32_t h);
	// Copy an image portion to the image (premultiplied alpha format)
	void AlphaBlit(int32_t xDst, int32_t yDst, const Bitmap* src, int32_t xSrc, int32_t ySrc, int32_t w, int32_t h);
	// Copy and scale an image portion to the image (premultiplied alpha format)
	void AlphaScaleBlit(int32_t xDst, int32_t yDst, int32_t wDst, int32_t hDst, const Bitmap* src, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc);

	// Write a short text with the specified bitmap character set
	void Text(int x, int y, const char* text, int length, const BmpFont* font);

	void Allocate(int tx, int ty);
	void Deallocate();

	void PreMultiply();
	void MakeMipmaps();

	Handle context = 0;           // Handle available for graphic contexts
	Handle bitmap = 0;            // Handle available for bitmap

	int tx = 0;                   // Horizontal size of image in pixels
	int ty = 0;                   // Vertical size of image in pixels
	int txP2 = 0;                 // Horizontal size (power of 2)
	int tyP2 = 0;                 // Vertical size (power of 2)
	int flags = BITMAP_RGB;       // Image format and attributes

	void* data = nullptr;         // Pointer to raw data
	bool dataAllocated = false;   // Has data been allocated?

	Bitmap* mipmaps[BMP_MIPMAPS]; // Table of mipmaps (bitmap pointers)
	int mmLevels = 0;             // No of mipmaps
};

// Contain and manage a monospace bitmap font
class BmpFont
{
public:
	Bitmap* font = nullptr; // Bitmap with character set

	int charSizeX = 20;     // Horizontal size of character
	int charSizeY = 30;     // Vertical size of character
	int charBegin = 32;     // First character in set (ascii code)
	int charEnd = 128;      // Last character in set (ascii code)
	int spaceX = 16;        // Horizontal space between characters
	int spaceY = 30;        // Vertical space between characters
};
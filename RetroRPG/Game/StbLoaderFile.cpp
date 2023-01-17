#include "StbLoaderFile.h"
#include "Bitmap.h"
#include <stdlib.h>
#include <string.h>

#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_TGA
#define STBI_NO_GIF
#define STBI_NO_HDR
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_LINEAR
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
//-----------------------------------------------------------------------------
StbFile::StbFile(const char* filename)
{
	if( filename ) m_path = _strdup(filename);
}
//-----------------------------------------------------------------------------
StbFile::~StbFile()
{
	if( m_path ) free(m_path);
}
//-----------------------------------------------------------------------------
Bitmap* StbFile::Load()
{
	// TODO: ðàçíûå ôîðìàòû êîìïîíåíò öâåòà.

	int desiredÑhannels = STBI_rgb_alpha;
	//stbi_set_flip_vertically_on_load(verticallyFlip ? 1 : 0);
	int width = 0;
	int height = 0;
	int comps = 0;
	stbi_uc* pixelData = stbi_load(m_path, &width, &height, &comps, desiredÑhannels);

	Bitmap* bitmap = new Bitmap();
	bitmap->tx = width;
	bitmap->ty = height;
	bitmap->txP2 = Global::log2i32(bitmap->tx);
	bitmap->tyP2 = Global::log2i32(bitmap->ty);
	bitmap->flags = BITMAP_RGBA;
	bitmap->data = new Color[bitmap->tx * bitmap->ty];
	bitmap->dataAllocated = true;

	uint8_t* data = (uint8_t *)bitmap->data;
	unsigned long curId = 0;
	for( int i = 0; i < width * height * 4; i += 4 )
	{
		Color* d = (Color*)data + curId;
		d->r = pixelData[i + 0];
		d->g = pixelData[i + 1];
		d->b = pixelData[i + 2];
		d->a = pixelData[i + 3];
		curId++;
	}
	return bitmap;
}
//-----------------------------------------------------------------------------
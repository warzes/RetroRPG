#include "BmpFile.h"
#include <stdlib.h>
#include <string.h>
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct BMPFILEHEADER
{
	uint16_t bfType;
	uint32_t bfSize;
	int16_t  bfReserved1;
	int16_t  bfReserved2;
	uint32_t bfOffBits;
};
#pragma pack(pop)
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct BMPINFOHEADER
{
	uint32_t biSize;
	int32_t  biWidth;
	int32_t  biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t  biXPelsPerMeter;
	int32_t  biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};
#pragma pack(pop)
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct BMPCOLORSPACE
{
	uint32_t CSType;
	uint32_t Endpoints[9];
	uint32_t GammaRed;
	uint32_t GammaGreen;
	uint32_t GammaBlue;
};
//-----------------------------------------------------------------------------
#pragma pack(push, 1)
struct BMPCOLORMASK
{
	uint32_t mR;
	uint32_t mG;
	uint32_t mB;
	uint32_t mA;
};
#pragma pack(pop)
//-----------------------------------------------------------------------------
#define BI_RGB				0
#define BI_BITFIELDS		3
#define BI_ALPHABITFIELDS	6
//-----------------------------------------------------------------------------
BmpFile::BmpFile(const char * filename)
{
	if( filename ) m_path = _strdup(filename);
}
//-----------------------------------------------------------------------------
BmpFile::~BmpFile()
{
	if( m_path ) free(m_path);
}
//-----------------------------------------------------------------------------
Bitmap* BmpFile::Load()
{
	FILE* file = fopen(m_path, "rb");
	if( !file )
	{
		printf("bmpFile: file not found %s!\n", m_path);
		return NULL;
	}

	Bitmap* bitmap = new Bitmap();
	readBitmap(file, bitmap);
	fclose(file);
	return bitmap;
}
//-----------------------------------------------------------------------------
void BmpFile::Save(const Bitmap* bitmap)
{
	FILE* file = fopen(m_path, "wb");
	if( !file )
	{
		printf("bmpFile: file cannot be opened %s!\n", m_path);
		return;
	}

	writeBitmap(file, bitmap);
	fclose(file);
}
//-----------------------------------------------------------------------------
int BmpFile::readBitmap(FILE* file, Bitmap* bitmap)
{
	BMPFILEHEADER fileHeader;
	BMPINFOHEADER infoHeader;
	BMPCOLORMASK  mask = 
	{
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000
	};

	// Read the headers
	fread(&fileHeader, sizeof(BMPFILEHEADER), 1, file);
	fread(&infoHeader, sizeof(BMPINFOHEADER), 1, file);

	// Format the data (little-endianness)
	FROM_U32(fileHeader.bfSize);
	FROM_U32(fileHeader.bfOffBits);
	FROM_U32(infoHeader.biSize);
	FROM_S32(infoHeader.biWidth);
	FROM_S32(infoHeader.biHeight);
	FROM_U16(infoHeader.biPlanes);
	FROM_U16(infoHeader.biBitCount);
	FROM_U32(infoHeader.biCompression);
	FROM_U32(infoHeader.biSizeImage);
	FROM_S32(infoHeader.biXPelsPerMeter);
	FROM_S32(infoHeader.biYPelsPerMeter);
	FROM_U32(infoHeader.biClrUsed);
	FROM_U32(infoHeader.biClrImportant);

	// Check the headers
	if( strncmp((char *)&fileHeader.bfType, "BM", 2) )
	{
		printf("bmpFile: file not a bitmap!\n");
		return 0;
	}
	if( infoHeader.biBitCount != 24 && infoHeader.biBitCount != 32 )
	{
		printf("bmpFile: only 24bit or 32bit bitmaps are supported %d!\n", infoHeader.biBitCount);
		return 0;
	}
	if( infoHeader.biCompression != BI_RGB && infoHeader.biCompression != BI_BITFIELDS )
	{
		printf("bmpFile: only uncompressed formats are supported!\n");
		return 0;
	}

	// Load the bitmasks
	int shiftA = 24;
	int shiftR = 16;
	int shiftG = 8;
	int shiftB = 0;
	if( infoHeader.biCompression == BI_BITFIELDS )
	{
		fread(&mask, sizeof(BMPCOLORMASK), 1, file);
		shiftR = __builtin_ffs(mask.mR) - 1;
		shiftG = __builtin_ffs(mask.mG) - 1;
		shiftB = __builtin_ffs(mask.mB) - 1;
		shiftA = __builtin_ffs(mask.mA) - 1;
	}

	// Retrieve bitmap size
	bitmap->tx = infoHeader.biWidth;
	bitmap->ty = infoHeader.biHeight;
	bitmap->txP2 = Global::log2i32(bitmap->tx);
	bitmap->tyP2 = Global::log2i32(bitmap->ty);

	int upsidedown = 1;
	if( bitmap->ty < 0 )
	{
		bitmap->ty = -bitmap->ty;
		upsidedown = 0;
	}

	// Set bitmap flags
	bitmap->flags = BITMAP_RGB;
	if( infoHeader.biBitCount == 32 )
		bitmap->flags |= BITMAP_RGBA;

	// Allocate bitmap memory
	int srcScan;
	srcScan = bitmap->tx * (infoHeader.biBitCount >> 3);
	srcScan = (srcScan + 0x3) & ~0x3;

	bitmap->data = new Color[bitmap->tx * bitmap->ty];
	bitmap->dataAllocated = true;

	// Load bitmap data
	uint8_t* buffer = (uint8_t *)alloca(srcScan);
	uint8_t* data = (uint8_t *)bitmap->data;

	int dstScan = bitmap->tx * sizeof(uint32_t);
	fseek(file, fileHeader.bfOffBits, SEEK_SET);
	if( upsidedown )
		data += dstScan * (bitmap->ty - 1);

	if( infoHeader.biBitCount == 32 )
	{
		// Parse a 32 bits image
		for( int y = 0; y < bitmap->ty; y++ )
		{
			fread(buffer, srcScan, 1, file);
			Color* d = (Color*)data;
			uint32_t* s = (uint32_t *)buffer;
			for( int i = 0; i < bitmap->tx; i++ )
			{
				uint32_t c = *s++;
				d->a = (uint8_t)((c & mask.mA) >> shiftA);
				d->r = (uint8_t)((c & mask.mR) >> shiftR);
				d->g = (uint8_t)((c & mask.mG) >> shiftG);
				d->b = (uint8_t)((c & mask.mB) >> shiftB);
				d++;
			}
			if( upsidedown ) data -= dstScan;
			else data += dstScan;
		}
	}
	else
	{
		// Parse a 24 bits image
		for( int y = 0; y < bitmap->ty; y++ )
		{
			fread(buffer, srcScan, 1, file);
			Color* d = (Color*)data;
			uint8_t	 * s = buffer;
			for( int i = 0; i < bitmap->tx; i++ )
			{
				d->b = *s++;
				d->g = *s++;
				d->r = *s++;
				d++;
			}
			if( upsidedown ) data -= dstScan;
			else data += dstScan;
		}
	}

	return 1;
}
//-----------------------------------------------------------------------------
int BmpFile::writeBitmap(FILE* file, const Bitmap * bitmap)
{
#define HEAD_LEN sizeof(BMPFILEHEADER) + sizeof(BMPINFOHEADER) + sizeof(BMPCOLORMASK)
	// Prepare the headers
	size_t size = bitmap->tx * bitmap->ty * sizeof(uint32_t);
	BMPFILEHEADER fileHeader;
	fileHeader.bfType = 0x4D42;
	fileHeader.bfSize = HEAD_LEN + size;
	fileHeader.bfReserved1 = 0;
	fileHeader.bfReserved2 = 0;
	fileHeader.bfOffBits = HEAD_LEN;

	BMPINFOHEADER infoHeader;
	infoHeader.biSize = sizeof(BMPINFOHEADER) + sizeof(BMPCOLORMASK);
	infoHeader.biWidth = bitmap->tx;
	infoHeader.biHeight = bitmap->ty;
	infoHeader.biPlanes = 1;
	infoHeader.biBitCount = 32;
	infoHeader.biCompression = BI_BITFIELDS;
	infoHeader.biSizeImage = 0;
	infoHeader.biXPelsPerMeter = 96;
	infoHeader.biYPelsPerMeter = 96;
	infoHeader.biClrUsed = 0;
	infoHeader.biClrImportant = 0;

	BMPCOLORMASK mask = {
		0x00FF0000,
		0x0000FF00,
		0x000000FF,
		0xFF000000
	};

	// Format the data (little-endianness)
	TO_U16(fileHeader.bfType);
	TO_U32(fileHeader.bfSize);
	TO_U32(fileHeader.bfOffBits);
	TO_U32(infoHeader.biSize);
	TO_S32(infoHeader.biWidth);
	TO_S32(infoHeader.biHeight);
	TO_U16(infoHeader.biPlanes);
	TO_U16(infoHeader.biBitCount);
	TO_U32(infoHeader.biCompression);
	TO_U32(infoHeader.biSizeImage);
	TO_S32(infoHeader.biXPelsPerMeter);
	TO_S32(infoHeader.biYPelsPerMeter);
	TO_U32(infoHeader.biClrUsed);
	TO_U32(infoHeader.biClrImportant);
	//TO_U32(mask[0]);
	//TO_U32(mask[1]);
	//TO_U32(mask[2]);
	//TO_U32(mask[3]);

// Write the headers
	fwrite(&fileHeader, sizeof(BMPFILEHEADER), 1, file);
	fwrite(&infoHeader, sizeof(BMPINFOHEADER), 1, file);
	fwrite(&mask, sizeof(BMPCOLORMASK), 1, file);

	// Save the picture
	uint32_t scan = bitmap->tx * sizeof(uint32_t);
	uint8_t* data = (uint8_t *)bitmap->data;
	for( int y = 0; y < bitmap->ty; y++ )
	{
		fwrite(data, scan, 1, file);
		data += scan;
	}
	return 0;
}
//-----------------------------------------------------------------------------
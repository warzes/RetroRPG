#define  _CRT_SECURE_NO_WARNINGS // TODO: delete
#include "ImageCache.h"
#include "BmpFile.h"
#ifdef _MSC_VER
#include "vs-dirent.h"
#else
#include <dirent.h>
#endif
#include <string.h>
//-----------------------------------------------------------------------------
ImageCache gImageCache;
//-----------------------------------------------------------------------------
ImageCache::ImageCache()
{
	memset(cacheSlots, 0, sizeof(Slot) * BMPCACHE_SLOTS);

	// Create the default bitmap (32x32 all white)
	Bitmap* defBitmap = new Bitmap();
	defBitmap->Allocate(32, 32);
	memset(defBitmap->data, 0xFF, 32 * 32 * sizeof(Color));

	// Register the default bitmap
	Slot* defSlot = &cacheSlots[0];
	defSlot->bitmap = defBitmap;
	strcpy(defSlot->path, "none");
	strcpy(defSlot->name, "default");
	defSlot->extras = nullptr;
	defSlot->numberExtras = 0;
	defSlot->cursor = 0;
	defSlot->flags = 0;
	numberSlots = 1;
}
//-----------------------------------------------------------------------------
ImageCache::~ImageCache()
{
	Clean();
}
//-----------------------------------------------------------------------------
void ImageCache::Clean()
{
	for( int i = 0; i < BMPCACHE_SLOTS; i++ )
		deleteSlot(i);
	numberSlots = 0;
}
//-----------------------------------------------------------------------------
Bitmap* ImageCache::LoadBMP(const char* path)
{
	if( numberSlots >= BMPCACHE_SLOTS )
	{
		printf("gImageCache: no free cacheSlots!\n");
		return NULL;
	}

	BmpFile bmpFile = BmpFile(path);
	Bitmap* bitmap = bmpFile.Load();
	if( !bitmap ) return NULL;

	int slot = createSlot(bitmap, path);
	if( slot < 0 )
	{
		delete bitmap;
		return NULL;
	}

	bitmap->MakeMipmaps();
	cacheSlots[slot].flags |= BMPCACHE_MIPMAPPED;

	if( bitmap->flags & BMPCACHE_RGBA )
	{
		bitmap->PreMultiply();
		cacheSlots[slot].flags |= BMPCACHE_RGBA;
	}

	return bitmap;
}
//-----------------------------------------------------------------------------
void ImageCache::LoadDirectory(const char* path)
{
	char ext[MAX_FILE_EXTENSION + 1];
	char filePath[MAX_FILE_PATH + 1];

	DIR* dir = opendir(path);
	struct dirent* dd;

	while( (dd = readdir(dir)) )
	{
		if( dd->d_name[0] == '.' ) continue;
		Global::getFileExtention(ext, MAX_FILE_EXTENSION, (const char*)dd->d_name);

		if( strcmp(ext, "bmp") == 0 )
		{
			// Load a Windows bmp file
			snprintf(filePath, MAX_FILE_PATH, "%s/%s", path, dd->d_name);
			filePath[MAX_FILE_PATH] = '\0';
			printf("gImageCache: loading bitmap: %s\n", filePath);
			LoadBMP(filePath);
		}
	}
	closedir(dir);
}
//-----------------------------------------------------------------------------
int ImageCache::createSlot(Bitmap* bitmap, const char* path)
{
	for( int i = 0; i < BMPCACHE_SLOTS; i++ )
	{
		Slot* slot = &cacheSlots[i];
		if( slot->bitmap ) continue;

		// Register the bitmap
		slot->bitmap = bitmap;
		strncpy(slot->path, path, MAX_FILE_PATH);
		slot->path[MAX_FILE_PATH] = '\0';
		Global::getFileName(slot->name, MAX_FILE_NAME, path);

		// Initialize the flags
		slot->extras = NULL;
		slot->numberExtras = 0;
		slot->cursor = 0;
		slot->flags = 0;

		numberSlots++;
		return i;
	}
	return -1;
}
//-----------------------------------------------------------------------------
void ImageCache::deleteSlot(int index)
{
	Slot* slot = &cacheSlots[index];
	if( slot->bitmap ) delete slot->bitmap;
	if( slot->extras ) delete[] slot->extras;
	memset(slot, 0, sizeof(Slot));
	numberSlots--;
}
//-----------------------------------------------------------------------------
int ImageCache::GetSlotFromName(const char* path)
{
	char name[MAX_FILE_NAME + 1];
	Global::getFileName(name, MAX_FILE_NAME, path);

	// Search for resource
	for( int i = 0; i < numberSlots; i++ )
	{
		Slot* slot = &cacheSlots[i];
		if( !slot->bitmap ) continue;
		if( strcmp(slot->name, name) == 0 )
			return i;
	}

	// Resource not found
	printf("gImageCache: %s not found!\n", path);
	return 0;
}
//-----------------------------------------------------------------------------
Bitmap* ImageCache::GetBitmapFromName(const char* path)
{
	int slot = GetSlotFromName(path);
	return cacheSlots[slot].bitmap;
}
//-----------------------------------------------------------------------------
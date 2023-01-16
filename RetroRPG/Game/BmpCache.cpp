#define  _CRT_SECURE_NO_WARNINGS // TODO: delete
#include "BmpCache.h"
#include "BmpFile.h"
#ifdef _MSC_VER
#include "vs-dirent.h"
#else
#include <dirent.h>
#endif
#include <string.h>
//-----------------------------------------------------------------------------
BmpCache bmpCache;
//-----------------------------------------------------------------------------
BmpCache::BmpCache()
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
	defSlot->extras = NULL;
	defSlot->noExtras = 0;
	defSlot->cursor = 0;
	defSlot->flags = 0;
	noSlots = 1;
}
//-----------------------------------------------------------------------------
BmpCache::~BmpCache()
{
	Clean();
}
//-----------------------------------------------------------------------------
void BmpCache::Clean()
{
	for( int i = 0; i < BMPCACHE_SLOTS; i++ )
		deleteSlot(i);
	noSlots = 0;
}
//-----------------------------------------------------------------------------
Bitmap* BmpCache::LoadBMP(const char* path)
{
	if( noSlots >= BMPCACHE_SLOTS )
	{
		printf("bmpCache: no free cacheSlots!\n");
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
void BmpCache::LoadDirectory(const char* path)
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
			printf("bmpCache: loading bitmap: %s\n", filePath);
			LoadBMP(filePath);
		}
	}
	closedir(dir);
}
//-----------------------------------------------------------------------------
int BmpCache::createSlot(Bitmap* bitmap, const char* path)
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
		slot->noExtras = 0;
		slot->cursor = 0;
		slot->flags = 0;

		noSlots++;
		return i;
	}
	return -1;
}
//-----------------------------------------------------------------------------
void BmpCache::deleteSlot(int index)
{
	Slot* slot = &cacheSlots[index];
	if( slot->bitmap ) delete slot->bitmap;
	if( slot->extras ) delete[] slot->extras;
	memset(slot, 0, sizeof(Slot));
	noSlots--;
}
//-----------------------------------------------------------------------------
int BmpCache::GetSlotFromName(const char* path)
{
	char name[MAX_FILE_NAME + 1];
	Global::getFileName(name, MAX_FILE_NAME, path);

	// Search for resource
	for( int i = 0; i < noSlots; i++ )
	{
		Slot* slot = &cacheSlots[i];
		if( !slot->bitmap ) continue;
		if( strcmp(slot->name, name) == 0 )
			return i;
	}

	// Resource not found
	printf("bmpCache: %s not found!\n", path);
	return 0;
}
//-----------------------------------------------------------------------------
Bitmap* BmpCache::GetBitmapFromName(const char* path)
{
	int slot = GetSlotFromName(path);
	return cacheSlots[slot].bitmap;
}
//-----------------------------------------------------------------------------
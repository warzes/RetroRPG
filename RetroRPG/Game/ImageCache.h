#pragma once

// Image cache manager

#include "Bitmap.h"

enum BMPCACHE_FLAGS
{
	BMPCACHE_RGB = 0x00,       // Bitmap in 32bit RGB color format
	BMPCACHE_RGBA = 0x01,      // Bitmap in 32bit RGBA (alpha pre-multiplied) format
	BMPCACHE_ANIMATION = 0x02, // Bitmap with animation (uses cursor & extra bitmaps)
	BMPCACHE_MIPMAPPED = 0x04, // Bitmap with computed mipmaps
};

// Cache and inventory all image loaded
class ImageCache
{
public:
	ImageCache();
	~ImageCache();

	void Clean();

	void LoadDirectory(const char* path);
	Bitmap* LoadBMP(const char* path);

	int GetSlotFromName(const char* name);
	Bitmap* GetBitmapFromName(const char* name);

	// represents a bitmap cache slot
	struct Slot
	{
		Bitmap* bitmap;               // Bitmap associated to the slot
		char path[MAX_FILE_PATH + 1]; // Bitmap file full path
		char name[MAX_FILE_NAME + 1]; // Bitmap file name
		int flags;                    // Bitmap format and attributes

		Bitmap* extras;               // Extra bitmaps (for animation)
		int numberExtras;             // Number of extra bitmaps
		int cursor;                   // Cursor for animation playback
	};
	Slot cacheSlots[BMPCACHE_SLOTS];  // Slots in cache
	int numberSlots = 0;              // Number of cacheSlots in cache

private:
	int createSlot(Bitmap* bitmap, const char* path);
	void deleteSlot(int slot);
};

extern ImageCache gImageCache;
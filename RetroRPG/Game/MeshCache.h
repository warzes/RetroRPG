#pragma once

#include "Core.h"
#include "Mesh.h"

class MeshCache
{
public:
	MeshCache();
	~MeshCache();

	void Clean();

	void LoadDirectory(const char* path);
	Mesh* LoadOBJ(const char* path);

	int GetSlotFromName(const char* name);
	Mesh* GetMeshFromName(const char* path);

	struct Slot
	{
		Mesh* mesh;                   // Mesh associated to the slot
		char path[MAX_FILE_PATH + 1]; // Bitmap file full path
		char name[MAX_FILE_NAME + 1]; // Bitmap file name
		int flags;                    // Bitmap format and attributes
	};

	Slot cacheSlots[MESHCACHE_SLOTS]; // Slots in cache
	int noSlots = 0;                  // Number of cacheSlots in cache

private:
	int createSlot(Mesh* mesh, const char* path);
	void deleteSlot(int slot);
};

extern MeshCache meshCache;
#include "MeshCache.h"
#include "ObjFile.h"
#ifdef _MSC_VER
#include "vs-dirent.h"
#else
#include <dirent.h>
#endif
//-----------------------------------------------------------------------------
MeshCache meshCache;
//-----------------------------------------------------------------------------
MeshCache::MeshCache() 
{
	memset(cacheSlots, 0, sizeof(Slot) * MESHCACHE_SLOTS);

	// Create a default mesh
	Mesh* defMesh = new Mesh();

	// Register the default mesh
	Slot* defSlot = &cacheSlots[0];
	defSlot->mesh = defMesh;
	strcpy(defSlot->path, "none");
	strcpy(defSlot->name, "default");
	defSlot->flags = 0;

	noSlots = 1;
}
//-----------------------------------------------------------------------------
MeshCache::~MeshCache()
{
	Clean();
}
//-----------------------------------------------------------------------------
void MeshCache::Clean()
{
	for( int i = 0; i < MESHCACHE_SLOTS; i++ )
		deleteSlot(i);
	noSlots = 0;
}
//-----------------------------------------------------------------------------
Mesh* MeshCache::LoadOBJ(const char* path)
{
	if( noSlots >= MESHCACHE_SLOTS )
	{
		printf("meshCache: no free cacheSlots!\n");
		return NULL;
	}

	ObjFile objFile = ObjFile(path);
	Mesh* mesh = objFile.Load(0);
	if( !mesh ) return NULL;

	if( createSlot(mesh, path) < 0 )
	{
		delete mesh;
		return NULL;
	}
	return mesh;
}
//-----------------------------------------------------------------------------
void MeshCache::LoadDirectory(const char * path)
{
	char ext[MAX_FILE_EXTENSION + 1];
	char filePath[MAX_FILE_PATH + 1];

	DIR* dir = opendir(path);
	struct dirent* dd;

	while( (dd = readdir(dir)) )
	{
		if( dd->d_name[0] == '.' ) continue;
		Global::getFileExtention(ext, MAX_FILE_EXTENSION, (const char*)dd->d_name);

		if( strcmp(ext, "obj") == 0 )
		{
			// Load a Wavefront obj file
			snprintf(filePath, MAX_FILE_PATH, "%s/%s", path, dd->d_name);
			filePath[MAX_FILE_PATH] = '\0';
			printf("meshCache: loading mesh: %s\n", filePath);
			LoadOBJ(filePath);
		}

	}
	closedir(dir);
}
//-----------------------------------------------------------------------------
int MeshCache::createSlot(Mesh* mesh, const char* path)
{
	for( int i = 0; i < MESHCACHE_SLOTS; i++ )
	{
		Slot* slot = &cacheSlots[i];
		if( slot->mesh ) continue;

		// Register the mesh
		slot->mesh = mesh;
		strncpy(slot->path, path, MAX_FILE_PATH);
		slot->path[MAX_FILE_PATH] = '\0';
		Global::getFileName(slot->name, MAX_FILE_NAME, path);

		// Initialize the flags
		slot->flags = 0;

		noSlots++;
		return i;
	}
	return -1;
}
//-----------------------------------------------------------------------------
void MeshCache::deleteSlot(int index)
{
	Slot* slot = &cacheSlots[index];
	if( slot->mesh ) delete slot->mesh;
	memset(slot, 0, sizeof(Slot));
	noSlots--;
}
//-----------------------------------------------------------------------------
int MeshCache::GetSlotFromName(const char* path)
{
	char name[MAX_FILE_NAME + 1];
	Global::getFileName(name, MAX_FILE_NAME, path);

	// Search for resource
	for( int i = 0; i < noSlots; i++ )
	{
		Slot* slot = &cacheSlots[i];
		if( !slot->mesh ) continue;
		if( strcmp(slot->name, name) == 0 )
			return i;
	}

	// Resource not found
	printf("meshCache: %s not found!\n", path);
	return 0;
}
//-----------------------------------------------------------------------------
Mesh* MeshCache::GetMeshFromName(const char* path)
{
	int slot = GetSlotFromName(path);
	return cacheSlots[slot].mesh;
}
//-----------------------------------------------------------------------------
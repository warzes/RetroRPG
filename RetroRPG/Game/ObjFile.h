#pragma once

// Wavefront object file importer

#include "Core.h"
#include "Geometry.h"
#include "Color.h"
#include "Mesh.h"

#include <stdio.h>
#include <string.h>

// Contain and manage Wavefront object files materials
struct ObjMaterial
{
	ObjMaterial()
	{
		strcpy(name, "default");
		texture[0] = '\0';
	}

	char name[OBJ_MAX_NAME + 1];           // Name of the material
	Color ambient = { 255, 255, 255, 0 };  // Ambient color of the material
	Color diffuse = { 255, 255, 255, 0 };  // Diffuse color of the material
	Color specular = { 255, 255, 255, 0 }; // Specular color of the material
	float shininess = 0.0f;                // Shininess factor of the material
	float transparency = 0.0f;             // Transparency factor of the material
	char texture[OBJ_MAX_NAME + 1];        // Name of first texture of the material
};

// Load 3D meshes in Wavefront object format
class ObjFile
{
public:
	ObjFile(const char* filename);
	~ObjFile();

	int GetNoMeshes();
	const char* GetMeshName(int index);

	Mesh* Load(int index);
	void Save(const Mesh* mesh);

private:
	void importMeshAllocate(FILE* file, Mesh* mesh);
	void importMeshData(FILE* file, Mesh* mesh);

	void exportHeader(FILE* file, const Mesh* mesh);
	void exportVertexes(FILE* file, const Mesh* mesh);
	void exportNormals(FILE* file, const Mesh* mesh);
	void exportTexCoords(FILE* file, const Mesh* mesh);
	void exportTriangles(FILE* file, const Mesh* mesh);
	void exportMaterials(FILE* file, const Mesh* mesh);

	void loadMaterialLibraries(FILE* file);
	void importMaterialAllocate(FILE* file);
	void importMaterialData(FILE* file);
	ObjMaterial* getMaterialFromName(const char* name);

	void readVertex(Mesh* mesh, int index);
	void readNormal(Mesh* mesh, int index);
	void readTriangle(Mesh* mesh, int index);
	void readTexCoord(Mesh* mesh, int index);
	void readColor(const char* buffer, Color& color);
	int	readLine(FILE* file);

	char* m_path = nullptr;             // File path of the mesh

	ObjMaterial* m_materials = nullptr; // Internal - file materials
	ObjMaterial* m_curMaterial;         // Currently selected material
	int m_noMaterials = 0;              // Internal - number of materials

	Vertex* m_normals = nullptr;        // Internal - object normals
	int m_noNormals = 0;                // Internal - number of normals

	char m_line[OBJ_MAX_LINE + 1];      // Line buffer (for parsing the file)
};
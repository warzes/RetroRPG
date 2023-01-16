#pragma once

#include "Core.h"
#include "Color.h"
#include "Geometry.h"

// Contain and manage a 3D mesh
class Mesh
{
public:
	Mesh();
	~Mesh();

	// Duplicate the mesh without copying its static data
	void ShadowCopy(Mesh* copy) const;
	void Copy(Mesh* copy) const;

	void Allocate(size_t noVertexes, size_t noTexCoords, size_t noTriangles);
	void Deallocate();

	void Transform(const Matrix& matrix);
	void SetMatrix(const Matrix& matrix);
	void UpdateMatrix();

	void ComputeNormals();
	void AllocateNormals();
	void AllocateShades();

	// Overall positioning
	Matrix view{};                       // View matrix of the mesh
	Vertex pos{};                        // Position of the mesh
	Vertex scale = { 1.0f, 1.0f, 1.0f }; // Scaling of the mesh
	Vertex angle{};                      // Absolute angle of the mesh (in degrees)

	// Static mesh data
	char name[OBJ_MAX_NAME + 1];         // Mesh name
	Vertex* vertexes = nullptr;          // Vertex positions (x, y, z) of the mesh
	size_t noVertexes = 0;               // Number of vertexes in the mesh
	float* texCoords = nullptr;          // Texture coordinates (u, v) of the mesh
	size_t noTexCoords = 0;                 // Number of texture coordinates in the mesh
	int* vertexesList = nullptr;         // Triangles - vertex indexes tripplets
	int* texCoordsList = nullptr;        // Triangles - texture coordinate indexes tripplets
	int* texSlotList = nullptr;          // Triangles - texture slots
	Color* colors = nullptr;             // Triangles - colors
	size_t noTriangles = 0;              // Number of triangles in the mesh

	// Computed mesh data
	Vertex* normals = nullptr;           // Normal vector per triangle
	Color* shades = nullptr;             // Shade color per triangle (lighting)

	bool allocated = false;              // Has data been allocated
};
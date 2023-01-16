#pragma once

#include "Core.h"
#include "Color.h"

// Represent a quadratic fog
struct FogDesc
{
	Color color;                         // fog basic color
	float near = -RENDERER_NEAR_DEFAULT; // fog start distance
	float far = -100.0f;                 // fog end distance
};

// Rendering flags per triangle
enum TRIANGLE_FLAGS
{
	TRIANGLE_DEFAULT = 0,   // default triangle type
	TRIANGLE_TEXTURED = 1,  // apply single layer texturing
	TRIANGLE_MIPMAPPED = 2, // apply mipmap filtering
	TRIANGLE_FOGGED = 4,    // apply per-fragment quadratic fog
	TRIANGLE_BLENDED = 8,   // apply alpha blending (for textures with alpha channel)
};

// Represent a rasterizable triangle
struct Triangle
{
	float xs[4];        // x coordinate of vertexes
	float ys[4];        // y coordinate of vertexes
	float zs[4];        // z coordinate of vertexes
	float us[4];        // u texture coordinate of vertexes
	float vs[4];        // v texture coordinate of vertexes
	float vd;           // average view distance

	Color solidColor;   // solid color
	int diffuseTexture; // diffuse texture slot
	int flags;          // extra flags
};

// Contain and manage triangle lists
class TriangleList
{
public:
	TriangleList();
	TriangleList(size_t noTrangles);
	~TriangleList();

	void ZSort();

	FogDesc fog;                   // associated quadratic fog model

	int* srcIndices = nullptr;     // array of triangle source indexes
	int* dstIndices = nullptr;     // array of triangle destination indexes
	Triangle* triangles = nullptr; // array of triangles

	size_t noAllocated = 0;        // number of allocated triangles
	int noUsed = 0;                // number of used triangles
	int noValid = 0;               // number of valid triangles

private:
	void allocate(size_t noTriangles);
	void zMergeSort(int indices[], int tmp[], int nb);
};
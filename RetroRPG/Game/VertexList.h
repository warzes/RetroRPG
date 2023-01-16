#pragma once

#include "Core.h"
#include "Geometry.h"

// Contain and manage vertex lists
class VertexList
{
public:
	VertexList();
	VertexList(size_t noVertexes);
	~VertexList();

	Vertex* vertexes = nullptr; // array of vertexes
	size_t noAllocated = 0;     // number of allocated vertexes
	int noUsed = 0;             // number of used vertexes

private:
	void allocate(size_t noVertexes);
};
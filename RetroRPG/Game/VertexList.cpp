#include "VertexList.h"
//-----------------------------------------------------------------------------
VertexList::VertexList()
{
	allocate(VERLIST_MAX);
}
//-----------------------------------------------------------------------------
VertexList::VertexList(size_t noVertexes)
{
	allocate(noVertexes);
}
//-----------------------------------------------------------------------------
VertexList::~VertexList()
{
	delete[] vertexes;
}
//-----------------------------------------------------------------------------
void VertexList::allocate(size_t noVertexes)
{
	vertexes = new Vertex[noVertexes];
	noAllocated = noVertexes;
}
//-----------------------------------------------------------------------------
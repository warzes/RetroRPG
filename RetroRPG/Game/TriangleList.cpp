#include "TriangleList.h"
//-----------------------------------------------------------------------------
TriangleList::TriangleList()
{
	allocate(TRILIST_MAX);
}
//-----------------------------------------------------------------------------
TriangleList::TriangleList(size_t noTriangles)
{
	allocate(noTriangles);
}
//-----------------------------------------------------------------------------
TriangleList::~TriangleList()
{
	delete[] triangles;
	delete[] srcIndices;
	delete[] dstIndices;
}
//-----------------------------------------------------------------------------
void TriangleList::allocate(size_t noTriangles)
{
	triangles = new Triangle[noTriangles];
	srcIndices = new int[noTriangles * 3];
	dstIndices = new int[noTriangles * 3];
	noAllocated = noTriangles;
}
//-----------------------------------------------------------------------------
void TriangleList::ZSort()
{
	if( !noValid ) return;
	zMergeSort(srcIndices, dstIndices, noValid);
}
//-----------------------------------------------------------------------------
void TriangleList::zMergeSort(int indices[], int tmp[], int nb)
{
	const int h1 = nb >> 1;
	const int h2 = nb - h1;
	if( h1 >= 2 ) zMergeSort(&indices[0], &tmp[0], h1);
	if( h2 >= 2 ) zMergeSort(&indices[h1], &tmp[h1], h2);
	int u = 0;
	int v = h1;
	float a = triangles[indices[u]].vd;
	float b = triangles[indices[v]].vd;
	for( int i = 0; i < nb; i++ )
	{
		if( a > b )
		{
			tmp[i] = indices[u++];
			if( u == h1 )
			{
				for( i++; i < nb; i++ )
					tmp[i] = indices[v++];
				for( int i = 0; i < nb; i++ ) // TODO: возможно заменить на j
					indices[i] = tmp[i];
				return;
			}
			a = triangles[indices[u]].vd;

		}
		else
		{
			tmp[i] = indices[v++];
			if( v == nb )
			{
				for( i++; i < nb; i++ )
					tmp[i] = indices[u++];
				for( int i = 0; i < nb; i++ ) // TODO: возможно заменить на j
					indices[i] = tmp[i];
				return;
			}
			b = triangles[indices[v]].vd;
		}
	}
}
//-----------------------------------------------------------------------------
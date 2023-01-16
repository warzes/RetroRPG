#include "Mesh.h"
#include <stdlib.h>
#include <string.h>
//-----------------------------------------------------------------------------
Mesh::Mesh()
{
	memset(name, 0, OBJ_MAX_NAME + 1);
	UpdateMatrix();
}
//-----------------------------------------------------------------------------
Mesh::~Mesh()
{
	Deallocate();
}
//-----------------------------------------------------------------------------
void Mesh::ShadowCopy(Mesh* copy) const
{
	if (copy->allocated) copy->Deallocate();

	copy->vertexes = vertexes;
	copy->noVertexes = noVertexes;
	copy->texCoords = texCoords;
	copy->noTexCoords = noTexCoords;
	copy->vertexesList = vertexesList;
	copy->texCoordsList = texCoordsList;
	copy->texSlotList = texSlotList;
	copy->colors = colors;
	copy->noTriangles = noTriangles;
	copy->allocated = false;

	if (normals)
	{
		copy->normals = new Vertex[noTriangles];
		memcpy(copy->normals, normals, noTriangles * sizeof(Vertex));
	}
	if (shades)
	{
		copy->shades = new Color[noTriangles];
		memcpy(copy->shades, shades, noTriangles * sizeof(Color));
	}
}
//-----------------------------------------------------------------------------
void Mesh::Copy(Mesh* copy) const
{
	if (copy->allocated) copy->Deallocate();

	copy->Allocate(noVertexes, noTexCoords, noTriangles);

	memcpy(copy->vertexes, vertexes, noVertexes * sizeof(Vertex));
	memcpy(copy->texCoords, texCoords, noTexCoords * sizeof(float) * 2);
	memcpy(copy->vertexesList, vertexesList, noTriangles * sizeof(int) * 3);
	memcpy(copy->texCoordsList, texCoordsList, noTriangles * sizeof(int) * 3);
	memcpy(copy->texSlotList, texSlotList, noTriangles * sizeof(int));
	memcpy(copy->colors, colors, noTriangles * sizeof(uint32_t));

	if (normals)
	{
		copy->normals = new Vertex[noTriangles];
		memcpy(copy->normals, normals, noTriangles * sizeof(Vertex));
	}
	if (shades)
	{
		copy->shades = new Color[noTriangles];
		memcpy(copy->shades, shades, noTriangles * sizeof(Color));
	}
}
//-----------------------------------------------------------------------------
void Mesh::Allocate(size_t numVertexes, size_t numTexCoords, size_t numTriangles)
{
	if( allocated ) Deallocate();

	vertexes = new Vertex[numVertexes];
	this->noVertexes = numVertexes;

	texCoords = new float[numTexCoords * 2];
	memset(texCoords, 0, sizeof(float) * numTexCoords * 2);
	this->noTexCoords = numTexCoords;

	vertexesList = new int[numTriangles * 3];
	texCoordsList = new int[numTriangles * 3];
	texSlotList = new int[numTriangles];
	colors = new Color[numTriangles];

	memset(vertexesList, 0, sizeof(int) * numTriangles * 3);
	memset(texCoordsList, 0, sizeof(int) * numTriangles * 3);
	memset(texSlotList, 0, sizeof(int) * numTriangles);
	memset(colors, 0xFF, sizeof(Color) * numTriangles);

	this->noTriangles = numTriangles;
	allocated = true;
}
//-----------------------------------------------------------------------------
void Mesh::Deallocate()
{
	// Deallocate static data
	if( allocated )
	{
		delete vertexes;
		vertexes = nullptr;
		noVertexes = 0;
		delete texCoords;
		texCoords = nullptr;
		noTexCoords = 0;
		delete vertexesList;
		vertexesList = nullptr;
		delete texCoordsList;
		texCoordsList = nullptr;
		delete texSlotList;
		texSlotList = nullptr;
		delete colors;
		colors = nullptr;

		noTriangles = 0;
		allocated = false;
	}

	// Deallocate temporary data
	delete normals;
	normals = nullptr;
	delete shades;
	shades = nullptr;
}
//-----------------------------------------------------------------------------
void Mesh::Transform(const Matrix &matrix)
{
	UpdateMatrix();
	view = matrix * view;
}
//-----------------------------------------------------------------------------
void Mesh::SetMatrix(const Matrix &matrix)
{
	view = matrix;
}
//-----------------------------------------------------------------------------
void Mesh::UpdateMatrix()
{
	view.Identity();
	view.Scale(scale);
	view.RotateEulerZYX(angle * d2r);
	view.Translate(pos);
}
//-----------------------------------------------------------------------------
void Mesh::ComputeNormals()
{
	if( !normals ) AllocateNormals();
	for(size_t i = 0; i < noTriangles; i++ )
	{
		Vertex v1 = vertexes[vertexesList[i * 3]];
		Vertex v2 = vertexes[vertexesList[i * 3 + 1]];
		Vertex v3 = vertexes[vertexesList[i * 3 + 2]];
		Vertex a = v2 - v1;
		Vertex b = v3 - v1;
		Vertex c = a.Cross(b);
		c.Normalize();
		normals[i] = c;
	}
}
//-----------------------------------------------------------------------------
void Mesh::AllocateNormals()
{
	if( !normals )
	{
		normals = new Vertex[noTriangles];
		memset(normals, 0x00, noTriangles * sizeof(Vertex));
	}
}
//-----------------------------------------------------------------------------
void Mesh::AllocateShades()
{
	if( !shades )
	{
		shades = new Color[noTriangles];
		memset(shades, 0xFF, noTriangles * sizeof(Color));
	}
}
//-----------------------------------------------------------------------------
#include "Billboard.h"
#include <string.h>
//-----------------------------------------------------------------------------
BillboardSet::BillboardSet() 
{
	UpdateMatrix();
}
//-----------------------------------------------------------------------------
BillboardSet::BillboardSet(int noBillboards)
{
	Allocate(noBillboards);
	Basic();
	UpdateMatrix();
}
//-----------------------------------------------------------------------------
BillboardSet::~BillboardSet()
{
	Deallocate();
}
//-----------------------------------------------------------------------------
void BillboardSet::ShadowCopy(BillboardSet* copy) const
{
	if( copy->allocated ) copy->Deallocate();

	copy->places = places;
	copy->sizes = sizes;
	copy->colors = colors;
	copy->texSlots = texSlots;
	copy->flags = flags;

	if( shades )
	{
		copy->shades = new Color[noBillboards];
		memcpy(copy->shades, shades, noBillboards * sizeof(Color));
	}
}
//-----------------------------------------------------------------------------
void BillboardSet::Copy(BillboardSet* copy) const
{
	if( copy->allocated ) copy->Deallocate();

	copy->Allocate(noBillboards);

	memcpy(copy->places, places, noBillboards * sizeof(Vertex));
	memcpy(copy->sizes, sizes, noBillboards * sizeof(float) * 2);
	memcpy(copy->colors, colors, noBillboards * sizeof(uint32_t));
	memcpy(copy->texSlots, texSlots, noBillboards * sizeof(int));
	memcpy(copy->flags, flags, noBillboards * sizeof(int));
}
//-----------------------------------------------------------------------------
void BillboardSet::Basic()
{
	for( int b = 0; b < noBillboards; b++ )
	{
		sizes[b * 2 + 0] = 1.0f;
		sizes[b * 2 + 1] = 1.0f;
		colors[b] = Color::RGB(0xFFFFFF);
		texSlots[b] = 0;
		flags[b] = BSET_EXIST;
	}
}
//-----------------------------------------------------------------------------
void BillboardSet::Clear()
{
	for( int b = 0; b < noBillboards; b++ )
		flags[b] = 0;
}
//-----------------------------------------------------------------------------
void BillboardSet::Allocate(int noBillboards)
{
	if( allocated ) Deallocate();

	places = new Vertex[noBillboards];
	sizes = new float[noBillboards * 2];
	colors = new Color[noBillboards];
	texSlots = new int[noBillboards];
	flags = new int[noBillboards];

	this->noBillboards = noBillboards;
	allocated = false;
}
//-----------------------------------------------------------------------------
void BillboardSet::Deallocate()
{
	if( allocated )
	{
		delete[] places;
		places = nullptr;
		delete[] sizes;
		sizes = nullptr;
		delete[] colors;
		colors = nullptr;
		delete[] texSlots;
		texSlots = nullptr;
		delete[] flags;
		flags = nullptr;

		noBillboards = 0;
		allocated = false;
	}

	delete[] shades;
	shades = nullptr;
}
//-----------------------------------------------------------------------------
void BillboardSet::Transform(const Matrix &matrix)
{
	UpdateMatrix();
	view = matrix * view;
}
//-----------------------------------------------------------------------------
void BillboardSet::SetMatrix(const Matrix &matrix)
{
	view = matrix;
}
//-----------------------------------------------------------------------------
void BillboardSet::UpdateMatrix()
{
	view.Identity();
	view.Scale(scale);
	view.RotateEulerYZX(angle * d2r);
	view.Translate(pos);
}
//-----------------------------------------------------------------------------

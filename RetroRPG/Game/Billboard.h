#pragma once

// Billboard set container and manipulator

#include "Core.h"
#include "Color.h"
#include "Geometry.h"

enum BSET_FLAGS
{
	BSET_EXIST = 0x01, // Billboard exist
};

// Contain and manage a billboard set
class BillboardSet
{
public:
	BillboardSet();
	BillboardSet(int noBillboards);
	~BillboardSet();

	void ShadowCopy(BillboardSet* copy) const;
	void Copy(BillboardSet* copy) const;

	void Basic();
	void Clear();

	void Allocate(int noBillboards);
	void Deallocate();

	void Transform(const Matrix& matrix);
	void SetMatrix(const Matrix& matrix);
	void UpdateMatrix();

	// Overall positioning
	Matrix view{};                       // View matrix of billboard set
	Vertex pos{};                        // Position of billboard set
	Vertex scale = { 1.0f, 1.0f, 1.0f }; // Scaling of billboard set
	Vertex angle{};                      // Absolute angle of billboard set (in degrees)

	// Static billboards data
	Vertex* places = nullptr;            // Position per billboard
	float* sizes = nullptr;              // Size (x, y) per billboard
	Color* colors = nullptr;             // Color per billboard
	int* texSlots = nullptr;             // Texture slot per billboard
	int* flags = nullptr;                // Flag per billboard

	int noBillboards = 0;                // Number of allocated billboards

	// Computed billboards data
	Color* shades = nullptr;             // Shade color per billboard (lighting)
	bool allocated = false;              // Has data been allocated
};
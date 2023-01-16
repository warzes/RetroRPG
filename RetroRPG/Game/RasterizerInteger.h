#pragma once

// Triangle rasterizer (fixed point)

#if RENDERER_INTRASTER

#include "DrawingContext.h"
#include "Geometry.h"
#include "TriangleList.h"
// Intrinsics includes
#if USE_SSE2
#	include <xmmintrin.h>
#	include <emmintrin.h>
#endif // USE_SSE2
#include "Bitmap.h"

class Rasterizer
{
public:
	Rasterizer(int width, int height);
	~Rasterizer();

	void RasterList(TriangleList* trilist);
	const void* GetPixels() { return m_pixels; }
	void Flush();

	Bitmap frame;     // frame buffer
	Color background; // background color

private:
	inline void fillTriangleZC(int vi1, int vi2, int vi3, bool top);
	inline void fillFlatTexZC(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2);
	inline void fillFlatTexZCFog(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2);
	inline void fillFlatTexAlphaZC(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2);
	inline void fillFlatTexAlphaZCFog(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2);

	Color* m_pixels = nullptr;            // frame pixel buffer
	Color* m_texDiffusePixels = nullptr;  // diffuse texture pixel buffer
	uint32_t m_texSizeU = 0;              // textures horizontal size
	uint32_t m_texSizeV = 0;              // textures vertical size
	uint32_t m_texMaskU = 0;              // textures horizontal mask
	uint32_t m_texMaskV = 0;              // textures vertical mask

	Triangle* m_curTriangle = nullptr;    // current triangle
	TriangleList* m_curTrilist = nullptr; // current triangle list

#if USE_SIMD && USE_SSE2
	__m128i m_color_4;
#endif // USE_SIMD && USE_SSE2

	int32_t m_xs[4], m_ys[4], m_ws[4];
	int32_t m_us[4], m_vs[4];
};

#endif // RENDERER_INTRASTER
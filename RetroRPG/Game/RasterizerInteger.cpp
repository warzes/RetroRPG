#include "Rasterizer.h"
#if RENDERER_INTRASTER
#include "Rasterizer.h"
#include "BmpCache.h"

#include <stdlib.h>
#include <string.h>
#include <float.h>
//-----------------------------------------------------------------------------
Rasterizer::Rasterizer(int width, int height) 
{
	memset(m_xs, 0, sizeof(int32_t) * 4);
	memset(m_ys, 0, sizeof(int32_t) * 4);
	memset(m_ws, 0, sizeof(int32_t) * 4);
	memset(m_us, 0, sizeof(int32_t) * 4);
	memset(m_vs, 0, sizeof(int32_t) * 4);

	frame.Allocate(width, height);
	frame.Clear(Color());
	m_pixels = (Color*)frame.data;
}
//-----------------------------------------------------------------------------
Rasterizer::~Rasterizer()
{
	frame.Deallocate();
}
//-----------------------------------------------------------------------------
void Rasterizer::RasterList(TriangleList* trilist)
{
	trilist->ZSort();

#if defined(__i386__) || defined(_M_IX86) || defined(_X86_) || defined(__x86_64__) || defined(_M_X64)
#if defined(_WIN32)
	_controlfp(_MCW_RC, _RC_CHOP);
	_controlfp(_MCW_DN, _DN_FLUSH);
#endif
#endif

	m_curTrilist = trilist;
	for (int i = 0; i < trilist->noValid; i++)
	{
		m_curTriangle = &trilist->triangles[trilist->srcIndices[i]];

		// Retrieve the material
		BmpCache::Slot* slot = &bmpCache.cacheSlots[m_curTriangle->diffuseTexture];
		Bitmap* bmp = slot->bitmap;
		if (slot->flags & BMPCACHE_ANIMATION)
			bmp = &slot->extras[slot->cursor];

		// Convert position coordinates
		m_xs[0] = cmbound((int32_t)(m_curTriangle->xs[0] + 0.5f), 0, frame.tx) << 16;
		m_xs[1] = cmbound((int32_t)(m_curTriangle->xs[1] + 0.5f), 0, frame.tx) << 16;
		m_xs[2] = cmbound((int32_t)(m_curTriangle->xs[2] + 0.5f), 0, frame.tx) << 16;
		m_ys[0] = cmbound((int32_t)(m_curTriangle->ys[0] + 0.5f), 0, frame.ty);
		m_ys[1] = cmbound((int32_t)(m_curTriangle->ys[1] + 0.5f), 0, frame.ty);
		m_ys[2] = cmbound((int32_t)(m_curTriangle->ys[2] + 0.5f), 0, frame.ty);

		const float sw = 0x1p30;
		m_ws[0] = (int32_t)(m_curTriangle->zs[0] * sw);
		m_ws[1] = (int32_t)(m_curTriangle->zs[1] * sw);
		m_ws[2] = (int32_t)(m_curTriangle->zs[2] * sw);

		// Sort vertexes vertically
		int vt = 0, vb = 0, vm1 = 0, vm2 = 3;
		if (m_ys[0] < m_ys[1])
		{
			if (m_ys[0] < m_ys[2])
			{
				vt = 0;
				if (m_ys[1] < m_ys[2]) { vm1 = 1; vb = 2; }
				else { vm1 = 2; vb = 1; }
			}
			else
			{
				vt = 2;	vm1 = 0; vb = 1;
			}
		}
		else
		{
			if (m_ys[1] < m_ys[2])
			{
				vt = 1;
				if (m_ys[0] < m_ys[2]) { vm1 = 0; vb = 2; }
				else { vm1 = 2; vb = 0; }
			}
			else
			{
				vt = 2; vm1 = 1; vb = 0;
			}
		}

		// Get vertical span
		int dy = m_ys[vb] - m_ys[vt];
		if (dy == 0) continue;

		// Choose the mipmap level
		if (m_curTriangle->flags & TRIANGLE_MIPMAPPED)
		{
			if (bmp->mmLevels)
			{
				float utop = m_curTriangle->us[vt] / m_curTriangle->zs[vt];
				float ubot = m_curTriangle->us[vb] / m_curTriangle->zs[vb];
				float vtop = m_curTriangle->vs[vt] / m_curTriangle->zs[vt];
				float vbot = m_curTriangle->vs[vb] / m_curTriangle->zs[vb];
				float d = cmmax(fabsf(utop - ubot), fabsf(vtop - vbot));

				int r = (int)((d * bmp->ty + dy * 0.5f) / dy);
				int l = Global::log2i32(r);
				l = cmmin(l, bmp->mmLevels - 1);
				bmp = bmp->mipmaps[l];
			}
		}

		// Retrieve texture information
		m_texDiffusePixels = (Color*)bmp->data;
		m_texSizeU = bmp->txP2;
		m_texSizeV = bmp->tyP2;
		m_texMaskU = (1 << bmp->txP2) - 1;
		m_texMaskV = (1 << bmp->tyP2) - 1;

		// Architecture specific pre-calculations
#if USE_SIMD && USE_SSE2
		__m128i zv = _mm_set1_epi32(0);
		m_color_4 = _mm_loadu_si128((__m128i*) & m_curTriangle->solidColor);
		m_color_4 = _mm_unpacklo_epi32(m_color_4, m_color_4);
		m_color_4 = _mm_unpacklo_epi8(m_color_4, zv);
#endif	// USE_SIMD && USE_SSE2

		// Convert texture coordinates
		const float su = (float)(65536 << bmp->txP2);
		m_us[0] = (int32_t)(m_curTriangle->us[0] * su);
		m_us[1] = (int32_t)(m_curTriangle->us[1] * su);
		m_us[2] = (int32_t)(m_curTriangle->us[2] * su);

		const float sv = (float)(65536 << bmp->tyP2);
		m_vs[0] = (int32_t)(m_curTriangle->vs[0] * sv);
		m_vs[1] = (int32_t)(m_curTriangle->vs[1] * sv);
		m_vs[2] = (int32_t)(m_curTriangle->vs[2] * sv);

		// Compute the mean vertex
		int n = ((m_ys[vm1] - m_ys[vt]) << 16) / dy;
		m_xs[3] = (((int64_t)(m_xs[vb] - m_xs[vt]) * n) >> 16) + m_xs[vt];
		m_ys[3] = m_ys[vm1];
		m_ws[3] = (((int64_t)(m_ws[vb] - m_ws[vt]) * n) >> 16) + m_ws[vt];
		m_us[3] = (((int64_t)(m_us[vb] - m_us[vt]) * n) >> 16) + m_us[vt];
		m_vs[3] = (((int64_t)(m_vs[vb] - m_vs[vt]) * n) >> 16) + m_vs[vt];

		// Sort vertexes horizontally
		int dx = m_xs[vm2] - m_xs[vm1];
		if (dx < 0) { int t = vm1; vm1 = vm2; vm2 = t; }

		// Render the triangle
		fillTriangleZC(vt, vm1, vm2, true);
		fillTriangleZC(vm1, vm2, vb, false);
	}
}
//-----------------------------------------------------------------------------
void Rasterizer::Flush()
{
	frame.Clear(background);
}
//-----------------------------------------------------------------------------
void Rasterizer::fillTriangleZC(int vi1, int vi2, int vi3, bool top)
{
	int	ax1, aw1, au1, av1;
	int	ax2, aw2, au2, av2;
	int x1, x2, y1, y2, w1, w2;
	int u1, u2, v1, v2;

	if (top) 
	{
		// Top triangle
		int d = m_ys[vi2] - m_ys[vi1];
		if (d == 0) return;

		ax1 = (m_xs[vi2] - m_xs[vi1]) / d;
		aw1 = (m_ws[vi2] - m_ws[vi1]) / d;
		au1 = (m_us[vi2] - m_us[vi1]) / d;
		av1 = (m_vs[vi2] - m_vs[vi1]) / d;

		ax2 = (m_xs[vi3] - m_xs[vi1]) / d;
		aw2 = (m_ws[vi3] - m_ws[vi1]) / d;
		au2 = (m_us[vi3] - m_us[vi1]) / d;
		av2 = (m_vs[vi3] - m_vs[vi1]) / d;

		x1 = m_xs[vi1];
		x2 = x1;
		w1 = m_ws[vi1];
		w2 = w1;

		u1 = m_us[vi1];
		u2 = u1;
		v1 = m_vs[vi1];
		v2 = v1;

		y1 = m_ys[vi1];
		y2 = m_ys[vi2];
		x2 += 0xFFFF;
	}
	else 
	{
		// Bottom triangle
		int d = m_ys[vi3] - m_ys[vi1];
		if (d == 0) return;

		ax1 = (m_xs[vi3] - m_xs[vi1]) / d;
		aw1 = (m_ws[vi3] - m_ws[vi1]) / d;
		au1 = (m_us[vi3] - m_us[vi1]) / d;
		av1 = (m_vs[vi3] - m_vs[vi1]) / d;

		ax2 = (m_xs[vi3] - m_xs[vi2]) / d;
		aw2 = (m_ws[vi3] - m_ws[vi2]) / d;
		au2 = (m_us[vi3] - m_us[vi2]) / d;
		av2 = (m_vs[vi3] - m_vs[vi2]) / d;

		x1 = m_xs[vi1];
		w1 = m_ws[vi1];
		u1 = m_us[vi1];
		v1 = m_vs[vi1];

		x2 = m_xs[vi2];
		w2 = m_ws[vi2];
		u2 = m_us[vi2];
		v2 = m_vs[vi2];

		y1 = m_ys[vi1];
		y2 = m_ys[vi3];
		x2 += 0xFFFF;
	}

	if (m_curTriangle->flags & TRIANGLE_BLENDED) 
	{
		if (m_curTriangle->flags & TRIANGLE_FOGGED) 
		{
			for (int y = y1; y < y2; y++) 
			{
				fillFlatTexAlphaZCFog(y, x1 >> 16, x2 >> 16, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
		else 
		{
			for (int y = y1; y < y2; y++) 
			{
				fillFlatTexAlphaZC(y, x1 >> 16, x2 >> 16, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
	}
	else 
	{
		if (m_curTriangle->flags & TRIANGLE_FOGGED) 
		{
			for (int y = y1; y < y2; y++) 
			{
				fillFlatTexZCFog(y, x1 >> 16, x2 >> 16, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
		else
		{
			for (int y = y1; y < y2; y++) 
			{
				fillFlatTexZC(y, x1 >> 16, x2 >> 16, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Filler (sse/integer) - flat textured z-corrected scans
inline void Rasterizer::fillFlatTexZC(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2)
{
#if USE_SIMD && USE_SSE2
	int d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	if (++x2 > frame.tx) x2 = frame.tx;
	Color* p = x1 + y * frame.tx + m_pixels;

	for (int x = x1; x < x2; x++) 
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & m_texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & m_texMaskV;

		__m128i zv = _mm_set1_epi32(0);
		__m128i tp;
		tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[tu + (tv << m_texSizeU)]);
		tp = _mm_unpacklo_epi8(tp, zv);
		tp = _mm_mullo_epi16(tp, m_color_4);
		tp = _mm_srli_epi16(tp, 8);
		tp = _mm_packus_epi16(tp, zv);
		*p++ = _mm_cvtsi128_si32(tp);

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#else
	uint8_t* sc = (uint8_t*)&curTriangle->solidColor;

	short d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	if (++x2 > frame.tx) x2 = frame.tx;
	uint8_t* p = (uint8_t*)(x1 + y * frame.tx + pixels);

	for (int x = x1; x < x2; x++) 
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		p[0] = (t[0] * sc[0]) >> 8;
		p[1] = (t[1] * sc[1]) >> 8;
		p[2] = (t[2] * sc[2]) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
// Filler (sse/integer) - flat textured z-corrected scans with fog
inline void Rasterizer::fillFlatTexZCFog(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2)
{
#if USE_SIMD && USE_SSE2
	// TODO: SSE Implementation
	uint8_t* sc = (uint8_t*)&m_curTriangle->solidColor;
	uint8_t* fc = (uint8_t*)&m_curTrilist->fog.color;

	short d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	const float sw = 0x1p8;
	int32_t znear = (int32_t)(m_curTrilist->fog.near * sw);
	int32_t zfar = (int32_t)(m_curTrilist->fog.far * sw);
	int32_t zscale = (1 << 30) / (zfar - znear);

	uint8_t* p = (uint8_t*)(x1 + y * frame.tx + m_pixels);

	for (int x = x1; x <= x2; x++) 
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & m_texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & m_texMaskV;
		uint8_t* t = (uint8_t*)&m_texDiffusePixels[tu + (tv << m_texSizeU)];

		int r = (t[0] * sc[0]) >> 8;
		int g = (t[1] * sc[1]) >> 8;
		int b = (t[2] * sc[2]) >> 8;

		int32_t ff = ((int64_t)(z - znear) * zscale) >> 15;
		ff = cmmax(0, ff);
		ff = cmmin((1 << 15), ff);
		int fb = (ff * ff) >> (14 + 8);

		p[0] = r + (((fc[0] - r) * fb) >> 8);
		p[1] = g + (((fc[1] - g) * fb) >> 8);
		p[2] = b + (((fc[2] - b) * fb) >> 8);
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#else
	uint8_t* sc = (uint8_t*)&curTriangle->solidColor;
	uint8_t* fc = (uint8_t*)&curTrilist->fog.color;

	short d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	const float sw = 0x1p8;
	int32_t znear = (int32_t)(curTrilist->fog.near * sw);
	int32_t zfar = (int32_t)(curTrilist->fog.far * sw);
	int32_t zscale = (1 << 30) / (zfar - znear);

	uint8_t* p = (uint8_t*)(x1 + y * frame.tx + pixels);

	for (int x = x1; x <= x2; x++) 
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		int r = (t[0] * sc[0]) >> 8;
		int g = (t[1] * sc[1]) >> 8;
		int b = (t[2] * sc[2]) >> 8;

		int32_t ff = ((int64_t)(z - znear) * zscale) >> 15;
		ff = cmmax(0, ff);
		ff = cmmin((1 << 15), ff);
		int fb = (ff * ff) >> (14 + 8);

		p[0] = r + (((fc[0] - r) * fb) >> 8);
		p[1] = g + (((fc[1] - g) * fb) >> 8);
		p[2] = b + (((fc[2] - b) * fb) >> 8);
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
// Filler (sse/integer) - flat textured & alpha blended z-corrected scans
inline void Rasterizer::fillFlatTexAlphaZC(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2)
{
#if USE_SIMD && USE_SSE2
	int d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	if (++x2 > frame.tx) x2 = frame.tx;
	Color* p = x1 + y * frame.tx + m_pixels;

	__m128i sc = _mm_set1_epi32(0x01000100);
	for (int x = x1; x < x2; x++) 
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & m_texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & m_texMaskV;

		__m128i zv = _mm_set1_epi32(0);
		__m128i tp, fp;
		fp = _mm_loadl_epi64((__m128i*) p);
		tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[tu + (tv << m_texSizeU)]);
		fp = _mm_unpacklo_epi8(fp, zv);
		tp = _mm_unpacklo_epi8(tp, zv);

		__m128i ap;
		ap = _mm_shufflelo_epi16(tp, 0xFF);
		ap = _mm_sub_epi16(sc, ap);

		tp = _mm_mullo_epi16(tp, m_color_4);
		fp = _mm_mullo_epi16(fp, ap);
		tp = _mm_adds_epu16(tp, fp);
		tp = _mm_srli_epi16(tp, 8);
		tp = _mm_packus_epi16(tp, zv);
		*p++ = _mm_cvtsi128_si32(tp);

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#else
	uint8_t* sc = (uint8_t*)&curTriangle->solidColor;

	int d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	if (++x2 > frame.tx) x2 = frame.tx;
	uint8_t* p = (uint8_t*)(x1 + y * frame.tx + pixels);

	for (int x = x1; x < x2; x++) 
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		uint16_t a = 256 - t[3];
		p[0] = (p[0] * a + t[0] * sc[0]) >> 8;
		p[1] = (p[1] * a + t[1] * sc[1]) >> 8;
		p[2] = (p[2] * a + t[2] * sc[2]) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
// Filler (sse/integer) - flat textured & alpha blended z-corrected scans with fog
inline void Rasterizer::fillFlatTexAlphaZCFog(int y, int x1, int x2, int w1, int w2, int u1, int u2, int v1, int v2)
{
#if USE_SIMD && USE_SSE2
	// TODO: SSE Implementation
	uint8_t* sc = (uint8_t*)&m_curTriangle->solidColor;
	uint8_t* fc = (uint8_t*)&m_curTrilist->fog.color;

	int d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	const float sw = 0x1p8;
	int32_t znear = (int32_t)(m_curTrilist->fog.near * sw);
	int32_t zfar = (int32_t)(m_curTrilist->fog.far * sw);
	int32_t zscale = (1 << 30) / (zfar - znear);

	if (++x2 > frame.tx) x2 = frame.tx;
	uint8_t* p = (uint8_t*)(x1 + y * frame.tx + m_pixels);

	for (int x = x1; x < x2; x++)
	{
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & m_texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & m_texMaskV;
		uint8_t* t = (uint8_t*)&m_texDiffusePixels[tu + (tv << m_texSizeU)];

		int32_t ff = ((int64_t)(z - znear) * zscale) >> 15;
		ff = cmmax(0, ff);
		ff = cmmin((1 << 15), ff);
		int fb = (ff * ff) >> (14 + 8);

		int n = t[3];
		int a = 256 - n;

		int r = t[0] * sc[0];
		int g = t[1] * sc[1];
		int b = t[2] * sc[2];
		r = r + (((fc[0] * n - r) * fb) >> 8);
		g = g + (((fc[1] * n - g) * fb) >> 8);
		b = b + (((fc[2] * n - b) * fb) >> 8);

		p[0] = (p[0] * a + r) >> 8;
		p[1] = (p[1] * a + g) >> 8;
		p[2] = (p[2] * a + b) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#else
	uint8_t* sc = (uint8_t*)&curTriangle->solidColor;
	uint8_t* fc = (uint8_t*)&curTrilist->fog.color;

	int d = x2 - x1;
	if (d == 0) return;

	int au = (u2 - u1) / d;
	int av = (v2 - v1) / d;
	int aw = (w2 - w1) / d;

	const float sw = 0x1p8;
	int32_t znear = (int32_t)(curTrilist->fog.near * sw);
	int32_t zfar = (int32_t)(curTrilist->fog.far * sw);
	int32_t zscale = (1 << 30) / (zfar - znear);

	if (++x2 > frame.tx) x2 = frame.tx;
	uint8_t* p = (uint8_t*)(x1 + y * frame.tx + pixels);

	for (int x = x1; x < x2; x++) {
		int32_t z = (1 << 30) / (w1 >> 8);
		uint32_t tu = (((int64_t)u1 * z) >> 24) & texMaskU;
		uint32_t tv = (((int64_t)v1 * z) >> 24) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		int32_t ff = ((int64_t)(z - znear) * zscale) >> 15;
		ff = cmmax(0, ff);
		ff = cmmin((1 << 15), ff);
		int fb = (ff * ff) >> (14 + 8);

		int n = t[3];
		int a = 256 - n;

		int r = t[0] * sc[0];
		int g = t[1] * sc[1];
		int b = t[2] * sc[2];
		r = r + (((fc[0] * n - r) * fb) >> 8);
		g = g + (((fc[1] * n - g) * fb) >> 8);
		b = b + (((fc[2] * n - b) * fb) >> 8);

		p[0] = (p[0] * a + r) >> 8;
		p[1] = (p[1] * a + g) >> 8;
		p[2] = (p[2] * a + b) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
#endif // RENDERER_INTRASTER
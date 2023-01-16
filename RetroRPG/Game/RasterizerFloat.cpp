#include "Rasterizer.h"
#if !RENDERER_INTRASTER
#include "BmpCache.h"
#include <string.h>
#include <float.h>
//-----------------------------------------------------------------------------
Rasterizer::Rasterizer(int width, int height)
{
	memset(m_xs, 0, sizeof(float) * 4);
	memset(m_ys, 0, sizeof(float) * 4);
	memset(m_ws, 0, sizeof(float) * 4);
	memset(m_us, 0, sizeof(float) * 4);
	memset(m_vs, 0, sizeof(float) * 4);

	frame.Allocate(width, height);
	frame.Clear(background);
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
		const float ftx = (float)frame.tx;
		const float fty = (float)frame.ty;
		m_xs[0] = cmbound(floorf(m_curTriangle->xs[0] + 0.5f), 0.0f, ftx);
		m_xs[1] = cmbound(floorf(m_curTriangle->xs[1] + 0.5f), 0.0f, ftx);
		m_xs[2] = cmbound(floorf(m_curTriangle->xs[2] + 0.5f), 0.0f, ftx);
		m_ys[0] = cmbound(floorf(m_curTriangle->ys[0] + 0.5f), 0.0f, fty);
		m_ys[1] = cmbound(floorf(m_curTriangle->ys[1] + 0.5f), 0.0f, fty);
		m_ys[2] = cmbound(floorf(m_curTriangle->ys[2] + 0.5f), 0.0f, fty);
		m_ws[0] = m_curTriangle->zs[0];
		m_ws[1] = m_curTriangle->zs[1];
		m_ws[2] = m_curTriangle->zs[2];

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
		const float dy = m_ys[vb] - m_ys[vt];
		if (dy == 0.0f) continue;

		// Choose the mipmap level
		if (m_curTriangle->flags & TRIANGLE_MIPMAPPED)
		{
			if (bmp->mmLevels)
			{
				const float utop = m_curTriangle->us[vt] / m_curTriangle->zs[vt];
				const float ubot = m_curTriangle->us[vb] / m_curTriangle->zs[vb];
				const float vtop = m_curTriangle->vs[vt] / m_curTriangle->zs[vt];
				const float vbot = m_curTriangle->vs[vb] / m_curTriangle->zs[vb];
				const float d = cmmax(fabsf(utop - ubot), fabsf(vtop - vbot));

				const int r = (int)((d * bmp->ty + dy * 0.5f) / dy);
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
		const float texSizeUFloat = (float)(1 << m_texSizeU);
		m_texScale_4 = _mm_set1_ps(texSizeUFloat);
		m_texMaskU_4 = _mm_set1_epi32(m_texMaskU);
		m_texMaskV_4 = _mm_set1_epi32(m_texMaskV << m_texSizeU);

		__m128i zv = _mm_set1_epi32(0);
		m_color_4 = _mm_loadu_si128((__m128i*) & m_curTriangle->solidColor);
		m_color_4 = _mm_unpacklo_epi32(m_color_4, m_color_4);
		m_color_4 = _mm_unpacklo_epi8(m_color_4, zv);
#endif	// USE_SIMD && USE_SSE2

		// Convert texture coordinates
		const float sx = (float)(1 << bmp->txP2);
		m_us[0] = m_curTriangle->us[0] * sx;
		m_us[1] = m_curTriangle->us[1] * sx;
		m_us[2] = m_curTriangle->us[2] * sx;

		const float sy = (float)(1 << bmp->tyP2);
		m_vs[0] = m_curTriangle->vs[0] * sy;
		m_vs[1] = m_curTriangle->vs[1] * sy;
		m_vs[2] = m_curTriangle->vs[2] * sy;

		// Compute the mean vertex
		const float n = (m_ys[vm1] - m_ys[vt]) / dy;
		m_xs[3] = (m_xs[vb] - m_xs[vt]) * n + m_xs[vt];
		m_ys[3] = m_ys[vm1];
		m_ws[3] = (m_ws[vb] - m_ws[vt]) * n + m_ws[vt];
		m_us[3] = (m_us[vb] - m_us[vt]) * n + m_us[vt];
		m_vs[3] = (m_vs[vb] - m_vs[vt]) * n + m_vs[vt];

		// Sort vertexes horizontally
		const int dx = (int)(m_xs[vm2] - m_xs[vm1]);
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
	float ax1, aw1, au1, av1;
	float ax2, aw2, au2, av2;
	int y1, y2;
	float x1, x2, w1, w2;
	float u1, u2, v1, v2;

	if( top )
	{
		// Top triangle
		const float d = m_ys[vi2] - m_ys[vi1];
		if( d == 0.0f ) return;

		const float id = 1.0f / d;
		ax1 = (m_xs[vi2] - m_xs[vi1]) * id;
		aw1 = (m_ws[vi2] - m_ws[vi1]) * id;
		au1 = (m_us[vi2] - m_us[vi1]) * id;
		av1 = (m_vs[vi2] - m_vs[vi1]) * id;
		ax2 = (m_xs[vi3] - m_xs[vi1]) * id;
		aw2 = (m_ws[vi3] - m_ws[vi1]) * id;
		au2 = (m_us[vi3] - m_us[vi1]) * id;
		av2 = (m_vs[vi3] - m_vs[vi1]) * id;

		x1 = m_xs[vi1];
		x2 = x1;
		w1 = m_ws[vi1];
		w2 = w1;
		u1 = m_us[vi1];
		u2 = u1;
		v1 = m_vs[vi1];
		v2 = v1;

		y1 = (int)m_ys[vi1];
		y2 = (int)m_ys[vi2];
	}
	else
	{
		// Bottom triangle
		const float d = m_ys[vi3] - m_ys[vi1];
		if( d == 0.0f ) return;

		const float id = 1.0f / d;
		ax1 = (m_xs[vi3] - m_xs[vi1]) * id;
		aw1 = (m_ws[vi3] - m_ws[vi1]) * id;
		au1 = (m_us[vi3] - m_us[vi1]) * id;
		av1 = (m_vs[vi3] - m_vs[vi1]) * id;
		ax2 = (m_xs[vi3] - m_xs[vi2]) * id;
		aw2 = (m_ws[vi3] - m_ws[vi2]) * id;
		au2 = (m_us[vi3] - m_us[vi2]) * id;
		av2 = (m_vs[vi3] - m_vs[vi2]) * id;

		x1 = m_xs[vi1];
		w1 = m_ws[vi1];
		u1 = m_us[vi1];
		v1 = m_vs[vi1];

		x2 = m_xs[vi2];
		w2 = m_ws[vi2];
		u2 = m_us[vi2];
		v2 = m_vs[vi2];

		y1 = (int)m_ys[vi1];
		y2 = (int)m_ys[vi3];
	}

	if( m_curTriangle->flags & TRIANGLE_BLENDED )
	{
		if( m_curTriangle->flags & TRIANGLE_FOGGED )
		{
			for( int y = y1; y < y2; y++ )
			{
				fillFlatTexAlphaZCFog(y, x1, x2, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
		else
		{
			for( int y = y1; y < y2; y++ )
			{
				fillFlatTexAlphaZC(y, x1, x2, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
	}
	else
	{
		if( m_curTriangle->flags & TRIANGLE_FOGGED )
		{
			for( int y = y1; y < y2; y++ )
			{
				fillFlatTexZCFog(y, x1, x2, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
		else
		{
			for( int y = y1; y < y2; y++ )
			{
				fillFlatTexZC(y, x1, x2, w1, w2, u1, u2, v1, v2);
				x1 += ax1; x2 += ax2;
				u1 += au1; u2 += au2;
				v1 += av1; v2 += av2;
				w1 += aw1; w2 += aw2;
			}
		}
	}
}
//-----------------------------------------------------------------------------
// Filler (sse/float) - flat textured z-corrected scans
inline void Rasterizer::fillFlatTexZC(int y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
#if USE_SIMD && USE_SSE2
	const float d = x2 - x1;
	if (d == 0.0f) return;

	const float id = 1.0f / d;
	const float au = (u2 - u1) * id;
	const float av = (v2 - v1) * id;
	const float aw = (w2 - w1) * id;

	__m128 u_4 = _mm_set_ps(u1 + 3.0f * au, u1 + 2.0f * au, u1 + au, u1);
	__m128 v_4 = _mm_set_ps(v1 + 3.0f * av, v1 + 2.0f * av, v1 + av, v1);
	__m128 w_4 = _mm_set_ps(w1 + 3.0f * aw, w1 + 2.0f * aw, w1 + aw, w1);

	__m128 au_4 = _mm_set1_ps(au * 4.0f);
	__m128 av_4 = _mm_set1_ps(av * 4.0f);
	__m128 aw_4 = _mm_set1_ps(aw * 4.0f);

	const int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	Color* p = xb + ((int)y) * frame.tx + m_pixels;
	const int b = (xe - xb) >> 2;
	const int r = (xe - xb) & 0x3;

	for (int x = 0; x < b; x++)
	{
		__m128 z_4 = _mm_rcp_ps(w_4);

		__m128 mu_4, mv_4;
		mu_4 = _mm_mul_ps(u_4, z_4);
		mv_4 = _mm_mul_ps(v_4, z_4);
		mv_4 = _mm_mul_ps(mv_4, m_texScale_4);

		__m128i mui_4, mvi_4;
		mui_4 = _mm_cvtps_epi32(mu_4);
		mvi_4 = _mm_cvtps_epi32(mv_4);
		mui_4 = _mm_and_si128(mui_4, m_texMaskU_4);
		mvi_4 = _mm_and_si128(mvi_4, m_texMaskV_4);
		mui_4 = _mm_add_epi32(mui_4, mvi_4);

		__m128i zv = _mm_set1_epi32(0);
		__m128i tp, tq, t1, t2;
		tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[0]]);
		tq = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[1]]);
		t1 = _mm_unpacklo_epi32(tp, tq);
		t1 = _mm_unpacklo_epi8(t1, zv);
		t1 = _mm_mullo_epi16(t1, m_color_4);
		t1 = _mm_srli_epi16(t1, 8);

		tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[2]]);
		tq = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[3]]);
		t2 = _mm_unpacklo_epi32(tp, tq);
		t2 = _mm_unpacklo_epi8(t2, zv);
		t2 = _mm_mullo_epi16(t2, m_color_4);
		t2 = _mm_srli_epi16(t2, 8);

		tp = _mm_packus_epi16(t1, t2);
		_mm_storeu_si128((__m128i*) p, tp);
		p += 4;

		w_4 = _mm_add_ps(w_4, aw_4);
		u_4 = _mm_add_ps(u_4, au_4);
		v_4 = _mm_add_ps(v_4, av_4);
	}

	if (r == 0) return;
	__m128 z_4 = _mm_rcp_ps(w_4);

	__m128 mu_4, mv_4;
	mu_4 = _mm_mul_ps(u_4, z_4);
	mv_4 = _mm_mul_ps(v_4, z_4);
	mv_4 = _mm_mul_ps(mv_4, m_texScale_4);

	__m128i mui_4, mvi_4;
	mui_4 = _mm_cvtps_epi32(mu_4);
	mvi_4 = _mm_cvtps_epi32(mv_4);
	mui_4 = _mm_and_si128(mui_4, m_texMaskU_4);
	mvi_4 = _mm_and_si128(mvi_4, m_texMaskV_4);
	mui_4 = _mm_add_epi32(mui_4, mvi_4);

	__m128i zv = _mm_set1_epi32(0);
	__m128i tp;
	tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[0]]);
	tp = _mm_unpacklo_epi8(tp, zv);
	tp = _mm_mullo_epi16(tp, m_color_4);
	tp = _mm_srli_epi16(tp, 8);
	tp = _mm_packus_epi16(tp, zv);
	*p++ = _mm_cvtsi128_si32(tp);

	if (r == 1) return;
	tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[1]]);
	tp = _mm_unpacklo_epi8(tp, zv);
	tp = _mm_mullo_epi16(tp, m_color_4);
	tp = _mm_srli_epi16(tp, 8);
	tp = _mm_packus_epi16(tp, zv);
	*p++ = _mm_cvtsi128_si32(tp);

	if (r == 2) return;
	tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[2]]);
	tp = _mm_unpacklo_epi8(tp, zv);
	tp = _mm_mullo_epi16(tp, m_color_4);
	tp = _mm_srli_epi16(tp, 8);
	tp = _mm_packus_epi16(tp, zv);
	*p++ = _mm_cvtsi128_si32(tp);
#else
	uint8_t* c = (uint8_t*)&curTriangle->solidColor;

	float d = x2 - x1;
	if (d == 0.0f) return;

	float au = (u2 - u1) / d;
	float av = (v2 - v1) / d;
	float aw = (w2 - w1) / d;

	int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	uint8_t* p = (uint8_t*)(xb + ((int)y) * frame.tx + pixels);

	for (int x = xb; x < xe; x++)
	{
		float z = 1.0f / w1;
		uint32_t tu = ((int32_t)(u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t)(v1 * z)) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		p[0] = (t[0] * c[0]) >> 8;
		p[1] = (t[1] * c[1]) >> 8;
		p[2] = (t[2] * c[2]) >> 8;
		p += 4;

		u1 += au;
		v1 += av;
		w1 += aw;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
// Filler (sse/float) - flat textured z-corrected scans with fog
inline void Rasterizer::fillFlatTexZCFog(int y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
#if USE_SIMD && USE_SSE2
	// TODO: SSE implementation
	uint8_t* sc = (uint8_t*)&m_curTriangle->solidColor;
	uint8_t* fc = (uint8_t*)&m_curTrilist->fog.color;

	const float d = x2 - x1;
	if (d == 0.0f) return;

	const float au = (u2 - u1) / d;
	const float av = (v2 - v1) / d;
	const float aw = (w2 - w1) / d;

	const float znear = m_curTrilist->fog.near;
	const float zfar = m_curTrilist->fog.far;
	const float zscale = -1.0f / (znear - zfar);

	const int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	uint8_t* p = (uint8_t*)(xb + y * frame.tx + m_pixels);

	for (int x = xb; x < xe; x++)
	{
		const float z = 1.0f / w1;
		uint32_t tu = ((int32_t)(u1 * z)) & m_texMaskU;
		uint32_t tv = ((int32_t)(v1 * z)) & m_texMaskV;
		uint8_t* t = (uint8_t*)&m_texDiffusePixels[tu + (tv << m_texSizeU)];

		const int r = (t[0] * sc[0]) >> 8;
		const int g = (t[1] * sc[1]) >> 8;
		const int b = (t[2] * sc[2]) >> 8;

		float ff = (z - znear) * zscale;
		ff = cmmax(0.0f, ff);
		ff = cmmin(1.0f, ff);
		ff = 256.0f * ff * ff;
		const int fb = (int)ff;

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

	float d = x2 - x1;
	if (d == 0.0f) return;

	float au = (u2 - u1) / d;
	float av = (v2 - v1) / d;
	float aw = (w2 - w1) / d;

	float znear = curTrilist->fog.near;
	float zfar = curTrilist->fog.far;
	float zscale = -1.0f / (znear - zfar);

	int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	uint8_t* p = (uint8_t*)(xb + y * frame.tx + pixels);

	for (int x = xb; x < xe; x++) {
		float z = 1.0f / w1;
		uint32_t tu = ((int32_t)(u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t)(v1 * z)) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		int r = (t[0] * sc[0]) >> 8;
		int g = (t[1] * sc[1]) >> 8;
		int b = (t[2] * sc[2]) >> 8;

		float ff = (z - znear) * zscale;
		ff = cmmax(0.0f, ff);
		ff = cmmin(1.0f, ff);
		ff = 256.0f * ff * ff;
		int fb = (int)ff;

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
// Filler (sse/float) - flat textured & alpha blended z-corrected scans
inline void Rasterizer::fillFlatTexAlphaZC(int y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
#if USE_SIMD && USE_SSE2
	const float d = x2 - x1;
	if (d == 0.0f) return;

	const float id = 1.0f / d;
	const float au = (u2 - u1) * id;
	const float av = (v2 - v1) * id;
	const float aw = (w2 - w1) * id;

	__m128 u_4 = _mm_set_ps(u1 + 3.0f * au, u1 + 2.0f * au, u1 + au, u1);
	__m128 v_4 = _mm_set_ps(v1 + 3.0f * av, v1 + 2.0f * av, v1 + av, v1);
	__m128 w_4 = _mm_set_ps(w1 + 3.0f * aw, w1 + 2.0f * aw, w1 + aw, w1);

	__m128 au_4 = _mm_set1_ps(au * 4.0f);
	__m128 av_4 = _mm_set1_ps(av * 4.0f);
	__m128 aw_4 = _mm_set1_ps(aw * 4.0f);

	const int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	Color* p = xb + ((int)y) * frame.tx + m_pixels;
	const int b = (xe - xb) >> 2;
	const int r = (xe - xb) & 0x3;

	__m128i sc = _mm_set1_epi32(0x01000100);
	for (int x = 0; x < b; x++) 
	{
		__m128 z_4 = _mm_rcp_ps(w_4);

		__m128 mu_4, mv_4;
		mu_4 = _mm_mul_ps(u_4, z_4);
		mv_4 = _mm_mul_ps(v_4, z_4);
		mv_4 = _mm_mul_ps(mv_4, m_texScale_4);

		__m128i mui_4, mvi_4;
		mui_4 = _mm_cvtps_epi32(mu_4);
		mvi_4 = _mm_cvtps_epi32(mv_4);
		mui_4 = _mm_and_si128(mui_4, m_texMaskU_4);
		mvi_4 = _mm_and_si128(mvi_4, m_texMaskV_4);
		mui_4 = _mm_add_epi32(mui_4, mvi_4);

		__m128i zv = _mm_set1_epi32(0);
		__m128i tp, tq, fp, t1, t2;
		__m128i ap, apl, aph;

		fp = _mm_loadl_epi64((__m128i*) p);
		tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[0]]);
		tq = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[1]]);
		t1 = _mm_unpacklo_epi32(tp, tq);
		fp = _mm_unpacklo_epi8(fp, zv);
		t1 = _mm_unpacklo_epi8(t1, zv);

		apl = _mm_shufflelo_epi16(t1, 0xFF);
		aph = _mm_shufflehi_epi16(t1, 0xFF);
		ap = _mm_castpd_si128(_mm_move_sd(_mm_castsi128_pd(aph), _mm_castsi128_pd(apl)));
		ap = _mm_sub_epi16(sc, ap);

		t1 = _mm_mullo_epi16(t1, m_color_4);
		fp = _mm_mullo_epi16(fp, ap);
		t1 = _mm_adds_epu16(t1, fp);
		t1 = _mm_srli_epi16(t1, 8);

		fp = _mm_loadl_epi64((__m128i*) (p + 2));
		tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[2]]);
		tq = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[3]]);
		t2 = _mm_unpacklo_epi32(tp, tq);
		fp = _mm_unpacklo_epi8(fp, zv);
		t2 = _mm_unpacklo_epi8(t2, zv);

		apl = _mm_shufflelo_epi16(t2, 0xFF);
		aph = _mm_shufflehi_epi16(t2, 0xFF);
		ap = _mm_castpd_si128(_mm_move_sd(_mm_castsi128_pd(aph), _mm_castsi128_pd(apl)));
		ap = _mm_sub_epi16(sc, ap);

		t2 = _mm_mullo_epi16(t2, m_color_4);
		fp = _mm_mullo_epi16(fp, ap);
		t2 = _mm_adds_epu16(t2, fp);
		t2 = _mm_srli_epi16(t2, 8);
		tp = _mm_packus_epi16(t1, t2);
		_mm_storeu_si128((__m128i*) p, tp);
		p += 4;

		w_4 = _mm_add_ps(w_4, aw_4);
		u_4 = _mm_add_ps(u_4, au_4);
		v_4 = _mm_add_ps(v_4, av_4);
	}

	if (r == 0) return;

	__m128 z_4 = _mm_rcp_ps(w_4);

	__m128 mu_4, mv_4;
	mu_4 = _mm_mul_ps(u_4, z_4);
	mv_4 = _mm_mul_ps(v_4, z_4);
	mv_4 = _mm_mul_ps(mv_4, m_texScale_4);

	__m128i mui_4, mvi_4;
	mui_4 = _mm_cvtps_epi32(mu_4);
	mvi_4 = _mm_cvtps_epi32(mv_4);
	mui_4 = _mm_and_si128(mui_4, m_texMaskU_4);
	mvi_4 = _mm_and_si128(mvi_4, m_texMaskV_4);
	mui_4 = _mm_add_epi32(mui_4, mvi_4);

	__m128i zv = _mm_set1_epi32(0);
	__m128i tp, fp;
	fp = _mm_loadl_epi64((__m128i*) p);
	tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[0]]);
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

	if (r == 1) return;
	fp = _mm_loadl_epi64((__m128i*) p);
	tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[1]]);
	fp = _mm_unpacklo_epi8(fp, zv);
	tp = _mm_unpacklo_epi8(tp, zv);

	ap = _mm_shufflelo_epi16(tp, 0xFF);
	ap = _mm_sub_epi16(sc, ap);

	tp = _mm_mullo_epi16(tp, m_color_4);
	fp = _mm_mullo_epi16(fp, ap);
	tp = _mm_adds_epu16(tp, fp);
	tp = _mm_srli_epi16(tp, 8);

	tp = _mm_packus_epi16(tp, zv);
	*p++ = _mm_cvtsi128_si32(tp);

	if (r == 2) return;
	fp = _mm_loadl_epi64((__m128i*) p);
	tp = _mm_loadl_epi64((__m128i*) & m_texDiffusePixels[((uint32_t*)&mui_4)[2]]);
	fp = _mm_unpacklo_epi8(fp, zv);
	tp = _mm_unpacklo_epi8(tp, zv);

	ap = _mm_shufflelo_epi16(tp, 0xFF);
	ap = _mm_sub_epi16(sc, ap);

	tp = _mm_mullo_epi16(tp, m_color_4);
	fp = _mm_mullo_epi16(fp, ap);
	tp = _mm_adds_epu16(tp, fp);
	tp = _mm_srli_epi16(tp, 8);

	tp = _mm_packus_epi16(tp, zv);
	*p++ = _mm_cvtsi128_si32(tp);
#else
	uint8_t* sc = (uint8_t*)&curTriangle->solidColor;

	float d = x2 - x1;
	if (d == 0.0f) return;

	float au = (u2 - u1) / d;
	float av = (v2 - v1) / d;
	float aw = (w2 - w1) / d;

	int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	uint8_t* p = (uint8_t*)(xb + ((int)y) * frame.tx + pixels);

	for (int x = xb; x < xe; x++) {

		float z = 1.0f / w1;
		uint32_t tu = ((int32_t)(u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t)(v1 * z)) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		int a = 256 - t[3];
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
// Filler (sse/float) - flat textured & alpha blended z-corrected scans with fog
inline void Rasterizer::fillFlatTexAlphaZCFog(int y, float x1, float x2, float w1, float w2, float u1, float u2, float v1, float v2)
{
#if USE_SIMD && USE_SSE2
	// TODO: SSE implementation
	uint8_t* sc = (uint8_t*)&m_curTriangle->solidColor;
	uint8_t* fc = (uint8_t*)&m_curTrilist->fog.color;

	const float d = x2 - x1;
	if (d == 0.0f) return;

	const float au = (u2 - u1) / d;
	const float av = (v2 - v1) / d;
	const float aw = (w2 - w1) / d;

	const float znear = m_curTrilist->fog.near;
	const float zfar = m_curTrilist->fog.far;
	const float zscale = -1.0f / (znear - zfar);

	const int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	uint8_t* p = (uint8_t*)(xb + ((int)y) * frame.tx + m_pixels);

	for (int x = xb; x < xe; x++)
	{
		const float z = 1.0f / w1;
		uint32_t tu = ((int32_t)(u1 * z)) & m_texMaskU;
		uint32_t tv = ((int32_t)(v1 * z)) & m_texMaskV;
		uint8_t* t = (uint8_t*)&m_texDiffusePixels[tu + (tv << m_texSizeU)];

		float ff = (z - znear) * zscale;
		ff = cmmax(0.0f, ff);
		ff = cmmin(1.0f, ff);
		ff = 256.0f * ff * ff;
		const int fb = (int)ff;

		const int n = t[3];
		const int a = 256 - n;

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

	float d = x2 - x1;
	if (d == 0.0f) return;

	float au = (u2 - u1) / d;
	float av = (v2 - v1) / d;
	float aw = (w2 - w1) / d;

	float znear = curTrilist->fog.near;
	float zfar = curTrilist->fog.far;
	float zscale = -1.0f / (znear - zfar);

	int xb = (int)(x1);
	int xe = (int)(x2 + 0.9999f);
	if (xe > frame.tx) xe = frame.tx;

	uint8_t* p = (uint8_t*)(xb + ((int)y) * frame.tx + pixels);

	for (int x = xb; x < xe; x++) {
		float z = 1.0f / w1;
		uint32_t tu = ((int32_t)(u1 * z)) & texMaskU;
		uint32_t tv = ((int32_t)(v1 * z)) & texMaskV;
		uint8_t* t = (uint8_t*)&texDiffusePixels[tu + (tv << texSizeU)];

		float ff = (z - znear) * zscale;
		ff = cmmax(0.0f, ff);
		ff = cmmin(1.0f, ff);
		ff = 256.0f * ff * ff;
		int fb = (int)ff;

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
#endif // !RENDERER_INTRASTER
#include "Bitmap.h"
// Intrinsics includes
#if USE_SSE2
#	include <xmmintrin.h>
#	include <emmintrin.h>
#endif // USE_SSE2
#include <string.h>
//-----------------------------------------------------------------------------
Bitmap::Bitmap()
{
	for( int l = 0; l < BMP_MIPMAPS; l++ )
		mipmaps[l] = nullptr;
}
//-----------------------------------------------------------------------------
Bitmap::~Bitmap()
{
	Deallocate();
}
//-----------------------------------------------------------------------------
void Bitmap::Clear(Color color)
{
#if USE_SIMD && USE_SSE2
	const int size = tx * ty;
	const int b = size >> 2;
	const int r = size & 0x3;

	__m128i color_4 = _mm_set_epi32(color, color, color, color);
	__m128i* p_4 = (__m128i *) data;
	for( int t = 0; t < b; t++ )
		*p_4++ = color_4;

	Color* p = (Color*)p_4;
	if( r == 0 ) return;
	*p++ = color;
	if( r == 1 ) return;
	*p++ = color;
	if( r == 2 ) return;
	*p++ = color;
#else
	const size_t size = tx * ty;
	Color* p = (Color*)data;
	for (size_t t = 0; t < size; t++)
		p[t] = color;
#endif	// USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
void Bitmap::Rect(int32_t x, int32_t y, int32_t w, int32_t h, Color color)
{
	if( x >= tx ) return;
	if( y >= ty ) return;
	int xe = x + w;
	if( xe <= 0 ) return;
	int ye = y + h;
	if( ye <= 0 ) return;

	if( x < 0 ) x = 0;
	if( y < 0 ) y = 0;
	if( xe > tx ) xe = tx;
	if( ye > ty ) ye = ty;

	Color* d = (Color*)data;
	d += x + y * tx;
	w = xe - x;
	h = ye - y;

	const int step = tx - w;
	for( int j = 0; j < h; j++ )
	{
		for( int i = 0; i < w; i++ )
			*d++ = color;
		d += step;
	}
}
//-----------------------------------------------------------------------------
void Bitmap::Blit(int32_t xDst, int32_t yDst, const Bitmap* src, int32_t xSrc, int32_t ySrc, int32_t w, int32_t h)
{
	if( xDst >= tx ) return;
	if( yDst >= ty ) return;
	int xeDst = xDst + w;
	if( xeDst <= 0 ) return;
	int yeDst = yDst + h;
	if( yeDst <= 0 ) return;

	if( xDst < 0 )
	{
		xDst = 0; xSrc -= xDst;
	}
	if( yDst < 0 )
	{
		yDst = 0; ySrc -= yDst;
	}
	if( xeDst > tx ) xeDst = tx;
	if( yeDst > ty ) yeDst = ty;

	Color* d = (Color*)data;
	Color* s = (Color*)src->data;
	d += xDst + yDst * tx;
	s += xSrc + ySrc * src->tx;
	w = xeDst - xDst;
	h = yeDst - yDst;

	const int stepDst = tx - w;
	const int stepSrc = src->tx - w;

	for( int y = 0; y < h; y++ )
	{
		for( int x = 0; x < w; x++ )
		{
			*d++ = *s++;
		}
		s += stepSrc;
		d += stepDst;
	}
}
//-----------------------------------------------------------------------------
void Bitmap::AlphaBlit(int32_t xDst, int32_t yDst, const Bitmap* src, int32_t xSrc, int32_t ySrc, int32_t w, int32_t h)
{
#if USE_SIMD && USE_SSE2
	if( xDst >= tx ) return;
	if( yDst >= ty ) return;
	int xeDst = xDst + w;
	if( xeDst <= 0 ) return;
	int yeDst = yDst + h;
	if( yeDst <= 0 ) return;

	if( xDst < 0 )
	{
		xDst = 0; xSrc -= xDst;
	}
	if( yDst < 0 )
	{
		yDst = 0; ySrc -= yDst;
	}
	if( xeDst > tx ) xeDst = tx;
	if( yeDst > ty ) yeDst = ty;

	Color* d = (Color*)data;
	Color* s = (Color*)src->data;
	d += xDst + yDst * tx;
	s += xSrc + ySrc * src->tx;
	w = xeDst - xDst;
	h = yeDst - yDst;

	const int stepDst = tx - w;
	const int stepSrc = src->tx - w;

	__m128i zv = _mm_set_epi32(0, 0, 0, 0);
	__m128i sc = _mm_set_epi32(0x01000100, 0x01000100, 0x01000100, 0x01000100);

	for( int y = 0; y < h; y++ )
	{
		for( int x = 0; x < w; x++ )
		{
			__m128i dp, sp;
			dp = _mm_loadl_epi64((__m128i *) d);
			sp = _mm_loadl_epi64((__m128i *) s++);
			dp = _mm_unpacklo_epi8(zv, dp);
			sp = _mm_unpacklo_epi8(sp, zv);

			__m128i ap;
			ap = _mm_shufflelo_epi16(sp, 0xFF);
			ap = _mm_sub_epi16(sc, ap);
			dp = _mm_mulhi_epu16(dp, ap);
			dp = _mm_adds_epu16(sp, dp);
			dp = _mm_packus_epi16(dp, zv);

			*d++ = _mm_cvtsi128_si32(dp);
		}

		s += stepSrc;
		d += stepDst;
	}
#else
	if (xDst >= tx) return;
	if (yDst >= ty) return;
	int xeDst = xDst + w;
	if (xeDst <= 0) return;
	int yeDst = yDst + h;
	if (yeDst <= 0) return;

	if (xDst < 0)
	{
		xDst = 0; xSrc -= xDst;
	}
	if (yDst < 0)
	{
		yDst = 0; ySrc -= yDst;
	}
	if (xeDst > tx) xeDst = tx;
	if (yeDst > ty) yeDst = ty;

	Color* d = (Color*)data;
	Color* s = (Color*)src->data;
	d += xDst + yDst * tx;
	s += xSrc + ySrc * src->tx;
	w = xeDst - xDst;
	h = yeDst - yDst;

	int stepDst = tx - w;
	int stepSrc = src->tx - w;

	for (int y = 0; y < h; y++)
	{
		for (int x = 0; x < w; x++)
		{
			Color* dPix = (Color*)d++;
			Color* sPix = (Color*)s++;
			uint16_t a = 256 - sPix->a;
			dPix->r = ((dPix->r * a) >> 8) + sPix->r;
			dPix->g = ((dPix->g * a) >> 8) + sPix->g;
			dPix->b = ((dPix->b * a) >> 8) + sPix->b;
			dPix->a = ((dPix->a * a) >> 8) + sPix->a;
		}

		s += stepSrc;
		d += stepDst;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
void Bitmap::AlphaScaleBlit(int32_t xDst, int32_t yDst, int32_t wDst, int32_t hDst, const Bitmap* src, int32_t xSrc, int32_t ySrc, int32_t wSrc, int32_t hSrc)
{
#if USE_SIMD && USE_SSE2
	if( wDst <= 0 ) return;
	if( hDst <= 0 ) return;

	int32_t us = (wSrc << 16) / wDst;
	int32_t vs = (hSrc << 16) / hDst;

	if( xDst >= tx ) return;
	if( yDst >= ty ) return;
	int xeDst = xDst + wDst;
	if( xeDst <= 0 ) return;
	int yeDst = yDst + hDst;
	if( yeDst <= 0 ) return;

	int32_t ub = 0;
	int32_t vb = 0;
	if( xDst < 0 )
	{
		ub = ((int64_t)(-xDst * wSrc) << 16) / wDst;
		xDst = 0;
	}
	if( yDst < 0 )
	{
		vb = ((int64_t)(-yDst * hSrc) << 16) / hDst;
		yDst = 0;
	}
	if( xeDst > tx ) xeDst = tx;
	if( yeDst > ty ) yeDst = ty;

	Color* d = (Color*)data;
	Color* s = (Color*)src->data;
	d += xDst + yDst * tx;

	wDst = xeDst - xDst;
	hDst = yeDst - yDst;
	int stepDst = tx - wDst;

	__m128i zv = _mm_set_epi32(0, 0, 0, 0);
	__m128i sc = _mm_set_epi32(0x01000100, 0x01000100, 0x01000100, 0x01000100);

	int32_t u = ub;
	int32_t v = vb;
	for( int y = 0; y < hDst; y++ )
	{
		for( int x = 0; x < wDst; x++ )
		{
			Color* p = &s[(u >> 16) + (v >> 16) * src->tx];
			u += us;

			__m128i dp, sp;
			dp = _mm_loadl_epi64((__m128i *) d);
			sp = _mm_loadl_epi64((__m128i *) p);
			dp = _mm_unpacklo_epi8(zv, dp);
			sp = _mm_unpacklo_epi8(sp, zv);

			__m128i ap;
			ap = _mm_shufflelo_epi16(sp, 0xFF);
			ap = _mm_sub_epi16(sc, ap);
			dp = _mm_mulhi_epu16(dp, ap);
			dp = _mm_adds_epu16(sp, dp);
			dp = _mm_packus_epi16(dp, zv);
			*d++ = _mm_cvtsi128_si32(dp);
		}

		u = ub;
		v += vs;
		d += stepDst;
	}
#else
	if (wDst <= 0) return;
	if (hDst <= 0) return;

	int32_t us = (wSrc << 16) / wDst;
	int32_t vs = (hSrc << 16) / hDst;

	if (xDst >= tx) return;
	if (yDst >= ty) return;
	int xeDst = xDst + wDst;
	if (xeDst <= 0) return;
	int yeDst = yDst + hDst;
	if (yeDst <= 0) return;

	int32_t ub = 0;
	int32_t vb = 0;
	if (xDst < 0)
	{
		ub = ((int64_t)(-xDst * wSrc) << 16) / wDst;
		xDst = 0;
	}
	if (yDst < 0)
	{
		vb = ((int64_t)(-yDst * hSrc) << 16) / hDst;
		yDst = 0;
	}
	if (xeDst > tx) xeDst = tx;
	if (yeDst > ty) yeDst = ty;

	LeColor* d = (LeColor*)data;
	LeColor* s = (LeColor*)src->data;
	d += xDst + yDst * tx;

	wDst = xeDst - xDst;
	hDst = yeDst - yDst;
	int stepDst = tx - wDst;

	int32_t u = ub;
	int32_t v = vb;
	for (int y = 0; y < hDst; y++)
	{
		for (int x = 0; x < wDst; x++)
		{
			uint8_t* sPix = (uint8_t*)&s[(u >> 16) + (v >> 16) * src->tx];
			uint8_t* dPix = (uint8_t*)d++;
			u += us;

			uint16_t a = 256 - sPix[3];
			dPix[0] = ((dPix[0] * a) >> 8) + sPix[0];
			dPix[1] = ((dPix[1] * a) >> 8) + sPix[1];
			dPix[2] = ((dPix[2] * a) >> 8) + sPix[2];
			dPix[3] = ((dPix[3] * a) >> 8) + sPix[3];
		}

		u = ub;
		v += vs;
		d += stepDst;
	}
#endif // USE_SIMD && USE_SSE2
}
//-----------------------------------------------------------------------------
void Bitmap::Text(int x, int y, const char* text, int length, const BmpFont* font)
{
	const int cx = font->charSizeX;
	const int cy = font->charSizeY;
	int lx = 0, ly = 0;

	for( int i = 0; i < length; i++ )
	{
		int c = text[i];
		if( c == '\n' )
		{
			lx = 0; ly++;
		}
		else
		{
			if( c >= font->charBegin && c < font->charEnd )
			{
				const int px = x + lx * font->spaceX;
				const int py = y + ly * font->spaceY;
				AlphaBlit(px, py, font->font, 0, (c - font->charBegin) * cy, cx, cy);
			}
			lx++;
		}
	}
}
//-----------------------------------------------------------------------------
void Bitmap::Allocate(int tx, int ty)
{
	int size = tx * ty;
#if USE_SIMD && USE_SSE2
	size += 4;
#endif
	data = new Color[size];
	memset(data, 0, sizeof(Color) * size);
	dataAllocated = true;

	this->tx = tx;
	this->ty = ty;
	txP2 = Global::log2i32(tx);
	tyP2 = Global::log2i32(ty);
	flags = BITMAP_RGB;
}
//-----------------------------------------------------------------------------
void Bitmap::Deallocate()
{
	if( dataAllocated && data )
		delete[](Color*) data;
	dataAllocated = false;

	for( int l = 1; l < mmLevels; l++ )
		delete mipmaps[l];

	tx = ty = 0;
	txP2 = tyP2 = 0;
	flags = 0;
}
//-----------------------------------------------------------------------------
void Bitmap::PreMultiply()
{
	const size_t noPixels = tx * ty;

	Color* c = (Color*)data;
	for( size_t i = 0; i < noPixels; i++ )
	{
		c->r = (c->r * c->a) >> 8;
		c->g = (c->g * c->a) >> 8;
		c->b = (c->b * c->a) >> 8;
		c++;
	}

	flags |= BITMAP_RGBA | BITMAP_PREMULTIPLIED;
	for( int l = 1; l < mmLevels; l++ )
		mipmaps[l]->PreMultiply();
}
//-----------------------------------------------------------------------------
void Bitmap::MakeMipmaps()
{
	if( (tx & (tx - 1)) != 0 || (ty & (ty - 1)) != 0 )
		return;

	mmLevels = 0;
	mipmaps[mmLevels++] = this;

	int mtx = tx / 2;
	int mty = ty / 2;

	Color* o = (Color*)data;

	for( int l = 0; l < BMP_MIPMAPS; l++ )
	{
		if( mtx < 4 || mty < 4 ) break;

		Bitmap* bmp = new Bitmap();
		bmp->Allocate(mtx, mty);

		Color* p = (Color*)bmp->data;

		for( int y = 0; y < mty; y++ )
		{
			for( int x = 0; x < mtx; x++ )
			{
				Color* s1 = o;
				Color* s2 = o + 1;
				Color* s3 = o + mtx * 2;
				Color* s4 = o + mtx * 2 + 1;
				int r = (s1->r + s2->r + s3->r + s4->r) >> 2;
				int g = (s1->g + s2->g + s3->g + s4->g) >> 2;
				int b = (s1->b + s2->b + s3->b + s4->b) >> 2;
				int a = (s1->a + s2->a + s3->a + s4->a) >> 2;
				*p++ = Color(r, g, b, a);
				o += 2;
			}
			o += mtx * 2;
		}

		mtx /= 2;
		mty /= 2;
		o = (Color*)bmp->data;

		mipmaps[mmLevels++] = bmp;
	}
}
//-----------------------------------------------------------------------------
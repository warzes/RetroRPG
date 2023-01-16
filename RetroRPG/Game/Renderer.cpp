#include "Renderer.h"
#include "BmpCache.h"
#include <string.h>
//-----------------------------------------------------------------------------
Renderer::Renderer(int width, int height) 
	: m_usedVerlist(&m_intVerlist)
	, m_usedTrilist(&m_intTrilist)
{
	// Configure viewport
	SetViewport(0, 0, width, height);
	SetViewClipping(RENDERER_NEAR_DEFAULT, RENDERER_FAR_DEFAULT);

	// Configure default camera
	SetViewPosition({ 0.0f, 0.0f, 0.0f });
	SetViewAngle({ 0.0f, 0.0f, 0.0f });

	// Flush the lists
	Flush();
}
//-----------------------------------------------------------------------------
void Renderer::Render(const Mesh* mesh)
{
	// Check vertex memory space
	if( !checkMemory(mesh->noVertexes, mesh->noTriangles) )
		return;

	// Transform the geometry
	Triangle* triRender = &m_usedTrilist->triangles[m_usedTrilist->noUsed];
	int* id1 = &m_usedTrilist->srcIndices[m_usedTrilist->noValid];
	int* id2 = &m_usedTrilist->dstIndices[m_usedTrilist->noValid];

	transform(mesh->view, mesh->vertexes, m_usedVerlist->vertexes, mesh->noVertexes);
	int noTris = build(mesh, m_usedVerlist->vertexes, triRender, id1);
	m_extra = noTris;

	// Clip and project
	noTris = clip3D(triRender, id1, id2, noTris, m_viewFrontPlan);
	noTris = clip3D(triRender, id2, id1, noTris, m_viewBackPlan);

#if RENDERER_3DFRUSTRUM
	noTris = clip3D(triRender, id1, id2, noTris, m_viewLeftPlan);
	noTris = clip3D(triRender, id2, id1, noTris, m_viewRightPlan);
	noTris = clip3D(triRender, id1, id2, noTris, m_viewTopPlan);
	noTris = clip3D(triRender, id2, id1, noTris, m_viewBotPlan);
#endif // RENDERER_3DFRUSTRUM

	noTris = project(triRender, id1, id2, noTris);
	noTris = backculling(triRender, id2, id1, noTris);

#if RENDERER_2DFRAME
	noTris = clip2D(triRender, id1, id2, noTris, viewLeftAxis);
	noTris = clip2D(triRender, id2, id1, noTris, viewRightAxis);
	noTris = clip2D(triRender, id1, id2, noTris, viewTopAxis);
	noTris = clip2D(triRender, id2, id1, noTris, viewBottomAxis);
#endif // RENDERER_2DFRAME

	// Make render indices absolute
	for( int i = 0; i < noTris; i++ )
		id1[i] += m_usedTrilist->noUsed;

	// Modify the state
	m_usedTrilist->noUsed += m_extra;
	m_usedTrilist->noValid += noTris;
}
//-----------------------------------------------------------------------------
void Renderer::Render(const BillboardSet* bset)
{
	// Check vertex memory space
	size_t noVertexes = bset->noBillboards * 4u;
	size_t noTriangles = bset->noBillboards * 2u;
	if( !checkMemory(noVertexes, noTriangles) )
		return;

	// Transform the geometry
	Triangle* triRender = &m_usedTrilist->triangles[m_usedTrilist->noUsed];
	int* id1 = &m_usedTrilist->srcIndices[m_usedTrilist->noValid];
	int* id2 = &m_usedTrilist->dstIndices[m_usedTrilist->noValid];

	transform(bset->view, bset->places, m_usedVerlist->vertexes, bset->noBillboards);
	int noTris = build(bset, m_usedVerlist->vertexes, triRender, id1);
	m_extra = noTris;

	// Clip and project
	noTris = clip3D(triRender, id1, id2, noTris, m_viewFrontPlan);
	noTris = clip3D(triRender, id2, id1, noTris, m_viewBackPlan);

#if RENDERER_3DFRUSTRUM
	noTris = clip3D(triRender, id1, id2, noTris, m_viewLeftPlan);
	noTris = clip3D(triRender, id2, id1, noTris, m_viewRightPlan);
	noTris = clip3D(triRender, id1, id2, noTris, m_viewTopPlan);
	noTris = clip3D(triRender, id2, id1, noTris, m_viewBotPlan);
#endif // RENDERER_3DFRUSTRUM

	noTris = project(triRender, id1, id2, noTris);
	noTris = backculling(triRender, id2, id1, noTris);

#if RENDERER_2DFRAME
	noTris = clip2D(triRender, id1, id2, noTris, viewLeftAxis);
	noTris = clip2D(triRender, id2, id1, noTris, viewRightAxis);
	noTris = clip2D(triRender, id1, id2, noTris, viewTopAxis);
	noTris = clip2D(triRender, id2, id1, noTris, viewBottomAxis);
#endif // RENDERER_2DFRAME

	// Make render indices absolute
	for( int i = 0; i < noTris; i++ )
		id1[i] += m_usedTrilist->noUsed;

	// Modify the state
	m_usedTrilist->noUsed += m_extra;
	m_usedTrilist->noValid += noTris;
}
//-----------------------------------------------------------------------------
void Renderer::Flush()
{
	m_usedTrilist->noUsed = 0;
	m_usedTrilist->noValid = 0;
}
//-----------------------------------------------------------------------------
int Renderer::GetViewportCoordinates(const Vertex& pos, Vertex& outViewCoords) const
{
	const float centerX = (m_viewRightAxis.origin.x - m_viewLeftAxis.origin.x) * 0.5f;
	const float centerY = (m_viewBottomAxis.origin.y - m_viewTopAxis.origin.y) * 0.5f;

	const Vertex p = m_viewMatrix * pos;
	if( p.z > m_viewFrontPlan.zAxis.origin.z ) return 0;
	if( p.z <= m_viewBackPlan.zAxis.origin.z ) return 0;

	const float w = 1.0f / p.z;
	outViewCoords.x = p.x * m_ztx * w + centerX;
	outViewCoords.y = centerY - p.y * m_zty * w;
	outViewCoords.z = p.z;

	if(outViewCoords.x < m_viewLeftAxis.origin.x || outViewCoords.x >= m_viewRightAxis.origin.x) return 0;
	if(outViewCoords.y < m_viewTopAxis.origin.y || outViewCoords.y >= m_viewBottomAxis.origin.y) return 0;

	return 1;
}
//-----------------------------------------------------------------------------
void Renderer::SetViewPosition(const Vertex& pos)
{
	m_viewPosition = pos;
}
//-----------------------------------------------------------------------------
void Renderer::SetViewAngle(const Vertex& angleYZX)
{
	m_viewAngle = angleYZX;
}
//-----------------------------------------------------------------------------
void Renderer::UpdateViewMatrix()
{
	m_viewMatrix.Identity();
	m_viewMatrix.Translate(-m_viewPosition);
	m_viewMatrix.RotateEulerYZX(-m_viewAngle * d2r);
}
//-----------------------------------------------------------------------------
void Renderer::SetViewMatrix(const Matrix& view)
{
	m_viewMatrix = view;
}
//-----------------------------------------------------------------------------
void Renderer::SetViewport(int left, int top, int right, int bottom)
{
	const float leftFloat = (float)left;
	const float topFloat = (float)top;
	const float rightFloat = (float)right;
	const float bottomFloat = (float)bottom;

	m_viewLeftAxis = Axis(Vertex(leftFloat, topFloat, 0.0f), Vertex(leftFloat, bottomFloat, 0.0f));
	m_viewRightAxis = Axis(Vertex(rightFloat, bottomFloat, 0.0f), Vertex(rightFloat, topFloat, 0.0f));
	m_viewTopAxis = Axis(Vertex(rightFloat, topFloat, 0.0f), Vertex(leftFloat, topFloat, 0.0f));
	m_viewBottomAxis = Axis(Vertex(leftFloat, bottomFloat, 0.0f), Vertex(rightFloat, bottomFloat, 0.0f));
	SetViewProjection(m_viewFov);
}
//-----------------------------------------------------------------------------
void Renderer::SetViewClipping(float near, float far)
{
	m_viewFrontPlan.zAxis.origin.z = -near;
	m_viewBackPlan.zAxis.origin.z = -far;
	m_viewFrontPlan.zAxis.axis.z = -1.0f;
	m_viewBackPlan.zAxis.axis.z = 1.0f;
	SetViewProjection(m_viewFov);
}
//-----------------------------------------------------------------------------
void Renderer::SetViewProjection(float fov)
{
	const float width = m_viewRightAxis.origin.x - m_viewLeftAxis.origin.x;
	const float near = m_viewFrontPlan.zAxis.origin.z;
	const float ratio = tanf(fov * d2r) * near;
	m_ztx = width / ratio;
	m_zty = width / ratio;
	updateFrustrum();
}
//-----------------------------------------------------------------------------
void Renderer::SetViewOffset(float offset)
{
	m_vOffset = offset * offset * cmsgn(offset);
}
//-----------------------------------------------------------------------------
void Renderer::SetBackcullingMode(BACKCULLING_MODES mode)
{
	m_backMode = mode;
}
//-----------------------------------------------------------------------------
void Renderer::SetFog(bool enable)
{
	m_fogEnable = enable;
}
//-----------------------------------------------------------------------------
void Renderer::SetFogProperties(Color color, float near, float far)
{
	if (!m_usedTrilist) return;
	m_usedTrilist->fog.color = color;
	m_usedTrilist->fog.near = -near;
	m_usedTrilist->fog.far = -far;
}
//-----------------------------------------------------------------------------
void Renderer::SetMipmapping(bool enable)
{
	m_mipmappingEnable = enable;
}
//-----------------------------------------------------------------------------
void Renderer::SetTriangleList(TriangleList* trilist)
{
	if (!trilist) m_usedTrilist = &m_intTrilist;
	else m_usedTrilist = trilist;
}
//-----------------------------------------------------------------------------
TriangleList* Renderer::GetTriangleList()
{
	return m_usedTrilist;
}
//-----------------------------------------------------------------------------
bool Renderer::checkMemory(size_t noVertexes, size_t noTriangles)
{
	if( noVertexes > m_usedVerlist->noAllocated ) return false;
	m_usedVerlist->noUsed = 0;

	const size_t freeTriangles = m_usedTrilist->noAllocated - m_usedTrilist->noUsed;
	if( noTriangles > freeTriangles ) return false;
	m_extraMax = freeTriangles;

	return true;
}
//-----------------------------------------------------------------------------
int Renderer::build(const Mesh* mesh, Vertex vertexes[], Triangle tris[], int indices[])
{
	const float near = m_viewFrontPlan.zAxis.origin.z;
	const float far = m_viewBackPlan.zAxis.origin.z;

	if (mesh->shades) m_colors = mesh->shades;
	else m_colors = mesh->colors;

	int flags = TRIANGLE_TEXTURED;
	if (m_mipmappingEnable) flags |= TRIANGLE_MIPMAPPED;
	if (m_fogEnable) flags |= TRIANGLE_FOGGED;

	int k = 0;
	for (int i = 0; i < mesh->noTriangles; i++)
	{
		Vertex* v1 = &vertexes[mesh->vertexesList[i * 3]];
		Vertex* v2 = &vertexes[mesh->vertexesList[i * 3 + 1]];
		Vertex* v3 = &vertexes[mesh->vertexesList[i * 3 + 2]];

		// Hard clip
		if (v1->z > near && v2->z > near && v3->z > near) continue;
		if (v1->z <= far && v2->z <= far && v3->z <= far) continue;

		// Fetch triangle properties
		const int texSlot = mesh->texSlotList[i];
		int subFlags = flags;
		if (bmpCache.cacheSlots[texSlot].flags & BITMAP_RGBA)
			subFlags |= TRIANGLE_BLENDED;

		// Copy coordinates (for clipping)
		Triangle* tri = &tris[k];
		tri->xs[0] = v1->x;
		tri->ys[0] = v1->y;
		tri->zs[0] = v1->z;
		tri->xs[1] = v2->x;
		tri->ys[1] = v2->y;
		tri->zs[1] = v2->z;
		tri->xs[2] = v3->x;
		tri->ys[2] = v3->y;
		tri->zs[2] = v3->z;
		const int m1 = 2 * mesh->texCoordsList[i * 3];
		tri->us[0] = mesh->texCoords[m1];
		tri->vs[0] = mesh->texCoords[m1 + 1];
		const int m2 = 2 * mesh->texCoordsList[i * 3 + 1];
		tri->us[1] = mesh->texCoords[m2];
		tri->vs[1] = mesh->texCoords[m2 + 1];
		const int m3 = 2 * mesh->texCoordsList[i * 3 + 2];
		tri->us[2] = mesh->texCoords[m3];
		tri->vs[2] = mesh->texCoords[m3 + 1];

		// Compute view distance
		const float d1 = tri->zs[0] + tri->zs[1] + tri->zs[2];
		const float d2 = tri->ys[0] + tri->ys[1] + tri->ys[2];
		const float d3 = tri->xs[0] + tri->xs[1] + tri->xs[2];
		tri->vd = d1 * d1 + d2 * d2 + d3 * d3 - m_vOffset;

		// Set material properties
		tri->solidColor = m_colors[i];
		tri->diffuseTexture = texSlot;
		tri->flags = subFlags;

		indices[k] = k;
		k++;
	}
	return k;
}
//-----------------------------------------------------------------------------
int Renderer::build(const BillboardSet* bset, Vertex vertexes[], Triangle tris[], int indices[])
{
	const float near = m_viewFrontPlan.zAxis.origin.z;
	const float far = m_viewBackPlan.zAxis.origin.z;

	if (bset->shades) m_colors = bset->shades;
	else m_colors = bset->colors;

	int flags = TRIANGLE_TEXTURED;
	if (m_mipmappingEnable) flags |= TRIANGLE_MIPMAPPED;
	if (m_fogEnable) flags |= TRIANGLE_FOGGED;

	int k = 0;
	for (int i = 0; i < bset->noBillboards; i++)
	{
		if (!bset->flags[i]) continue;
		Vertex* v = &vertexes[i];

		// Hard clip
		if (v->z > near && v->z <= far) continue;

		// Construct billboard
		const float sx = bset->sizes[i * 2 + 0] * 0.5f;
		const float sy = bset->sizes[i * 2 + 1] * 0.5f;

		// Fetch billboard properties
		const int texSlot = bset->texSlots[i];
		int subFlags = flags;
		if (bmpCache.cacheSlots[texSlot].flags & BITMAP_RGBA)
			subFlags |= TRIANGLE_BLENDED;

		// First triangle
		Triangle* tri = &tris[k];
		tri->xs[0] = v->x + sx;
		tri->ys[0] = v->y + sy;
		tri->zs[0] = v->z;
		tri->xs[1] = v->x - sx;
		tri->ys[1] = v->y + sy;
		tri->zs[1] = v->z;
		tri->xs[2] = v->x - sx;
		tri->ys[2] = v->y - sy;
		tri->zs[2] = v->z;

		tri->us[0] = 1.0f;
		tri->vs[0] = 0.0f;
		tri->us[1] = 0.0f;
		tri->vs[1] = 0.0f;
		tri->us[2] = 0.0f;
		tri->vs[2] = 1.0f;

		// Compute view distance
		tri->vd = v->x * v->x + v->y * v->y + v->z * v->z - m_vOffset;

		// Set material properties
		tri->solidColor = m_colors[i];
		tri->diffuseTexture = texSlot;
		tri->flags = subFlags;
		indices[k] = k;
		k++;

		// Second triangle
		tri = &tris[k];
		tri->xs[0] = v->x + sx;
		tri->ys[0] = v->y + sy;
		tri->zs[0] = v->z;
		tri->xs[1] = v->x - sx;
		tri->ys[1] = v->y - sy;
		tri->zs[1] = v->z;
		tri->xs[2] = v->x + sx;
		tri->ys[2] = v->y - sy;
		tri->zs[2] = v->z;

		tri->us[0] = 1.0f;
		tri->vs[0] = 0.0f;
		tri->us[1] = 0.0f;
		tri->vs[1] = 1.0f;
		tri->us[2] = 1.0f;
		tri->vs[2] = 1.0f;

		// Compute view distance
		tri->vd = v->x * v->x + v->y * v->y + v->z * v->z - m_vOffset;

		// Set material properties
		tri->solidColor = m_colors[i];
		tri->diffuseTexture = texSlot;
		tri->flags = subFlags;
		indices[k] = k;
		k++;
	}
	return k;
}
//-----------------------------------------------------------------------------
void Renderer::updateFrustrum()
{
	const float near = m_viewFrontPlan.zAxis.origin.z;
	const float far = m_viewBackPlan.zAxis.origin.z;
	const float dr = far / near;

	const float height = m_viewBottomAxis.origin.y - m_viewTopAxis.origin.y;
	const float hf = height * 0.5f / m_zty;
	const float hb = hf * dr;
	m_viewTopPlan = Plane(Vertex(0.0f, -hb, far), Vertex(1.0f, -hb, far), Vertex(0.0f, -hf, near));
	m_viewBotPlan = Plane(Vertex(0.0f, hb, far), Vertex(-1.0f, hb, far), Vertex(0.0f, hf, near));

	const float width = m_viewRightAxis.origin.x - m_viewLeftAxis.origin.x;
	const float wf = width * 0.5f / m_ztx;
	const float wb = wf * dr;
	m_viewLeftPlan = Plane(Vertex(wf, 0.0f, near), Vertex(wb, 0.0f, far), Vertex(wf, 1.0f, near));
	m_viewRightPlan = Plane(Vertex(-wf, 0.0f, near), Vertex(-wb, 0.0f, far), Vertex(-wf, -1.0f, near));
}
//-----------------------------------------------------------------------------
void Renderer::transform(const Matrix& matrix, const Vertex srcVertexes[], Vertex dstVertexes[], int nb) const
{
	Matrix view = m_viewMatrix * matrix;
	for (int i = 0; i < nb; i++)
		dstVertexes[i] = view * srcVertexes[i];
}
//-----------------------------------------------------------------------------
int Renderer::project(Triangle tris[], const int srcIndices[], int dstIndices[], int nb)
{
	const float width = m_viewRightAxis.origin.x - m_viewLeftAxis.origin.x;
	const float height = m_viewBottomAxis.origin.y - m_viewTopAxis.origin.y;
	const float centerX = m_viewLeftAxis.origin.x + width * 0.5f;
	const float centerY = m_viewTopAxis.origin.y + height * 0.5f;
	const float near = -m_viewFrontPlan.zAxis.origin.z;

	int k = 0;
	for (int i = 0; i < nb; i++)
	{
		const int j = srcIndices[i];

		// Project coordinates on viewport
		Triangle* tri = &tris[j];

		const float w0 = near / tri->zs[0];
		const float w1 = near / tri->zs[1];
		const float w2 = near / tri->zs[2];

		tri->xs[0] = tri->xs[0] * m_ztx * w0 + centerX;
		tri->xs[1] = tri->xs[1] * m_ztx * w1 + centerX;
		tri->xs[2] = tri->xs[2] * m_ztx * w2 + centerX;

		if (tri->xs[0] < m_viewLeftAxis.origin.x && tri->xs[1] < m_viewLeftAxis.origin.x && tri->xs[2] < m_viewLeftAxis.origin.x) continue;
		if (tri->xs[0] > m_viewRightAxis.origin.x && tri->xs[1] > m_viewRightAxis.origin.x && tri->xs[2] > m_viewRightAxis.origin.x) continue;

		tri->ys[0] = centerY - tri->ys[0] * m_zty * w0;
		tri->ys[1] = centerY - tri->ys[1] * m_zty * w1;
		tri->ys[2] = centerY - tri->ys[2] * m_zty * w2;

		if (tri->ys[0] < m_viewTopAxis.origin.y && tri->ys[1] < m_viewTopAxis.origin.y && tri->ys[2] < m_viewTopAxis.origin.y) continue;
		if (tri->ys[0] > m_viewBottomAxis.origin.y && tri->ys[1] > m_viewBottomAxis.origin.y && tri->ys[2] > m_viewBottomAxis.origin.y) continue;

		// Prepare texture coordinates
		tri->zs[0] = w0;
		tri->zs[1] = w1;
		tri->zs[2] = w2;
		tri->us[0] *= w0;
		tri->us[1] *= w1;
		tri->us[2] *= w2;
		tri->vs[0] *= w0;
		tri->vs[1] *= w1;
		tri->vs[2] *= w2;

		dstIndices[k++] = j;
	}
	return k;
}
//-----------------------------------------------------------------------------
int Renderer::clip3D(Triangle tris[], const int srcIndices[], int dstIndices[], int nb, Plane& plane)
{
	int k = 0;
	for (int i = 0; i < nb; i++)
	{
		int s = 0;
		const int j = srcIndices[i];

		// Project against clipping plane
		Triangle* tri = &tris[j];
		const float pj1 = (tri->xs[0] - plane.zAxis.origin.x) * plane.zAxis.axis.x + (tri->ys[0] - plane.zAxis.origin.y) * plane.zAxis.axis.y + (tri->zs[0] - plane.zAxis.origin.z) * plane.zAxis.axis.z;
		const float pj2 = (tri->xs[1] - plane.zAxis.origin.x) * plane.zAxis.axis.x + (tri->ys[1] - plane.zAxis.origin.y) * plane.zAxis.axis.y + (tri->zs[1] - plane.zAxis.origin.z) * plane.zAxis.axis.z;
		const float pj3 = (tri->xs[2] - plane.zAxis.origin.x) * plane.zAxis.axis.x + (tri->ys[2] - plane.zAxis.origin.y) * plane.zAxis.axis.y + (tri->zs[2] - plane.zAxis.origin.z) * plane.zAxis.axis.z;

		// Compute triangle intersections
		float nx[4], ny[4], nz[4];
		float nu[4], nv[4];
		if (pj1 >= 0.0f)
		{
			nx[s] = tri->xs[0];
			ny[s] = tri->ys[0];
			nz[s] = tri->zs[0];
			nu[s] = tri->us[0];
			nv[s++] = tri->vs[0];
		}
		if (pj1 * pj2 < 0.0f)
		{
			const float ratio = cmabs(pj1 / (pj1 - pj2));
			nx[s] = tri->xs[0] + ratio * (tri->xs[1] - tri->xs[0]);
			ny[s] = tri->ys[0] + ratio * (tri->ys[1] - tri->ys[0]);
			nz[s] = tri->zs[0] + ratio * (tri->zs[1] - tri->zs[0]);
			nu[s] = tri->us[0] + ratio * (tri->us[1] - tri->us[0]);
			nv[s++] = tri->vs[0] + ratio * (tri->vs[1] - tri->vs[0]);
		}
		if (pj2 >= 0.0f)
		{
			nx[s] = tri->xs[1];
			ny[s] = tri->ys[1];
			nz[s] = tri->zs[1];
			nu[s] = tri->us[1];
			nv[s++] = tri->vs[1];
		}
		if (pj2 * pj3 < 0.0f)
		{
			const float ratio = cmabs(pj2 / (pj2 - pj3));
			nx[s] = tri->xs[1] + ratio * (tri->xs[2] - tri->xs[1]);
			ny[s] = tri->ys[1] + ratio * (tri->ys[2] - tri->ys[1]);
			nz[s] = tri->zs[1] + ratio * (tri->zs[2] - tri->zs[1]);
			nu[s] = tri->us[1] + ratio * (tri->us[2] - tri->us[1]);
			nv[s++] = tri->vs[1] + ratio * (tri->vs[2] - tri->vs[1]);
		}
		if (pj3 >= 0.0f)
		{
			nx[s] = tri->xs[2];
			ny[s] = tri->ys[2];
			nz[s] = tri->zs[2];
			nu[s] = tri->us[2];
			nv[s++] = tri->vs[2];
		}
		if (pj3 * pj1 < 0.0f)
		{
			const float ratio = cmabs(pj3 / (pj3 - pj1));
			nx[s] = tri->xs[2] + ratio * (tri->xs[0] - tri->xs[2]);
			ny[s] = tri->ys[2] + ratio * (tri->ys[0] - tri->ys[2]);
			nz[s] = tri->zs[2] + ratio * (tri->zs[0] - tri->zs[2]);
			nu[s] = tri->us[2] + ratio * (tri->us[0] - tri->us[2]);
			nv[s++] = tri->vs[2] + ratio * (tri->vs[0] - tri->vs[2]);
		}
		// Build triangle list
		if (s >= 3)
		{
			// Correct triangle coordinates
			tri->xs[0] = nx[0];
			tri->xs[1] = nx[1];
			tri->xs[2] = nx[2];
			tri->ys[0] = ny[0];
			tri->ys[1] = ny[1];
			tri->ys[2] = ny[2];
			tri->zs[0] = nz[0];
			tri->zs[1] = nz[1];
			tri->zs[2] = nz[2];
			tri->us[0] = nu[0];
			tri->us[1] = nu[1];
			tri->us[2] = nu[2];
			tri->vs[0] = nv[0];
			tri->vs[1] = nv[1];
			tri->vs[2] = nv[2];

			// Correct view distance
			const float dx = nx[0] + nx[1] + nx[2];
			const float dy = ny[0] + ny[1] + ny[2];
			const float dz = nz[0] + nz[1] + nz[2];
			tri->vd = dx * dx + dy * dy + dz * dz - m_vOffset;
			dstIndices[k++] = j;
		}
		if (s >= 4 && m_extra < m_extraMax)
		{
			// Copy triangle coordinates
			Triangle* ntri = &tris[m_extra];
			ntri->xs[0] = nx[0];
			ntri->xs[1] = nx[2];
			ntri->xs[2] = nx[3];
			ntri->ys[0] = ny[0];
			ntri->ys[1] = ny[2];
			ntri->ys[2] = ny[3];
			ntri->zs[0] = nz[0];
			ntri->zs[1] = nz[2];
			ntri->zs[2] = nz[3];
			ntri->us[0] = nu[0];
			ntri->us[1] = nu[2];
			ntri->us[2] = nu[3];
			ntri->vs[0] = nv[0];
			ntri->vs[1] = nv[2];
			ntri->vs[2] = nv[3];

			// Compute new view distance
			const float dx = nx[0] + nx[2] + nx[3];
			const float dy = ny[0] + ny[2] + ny[3];
			const float dz = nz[0] + nz[2] + nz[3];
			ntri->vd = dx * dx + dy * dy + dz * dz - m_vOffset;

			// Copy other parameters
			ntri->solidColor = tri->solidColor;
			ntri->diffuseTexture = tri->diffuseTexture;
			ntri->flags = tri->flags;

			dstIndices[k++] = m_extra++;
		}
	}
	return k;
}
//-----------------------------------------------------------------------------
int Renderer::clip2D(Triangle tris[], const int srcIndices[], int dstIndices[], int nb, Axis& axis)
{
	int k = 0;
	for (int i = 0; i < nb; i++)
	{
		int s = 0;
		const int j = srcIndices[i];

		// Project against clipping line
		Triangle* tri = &tris[j];
		const float pj1 = (tri->xs[0] - axis.origin.x) * axis.axis.y - (tri->ys[0] - axis.origin.y) * axis.axis.x;
		const float pj2 = (tri->xs[1] - axis.origin.x) * axis.axis.y - (tri->ys[1] - axis.origin.y) * axis.axis.x;
		const float pj3 = (tri->xs[2] - axis.origin.x) * axis.axis.y - (tri->ys[2] - axis.origin.y) * axis.axis.x;

		// Compute intersections
		float nx[4], ny[4], nz[4];
		float nu[4], nv[4];
		if (pj1 >= 0.0f)
		{
			nx[s] = tri->xs[0];
			ny[s] = tri->ys[0];
			nz[s] = tri->zs[0];
			nu[s] = tri->us[0];
			nv[s++] = tri->vs[0];
		}
		if (pj1 * pj2 < 0.0f)
		{
			const float ratio = cmabs(pj1 / (pj1 - pj2));
			nx[s] = tri->xs[0] + ratio * (tri->xs[1] - tri->xs[0]);
			ny[s] = tri->ys[0] + ratio * (tri->ys[1] - tri->ys[0]);
			nz[s] = tri->zs[0] + ratio * (tri->zs[1] - tri->zs[0]);
			nu[s] = tri->us[0] + ratio * (tri->us[1] - tri->us[0]);
			nv[s++] = tri->vs[0] + ratio * (tri->vs[1] - tri->vs[0]);
		}
		if (pj2 >= 0.0f)
		{
			nx[s] = tri->xs[1];
			ny[s] = tri->ys[1];
			nz[s] = tri->zs[1];
			nu[s] = tri->us[1];
			nv[s++] = tri->vs[1];
		}
		if (pj2 * pj3 < 0.0f)
		{
			const float ratio = cmabs(pj2 / (pj2 - pj3));
			nx[s] = tri->xs[1] + ratio * (tri->xs[2] - tri->xs[1]);
			ny[s] = tri->ys[1] + ratio * (tri->ys[2] - tri->ys[1]);
			nz[s] = tri->zs[1] + ratio * (tri->zs[2] - tri->zs[1]);
			nu[s] = tri->us[1] + ratio * (tri->us[2] - tri->us[1]);
			nv[s++] = tri->vs[1] + ratio * (tri->vs[2] - tri->vs[1]);
		}
		if (pj3 >= 0.0f)
		{
			nx[s] = tri->xs[2];
			ny[s] = tri->ys[2];
			nz[s] = tri->zs[2];
			nu[s] = tri->us[2];
			nv[s++] = tri->vs[2];
		}
		if (pj3 * pj1 < 0.0f)
		{
			const float ratio = cmabs(pj3 / (pj3 - pj1));
			nx[s] = tri->xs[2] + ratio * (tri->xs[0] - tri->xs[2]);
			ny[s] = tri->ys[2] + ratio * (tri->ys[0] - tri->ys[2]);
			nz[s] = tri->zs[2] + ratio * (tri->zs[0] - tri->zs[2]);
			nu[s] = tri->us[2] + ratio * (tri->us[0] - tri->us[2]);
			nv[s++] = tri->vs[2] + ratio * (tri->vs[0] - tri->vs[2]);
		}
		// Build triangle list
		if (s >= 3)
		{
			// Correct the triangle
			tri->xs[0] = nx[0];
			tri->xs[1] = nx[1];
			tri->xs[2] = nx[2];
			tri->ys[0] = ny[0];
			tri->ys[1] = ny[1];
			tri->ys[2] = ny[2];
			tri->zs[0] = nz[0];
			tri->zs[1] = nz[1];
			tri->zs[2] = nz[2];
			tri->us[0] = nu[0];
			tri->us[1] = nu[1];
			tri->us[2] = nu[2];
			tri->vs[0] = nv[0];
			tri->vs[1] = nv[1];
			tri->vs[2] = nv[2];

			dstIndices[k++] = j;
		}
		if (s >= 4 && m_extra < m_extraMax)
		{
			// Copy triangle coordinates
			Triangle* ntri = &tris[m_extra];
			ntri->xs[0] = nx[0];
			ntri->xs[1] = nx[2];
			ntri->xs[2] = nx[3];
			ntri->ys[0] = ny[0];
			ntri->ys[1] = ny[2];
			ntri->ys[2] = ny[3];
			ntri->zs[0] = nz[0];
			ntri->zs[1] = nz[2];
			ntri->zs[2] = nz[3];
			ntri->us[0] = nu[0];
			ntri->us[1] = nu[2];
			ntri->us[2] = nu[3];
			ntri->vs[0] = nv[0];
			ntri->vs[1] = nv[2];
			ntri->vs[2] = nv[3];
			ntri->vd = tri->vd;

			// Copy other parameters
			ntri->solidColor = tri->solidColor;
			ntri->diffuseTexture = tri->diffuseTexture;
			ntri->flags = tri->flags;

			dstIndices[k++] = m_extra++;
		}
	}
	return k;
}
//-----------------------------------------------------------------------------
int Renderer::backculling(Triangle tris[], const int srcIndices[], int dstIndices[], int nb)
{
	if( m_backMode == BACKCULLING_NONE )
	{
		memcpy(dstIndices, srcIndices, nb * (sizeof(int)));
		return nb;
	}

	if( m_backMode == BACKCULLING_CCW )
	{
		int k = 0;
		for( int i = 0; i < nb; i++ )
		{
			const int j = srcIndices[i];
			Triangle* tri = &tris[j];
			float dir;
			dir = (tri->xs[1] - tri->xs[0]) * (tri->ys[2] - tri->ys[0]);
			dir -= (tri->ys[1] - tri->ys[0]) * (tri->xs[2] - tri->xs[0]);
			if( dir > 0.0f ) continue;
			dstIndices[k++] = j;
		}
		return k;
	}

	if( m_backMode == BACKCULLING_CW )
	{
		int k = 0;
		for( int i = 0; i < nb; i++ )
		{
			const int j = srcIndices[i];
			Triangle* tri = &tris[j];
			float dir;
			dir = (tri->xs[1] - tri->xs[0]) * (tri->ys[2] - tri->ys[0]);
			dir -= (tri->ys[1] - tri->ys[0]) * (tri->xs[2] - tri->xs[0]);
			if( dir < 0.0f ) continue;
			dstIndices[k++] = j;
		}
		return k;
	}

	if( m_backMode == BACKCULLING_CCW_ALPHA )
	{
		int k = 0;
		for( int i = 0; i < nb; i++ )
		{
			const int j = srcIndices[i];
			Triangle* tri = &tris[j];
			if( !(tri->flags & TRIANGLE_BLENDED) )
			{
				float dir;
				dir = (tri->xs[1] - tri->xs[0]) * (tri->ys[2] - tri->ys[0]);
				dir -= (tri->ys[1] - tri->ys[0]) * (tri->xs[2] - tri->xs[0]);
				if( dir > 0.0f ) continue;
			}
			dstIndices[k++] = j;
		}
		return k;
	}

	if( m_backMode == BACKCULLING_CW_ALPHA )
	{
		int k = 0;
		for( int i = 0; i < nb; i++ )
		{
			const int j = srcIndices[i];
			Triangle* tri = &tris[j];
			if( !(tri->flags & TRIANGLE_BLENDED) )
			{
				float dir;
				dir = (tri->xs[1] - tri->xs[0]) * (tri->ys[2] - tri->ys[0]);
				dir -= (tri->ys[1] - tri->ys[0]) * (tri->xs[2] - tri->xs[0]);
				if( dir < 0.0f ) continue;
			}
			dstIndices[k++] = j;
		}
		return k;
	}

	return 0;
}
//-----------------------------------------------------------------------------
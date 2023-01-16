#pragma once

// Meshes and billboard sets renderer

/*****************************************************************************
	The renderer assumes a right handed coordinate system:
	- X goes right
	- Y goes top
	- Z goes backward
******************************************************************************/

#include "Core.h"
#include "Mesh.h"
#include "Billboard.h"
#include "TriangleList.h"
#include "VertexList.h"

enum BACKCULLING_MODES
{
	BACKCULLING_NONE = 0,  // Backculling is disabled
	BACKCULLING_CCW,       // Backculling in counterclockwise mode (default)
	BACKCULLING_CW,        // Backculling in clockwise mode
	BACKCULLING_CCW_ALPHA, // Backculling in counterclockwise mode, transparent triangles are double sided
	BACKCULLING_CW_ALPHA,  // Backculling in clockwise mode, transparent triangles are double sided
};

// Render meshes and billboard sets to 2D triangle lista
class Renderer
{
public:
	Renderer(int width, int height);

	void Render(const Mesh* mesh);
	void Render(const BillboardSet* bset);
	void Flush();

	int GetViewportCoordinates(const Vertex& pos, Vertex& viewCoords) const;

	void SetViewPosition(const Vertex& pos);
	void SetViewAngle(const Vertex& angleYZX);
	void UpdateViewMatrix();

	void SetViewMatrix(const Matrix& view);

	void SetViewport(int left, int top, int right, int bottom);
	void SetViewClipping(float near, float far);
	void SetViewProjection(float fov);
	void SetViewOffset(float offset);

	void SetBackcullingMode(BACKCULLING_MODES mode);

	void SetFog(bool enable);
	void SetFogProperties(Color color, float near, float far);

	void SetMipmapping(bool enable);

	void SetTriangleList(TriangleList* trilist);
	TriangleList* GetTriangleList();

private:
	bool checkMemory(size_t noVertexes, size_t noTriangles);

	int build(const Mesh* mesh, Vertex vertexes[], Triangle tris[], int indices[]);
	int build(const BillboardSet* bset, Vertex vertexes[], Triangle tris[], int indices[]);

	void updateFrustrum();

	void transform(const Matrix &matrix, const Vertex srcVertexes[], Vertex dstVertexes[], int nb) const;
	int project(Triangle tris[], const int srcIndices[], int dstIndices[], int nb);
	int clip3D(Triangle tris[], const int srcIndices[], int dstIndices[], int nb, Plane& plane);
	int clip2D(Triangle tris[], const int srcIndices[], int dstIndices[], int nb, Axis& axis);
	int backculling(Triangle tris[], const int srcIndices[], int dstIndices[], int nb);

	VertexList m_intVerlist;                        // Internal vertex list
	TriangleList m_intTrilist;                      // Internal triangle list
	VertexList* m_usedVerlist;                      // Current vertex list in use
	TriangleList* m_usedTrilist;                    // Current triangle list in use

	int m_extra = 0;                                // Index of extra triangles
	int m_extraMax = 0;                             // Maximum number of extra triangles

	Color* m_colors = nullptr;                      // Color table for triangles

	Vertex m_viewPosition;                          // View position of renderer
	Vertex m_viewAngle;                             // View angle of renderer (in degrees)
	Matrix m_viewMatrix;                            // View matrix of renderer

	Plane m_viewFrontPlan;                          // Front clipping plane
	Plane m_viewBackPlan;                           // Back clipping plane

	Plane m_viewLeftPlan;                           // 3D frustrum left clipping plane
	Plane m_viewRightPlan;                          // 3D frustrum right clipping plane
	Plane m_viewTopPlan;                            // 3D frustrum top clipping plane
	Plane m_viewBotPlan;                            // 3D frustrum bot clipping plane

	Axis m_viewLeftAxis;                            // 2D left clipping axis
	Axis m_viewRightAxis;                           // 2D right clipping axis
	Axis m_viewTopAxis;                             // 2D top clipping axis
	Axis m_viewBottomAxis;                          // 2D bottom clipping axis
	float m_viewFov = RENDERER_FOV_DEFAULT;         // Viewport field-of-view

	float m_ztx = 1.0f;                             // Horizontal projection factor
	float m_zty = 1.0f;                             // Vertical projection factor

	float m_vOffset = 0.0f;                         // Distance view offset

	BACKCULLING_MODES m_backMode = BACKCULLING_CCW; // Backculling mode
	bool m_mipmappingEnable = true;                 // Mipmapping enable state
	bool m_fogEnable = false;                       // Fog enable state
};
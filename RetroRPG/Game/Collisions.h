#pragma once

#include "Core.h"
#include "Geometry.h"
#include "Mesh.h"

namespace Collisions 
{
	// Return values of the collideRectRect routine
	enum COLLISIONS_RECT_RESULT 
	{
		COLLISIONS_RECT_NO_COL = 0x00,     // No collision has occured
		COLLISIONS_RECT_LEFT_COL = 0x01,   // Left side (negative x) collision has occured
		COLLISIONS_RECT_RIGHT_COL = 0x02,  // Right side (positive x) collision has occured
		COLLISIONS_RECT_TOP_COL = 0x03,    // Top side (negative y) collision has occured
		COLLISIONS_RECT_BOTTOM_COL = 0x04, // Bottom side (positive y) collision has occured
	};

	// Return values of the collideSphereSphere routine
	enum COLLISIONS_SPHERE_RESULT
	{
		COLLISIONS_SPHERE_NO_COL = 0x00,  // No collision has occured
		COLLISIONS_SPHERE_SURFACE = 0x01, // Surface collision has occured
	};
	
	// Return values of the collideSphereBox routine
	enum COLLISIONS_BOX_RESULT 
	{
		COLLISIONS_BOX_NO_COL = 0x00, // No collision has occured
		COLLISIONS_BOX_SIDE = 0x01,   // Side collision has occured
		COLLISIONS_BOX_EDGE = 0x02,   // Edge collision has occured
		COLLISIONS_BOX_CORNER = 0x03, // Corner collision has occured
	};
	
	// Return values of the collideSphereMesh routine
	enum COLLISIONS_MESH_RESULT
	{
		COLLISIONS_MESH_NO_COL = 0x00, // No collision has occured
		COLLISIONS_MESH_SIDE = 0x01,   // Side collision has occured
		COLLISIONS_MESH_EDGE = 0x02,   // Edge collision has occured
	};

	// Return values of the traceBox routine
	enum COLLISIONS_BOX_TRACE_RESULT 
	{
		COLLISIONS_BOX_TRACE_NO_INTER = 0x00, // No intersection has occured
		COLLISIONS_BOX_TRACE_LEFT = 0x01,     // Intersection with left side (negative x)
		COLLISIONS_BOX_TRACE_RIGHT = 0x02,    // Intersection with right side (positive x)
		COLLISIONS_BOX_TRACE_BOTTOM = 0x03,   // Intersection with bottom side (negative y)
		COLLISIONS_BOX_TRACE_TOP = 0x04,      // Intersection with top side (positive y)
		COLLISIONS_BOX_TRACE_BACK = 0x05,     // Intersection with back side (negative z)
		COLLISIONS_BOX_TRACE_FRONT = 0x06,    // Intersection with front side (positive z)
	};

	int collideRectRect(float& ansX, float& ansY, float srcX, float srcY, float srcW, float srcH, float dstX, float dstY, float dstW, float dstH);
	int collideSphereSphere(Vertex& ans, Vertex& contact, const Vertex& pos, float srcRadius, float dstRadius);
	int collideSphereBox(Vertex& ans, Vertex& contact, const Vertex& pos, float srcRadius, const Vertex& dstSize);
	int collideSphereMesh(Vertex& ans, Vertex& contact, const Vertex& pos, float srcRadius, const Mesh* dstMesh);

	int traceSphere(const Vertex& pos, float radius, const Vertex& axis, float& distance);
	int traceBox(const Vertex& pos, const Vertex& size, const Vertex& axis, float& distance);
	int traceMesh(const Mesh* mesh, const Axis& axis, float& distance);

}
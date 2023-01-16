#include "Collisions.h"
#include <float.h>

int Collisions::collideRectRect(float& ansX, float& ansY, float srcX, float srcY, float srcW, float srcH, float dstX, float dstY, float dstW, float dstH)
{
	float mx = (srcW + dstW) * 0.5f;
	float my = (srcH + dstH) * 0.5f;
	float dx = fabsf(dstX - srcX);
	float dy = fabsf(dstY - srcY);

	if (dx < mx && dy < my) {
		float px = mx - dx;
		float py = my - dy;
		if (px < py) {
			if (dstX > srcX) {
				ansX -= px;
				return COLLISIONS_RECT_LEFT_COL;
			}
			else {
				ansX += px;
				return COLLISIONS_RECT_RIGHT_COL;
			}
		}
		else {
			if (dstY > srcY) {
				ansY -= py;
				return COLLISIONS_RECT_TOP_COL;
			}
			else {
				ansY += py;
				return COLLISIONS_RECT_BOTTOM_COL;
			}
		}
	}
	return COLLISIONS_RECT_NO_COL;
}

int Collisions::collideSphereSphere(Vertex& ans, Vertex& contact, const Vertex& pos, float srcRadius, float dstRadius)
{
	float rt = srcRadius + dstRadius;

	ans = Primitives::zero;
	contact = Primitives::zero;

	float n2 = pos.Dot(pos);
	if (n2 > rt * rt) return COLLISIONS_SPHERE_NO_COL;
	if (n2 == 0.0f) return COLLISIONS_SPHERE_SURFACE;

	float n = sqrtf(n2);
	Vertex dir = pos * (1.0f / n);
	ans = dir * (rt - n);
	contact = dir * srcRadius;

	return COLLISIONS_SPHERE_SURFACE;
}

int Collisions::traceSphere(const Vertex& pos, float radius, const Vertex& axis, float& distance)
{
	return 0;
}

int Collisions::collideSphereBox(Vertex& ans, Vertex& contact, const Vertex& pos, float srcRadius, const Vertex& dstSize)
{
	const float lx = srcRadius + dstSize.x;
	const float ly = srcRadius + dstSize.y;
	const float lz = srcRadius + dstSize.z;

	// Outside of the area
	ans = Primitives::zero;
	contact = Primitives::zero;

	if (pos.x > lx)	 return COLLISIONS_BOX_NO_COL;
	if (pos.x < -lx) return COLLISIONS_BOX_NO_COL;
	if (pos.y > ly)	 return COLLISIONS_BOX_NO_COL;
	if (pos.y < -ly) return COLLISIONS_BOX_NO_COL;
	if (pos.z > lz)	 return COLLISIONS_BOX_NO_COL;
	if (pos.z < -lz) return COLLISIONS_BOX_NO_COL;

	float dx = fabsf(pos.x);
	float dy = fabsf(pos.y);
	float dz = fabsf(pos.z);

	// Touching a side
	if (dx > dstSize.x) {
		if (dy <= dstSize.y && dz <= dstSize.z) {
			float sx = cmsgn(pos.x);
			ans.x = (lx - dx) * sx;
			contact = pos;
			contact.x = dstSize.x * sx;
			return COLLISIONS_BOX_SIDE;
		}
	}

	if (dy > dstSize.y) {
		if (dx <= dstSize.x && dz <= dstSize.z) {
			float sy = cmsgn(pos.y);
			ans.y = (ly - dy) * sy;
			contact = pos;
			contact.y = dstSize.y * sy;
			return COLLISIONS_BOX_SIDE;
		}
	}

	if (dz > dstSize.z) {
		if (dx <= dstSize.x && dy <= dstSize.y) {
			float sz = cmsgn(pos.z);
			ans.z = (lz - dz) * sz;
			contact.z = dstSize.y * sz;
			return COLLISIONS_BOX_SIDE;
		}
	}

	// Touching an edge
	int res = COLLISIONS_BOX_CORNER;
	Vertex corner = dstSize * pos.Sign();
	if (dx < dstSize.x) {
		corner.x = pos.x;
		res = COLLISIONS_BOX_EDGE;
	}
	if (dy < dstSize.y) {
		corner.y = pos.y;
		res = COLLISIONS_BOX_EDGE;
	}
	if (dz < dstSize.z) {
		corner.z = pos.z;
		res = COLLISIONS_BOX_EDGE;
	}

	// Touching a corner
	Vertex delta = pos - corner;
	float r = delta.Dot(delta);
	if (r > srcRadius * srcRadius)
		return COLLISIONS_BOX_NO_COL;

	delta.Normalize();
	ans = delta * (srcRadius - sqrtf(r));
	contact = corner + delta * srcRadius;
	return res;
}

int Collisions::traceBox(const Vertex& pos, const Vertex& size, const Vertex& axis, float& distance)
{
	int result = -1;
	float dMin = FLT_MAX;

	// X sides
	if (axis.x != 0.0f) {
		float d = (pos.x + size.x) / axis.x;
		Vertex h = pos + axis * d;
		if (d >= 0.0f) {
			if (fabs(h.y) < size.y && fabs(h.z) < size.z) {
				if (d < dMin) {
					dMin = d;
					result = COLLISIONS_BOX_TRACE_RIGHT;
				}
			}
		}
		d = (pos.x - size.x) / axis.x;
		h = pos + axis * d;
		if (d >= 0.0f) {
			if (fabs(h.y) < size.y && fabs(h.z) < size.z) {
				if (d < dMin) {
					dMin = d;
					result = COLLISIONS_BOX_TRACE_LEFT;
				}
			}
		}
	}

	// Y sides
	if (axis.y != 0.0f) {
		float d = (pos.y + size.y) / axis.y;
		Vertex h = pos + axis * d;
		if (d >= 0.0f) {
			if (fabs(h.x) < size.x && fabs(h.z) < size.z) {
				if (d < dMin) {
					dMin = d;
					result = COLLISIONS_BOX_TRACE_TOP;
				}
			}
		}
		d = (pos.y - size.y) / axis.y;
		h = pos + axis * d;
		if (d >= 0.0f) {
			if (fabs(h.x) < size.x && fabs(h.z) < size.z) {
				if (d < dMin) {
					dMin = d;
					result = COLLISIONS_BOX_TRACE_BOTTOM;
				}
			}
		}
	}

	// Z sides
	if (axis.z != 0.0f) {
		float d = (pos.z + size.z) / axis.z;
		Vertex h = pos + axis * d;
		if (d >= 0.0f) {
			if (fabs(h.x) < size.x && fabs(h.y) < size.y) {
				if (d < dMin) {
					dMin = d;
					result = COLLISIONS_BOX_TRACE_FRONT;
				}
			}
		}
		d = (pos.z - size.z) / axis.z;
		h = pos + axis * d;
		if (d >= 0.0f) {
			if (fabs(h.x) < size.x && fabs(h.y) < size.y) {
				if (d < dMin) {
					dMin = d;
					result = COLLISIONS_BOX_TRACE_BACK;
				}
			}
		}
	}

	distance = dMin;
	return result;
}

int collideCircleSegment(Vertex& localAns, Vertex& localContact, const Vertex& v1, const Vertex& v2, const Vertex& c, float radius2);
int Collisions::collideSphereMesh(Vertex& ans, Vertex& contact, const Vertex& pos, float srcRadius, const Mesh* dstMesh)
{
	ans = Primitives::zero;
	contact = Primitives::zero;

	Matrix mt;
	mt.Scale(dstMesh->scale);
	mt.RotateEulerYZX(dstMesh->angle * d2r);

	if (!dstMesh->normals) return COLLISIONS_MESH_NO_COL;
	for (int t = 0; t < dstMesh->noTriangles; t++) {
		// 1: Compute distance to plan
		Vertex n = dstMesh->normals[t];
		Vertex v1 = mt * dstMesh->vertexes[dstMesh->vertexesList[t * 3]];
		float d = n.Dot(pos - v1);
		if (d > srcRadius || d < 0.0f) continue;

		// 2: Check intersection in triangle
		Vertex h = pos - n * d;
		Vertex v2 = mt * dstMesh->vertexes[dstMesh->vertexesList[t * 3 + 1]];
		Vertex v3 = mt * dstMesh->vertexes[dstMesh->vertexesList[t * 3 + 2]];
		float n1 = n.Dot((v1 - h).Cross(v2 - h));
		if (n1 < 0.0f) continue;
		float n2 = n.Dot((v2 - h).Cross(v3 - h));
		if (n2 < 0.0f) continue;
		float n3 = n.Dot((v3 - h).Cross(v1 - h));
		if (n3 < 0.0f) continue;

		ans = n * (srcRadius - d);
		contact = h;
		return COLLISIONS_MESH_SIDE;
	}

	for (int t = 0; t < dstMesh->noTriangles; t++) {
		// 3: Compute intersection with edges
		Vertex v1 = mt * dstMesh->vertexes[dstMesh->vertexesList[t * 3]];
		Vertex v2 = mt * dstMesh->vertexes[dstMesh->vertexesList[t * 3 + 1]];
		Vertex v3 = mt * dstMesh->vertexes[dstMesh->vertexesList[t * 3 + 2]];

		Vertex localContact, localAns;
		int n = collideCircleSegment(localAns, localContact, v1, v2, pos, srcRadius);
		n += collideCircleSegment(localAns, localContact, v2, v3, pos, srcRadius);
		n += collideCircleSegment(localAns, localContact, v3, v1, pos, srcRadius);
		if (!n) continue;

		float in = 1.0f / n;
		ans = localAns * in;
		contact = localContact * in;
		return COLLISIONS_MESH_EDGE;
	}

	return COLLISIONS_MESH_NO_COL;
}

int collideCircleSegment(Vertex& ans, Vertex& contact, const Vertex& v1, const Vertex& v2, const Vertex& c, float radius)
{
	Vertex d = v2 - v1;
	Vertex r = v1 - c;

	float a = d.Dot(d);
	float b = 2.0f * d.Dot(r);
	float g = r.Dot(r) - radius * radius;

	float delta = b * b - 4.0f * a * g;
	if (delta < 0.0f) return 0;
	float sd = sqrtf(delta);

	float t1 = 0.5f * (-b - sd) / a;
	t1 = cmbound(t1, 0.0f, 1.0f);
	float t2 = 0.5f * (-b + sd) / a;
	t2 = cmbound(t2, 0.0f, 1.0f);
	if (t1 == t2) return 0;

	Vertex h = v1 + d * ((t1 + t2) * 0.5f);
	Vertex n = c - h;
	float t = n.Norm();

	ans += n * ((radius - t) / t);
	contact += h;
	return 1;
}

int Collisions::traceMesh(const Mesh* mesh, const Axis& axis, float& distance)
{
	int result = -1;
	float dMin = FLT_MAX;

	if (!mesh->normals) return -1;
	for (int t = 0; t < mesh->noTriangles; t++) {
		// 1: Check if intersection with plan
		Vertex n = mesh->normals[t];
		float a = n.Dot(axis.axis);
		if (a >= 0.0f) continue;

		// 2: Check distance to plan
		Vertex v1 = mesh->vertexes[mesh->vertexesList[t * 3]];
		float d = n.Dot(v1 - axis.origin) / a;
		if (d >= dMin) continue;

		// 3: Check if intersection in triangle
		Vertex h = axis.origin + axis.axis * d;
		Vertex v2 = mesh->vertexes[mesh->vertexesList[t * 3 + 1]];
		Vertex v3 = mesh->vertexes[mesh->vertexesList[t * 3 + 2]];

		float n1 = n.Dot((v1 - h).Cross(v2 - h));
		if (n1 < 0.0f) continue;
		float n2 = n.Dot((v2 - h).Cross(v3 - h));
		if (n2 < 0.0f) continue;
		float n3 = n.Dot((v3 - h).Cross(v1 - h));
		if (n3 < 0.0f) continue;

		dMin = d;
		result = t;
	}

	distance = dMin;
	return result;
}
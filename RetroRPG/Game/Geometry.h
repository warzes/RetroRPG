#pragma once

#include "Core.h"
// Intrinsics includes
#if USE_SSE2
#	include <xmmintrin.h>
#	include <emmintrin.h>
#endif // USE_SSE2
#include <math.h>

// Represent a vertex in 3D space
struct Vertex
{
	float x, y, z, w;

	Vertex()
	{
		x = 0.0f;
		y = 0.0f;
		z = 0.0f;
		w = 1.0f;
	}

	Vertex(float px, float py, float pz)
	{
		x = px;
		y = py;
		z = pz;
		w = 1.0f;
	}

	static Vertex Spherical(float azi, float inc, float dist)
	{
		Vertex r;
		r.x = cosf(azi) * cosf(inc) * dist;
		r.y = sinf(inc) * dist;
		r.z = -sinf(azi) * cosf(inc) * dist;
		return r;
	}

	Vertex operator+(Vertex v) const
	{
		Vertex r;
		r.x = x + v.x;
		r.y = y + v.y;
		r.z = z + v.z;
		return r;
	}

	Vertex operator+=(Vertex v)
	{
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Vertex operator-(Vertex v) const
	{
		Vertex r;
		r.x = x - v.x;
		r.y = y - v.y;
		r.z = z - v.z;
		return r;
	}

	Vertex operator-() const
	{
		Vertex r;
		r.x = -x;
		r.y = -y;
		r.z = -z;
		return r;
	}

	Vertex operator-=(Vertex v)
	{
		x -= v.x;
		y -= v.y;
		z -= v.z;
		return *this;
	}

	Vertex operator*(Vertex v) const
	{
		Vertex r;
		r.x = x * v.x;
		r.y = y * v.y;
		r.z = z * v.z;
		return r;
	}

	Vertex operator/(Vertex v) const
	{
		Vertex r;
		r.x = x / v.x;
		r.y = y / v.y;
		r.z = z / v.z;
		return r;
	}

	Vertex operator*(float v) const
	{
		Vertex r;
		r.x = x * v;
		r.y = y * v;
		r.z = z * v;
		return r;
	}

	Vertex operator/(float v) const
	{
		float iv = 1.0f / v;
		Vertex r;
		r.x = x * iv;
		r.y = y * iv;
		r.z = z * iv;
		return r;
	}

	Vertex operator*=(Vertex v)
	{
		x *= v.x;
		y *= v.y;
		z *= v.z;
		return *this;
	}

	Vertex operator/=(Vertex v)
	{
		x /= v.x;
		y /= v.y;
		z /= v.z;
		return *this;
	}

	Vertex operator*=(float v)
	{
		x *= v;
		y *= v;
		z *= v;
		return *this;
	}

	Vertex operator/=(float v)
	{
		float iv = 1.0f / v;
		x *= iv;
		y *= iv;
		z *= iv;
		return *this;
	}

	bool operator==(Vertex v)
	{
		return x == v.x && y == v.y && z == v.z;
	}

	float Dot(Vertex v) const
	{
		return x * v.x + y * v.y + z * v.z;
	}

	Vertex Cross(Vertex v) const
	{
		Vertex c;
		c.x = y * v.z - v.y * z;
		c.y = -x * v.z + v.x * z;
		c.z = x * v.y - v.x * y;
		return c;
	}

	Vertex Sign() const
	{
		Vertex c;
		c.x = copysignf(1.0f, x);
		c.y = copysignf(1.0f, y);
		c.z = copysignf(1.0f, z);
		c.w = copysignf(1.0f, w);
		return c;
	}

	Vertex Round() const
	{
		Vertex c;
		c.x = floorf(x + 0.5f);
		c.y = floorf(y + 0.5f);
		c.z = floorf(z + 0.5f);
		c.w = floorf(w + 0.5f);
		return c;
	}

	Vertex Floor() const
	{
		Vertex c;
		c.x = floorf(x);
		c.y = floorf(y);
		c.z = floorf(z);
		c.w = floorf(w);
		return c;
	}

	Vertex Normalize()
	{
		float d = Norm();
		if (d == 0.0f)
		{
			x = y = z = 0.0f;
			w = 1.0f;
		}
		else
		{
			d = 1.0f / d;
			x *= d;
			y *= d;
			z *= d;
			w *= d;
		}
		return *this;
	}

	float Norm() const
	{
		return sqrtf(Dot(*this));
	}
};

// Represent an axis in 3D space
struct Axis
{
	Vertex origin; // Origin of the axis
	Vertex axis; // Direction of the axis (normalized)
	float norm; // Length of the axis

	Axis()
	{
		origin.x = 0.0f;
		origin.y = 0.0f;
		origin.z = 0.0f;
		axis.x = 0.0f;
		axis.y = 0.0f;
		axis.z = 1.0f;
		norm = 1.0f;
	}

	Axis(Vertex v1, Vertex v2)
	{
		origin.x = v1.x;
		origin.y = v1.y;
		origin.z = v1.z;
		float dx = v2.x - v1.x;
		float dy = v2.y - v1.y;
		float dz = v2.z - v1.z;
		norm = sqrtf(dx * dx + dy * dy + dz * dz);
		axis.x = dx / norm;
		axis.y = dy / norm;
		axis.z = dz / norm;
	}
};

// Represent a plane in 3D space
struct Plane
{
	Axis xAxis;
	Axis yAxis;
	Axis zAxis;

	Plane()
	{
		xAxis.axis.x = 1.0f;
		xAxis.axis.y = 0.0f;
		xAxis.axis.z = 0.0f;

		yAxis.axis.x = 0.0f;
		yAxis.axis.y = 1.0f;
		yAxis.axis.z = 0.0f;

		zAxis.axis.x = 0.0f;
		zAxis.axis.y = 0.0f;
		zAxis.axis.z = 1.0f;
	}

	Plane(Vertex v1, Vertex v2, Vertex v3) : xAxis(Axis(v1, v2)), yAxis(Axis(v1, v3))
	{
		Vertex tv1, tv2;
		tv1.x = xAxis.axis.z * yAxis.axis.y;
		tv1.y = xAxis.axis.x * yAxis.axis.z;
		tv1.z = xAxis.axis.y * yAxis.axis.x;
		tv2.x = xAxis.axis.y * yAxis.axis.z;
		tv2.y = xAxis.axis.z * yAxis.axis.x;
		tv2.z = xAxis.axis.x * yAxis.axis.y;
		zAxis = Axis(tv1, tv2);
		zAxis.origin.x = v1.x;
		zAxis.origin.y = v1.y;
		zAxis.origin.z = v1.z;
	}
};

// Represent a 4x4 matrix to handle 3D transforms
struct Matrix
{
	float mat[4][4];

	Matrix()
	{
		Identity();
	}

	void Identity()
	{
		mat[0][0] = 1.0f;
		mat[0][1] = 0.0f;
		mat[0][2] = 0.0f;
		mat[0][3] = 0.0f;

		mat[1][0] = 0.0f;
		mat[1][1] = 1.0f;
		mat[1][2] = 0.0f;
		mat[1][3] = 0.0f;

		mat[2][0] = 0.0f;
		mat[2][1] = 0.0f;
		mat[2][2] = 1.0f;
		mat[2][3] = 0.0f;

		mat[3][0] = 0.0f;
		mat[3][1] = 0.0f;
		mat[3][2] = 0.0f;
		mat[3][3] = 1.0f;
	}

	void Zero()
	{
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				mat[i][j] = 0;
	}

	void Transpose()
	{
		Matrix m;
		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				m.mat[j][i] = mat[i][j];
		*this = m;
	}

	void Translate(Vertex d)
	{
		mat[0][3] += d.x;
		mat[1][3] += d.y;
		mat[2][3] += d.z;
	}

	void Scale(Vertex s)
	{
		Matrix m;
		m.mat[0][0] *= s.x;
		m.mat[1][1] *= s.y;
		m.mat[2][2] *= s.z;
		*this = m * *this;
	}

	void RotateEulerXYZ(Vertex a)
	{
		RotateX(a.x);
		RotateY(a.y);
		RotateZ(a.z);
	}

	void RotateEulerXZY(Vertex a)
	{
		RotateX(a.x);
		RotateZ(a.z);
		RotateY(a.y);
	}

	void RotateEulerYZX(Vertex a)
	{
		RotateY(a.y);
		RotateZ(a.z);
		RotateX(a.x);
	}

	void RotateEulerYXZ(Vertex a)
	{
		RotateY(a.y);
		RotateX(a.x);
		RotateZ(a.z);
	}

	void RotateEulerZXY(Vertex a)
	{
		RotateZ(a.z);
		RotateX(a.x);
		RotateY(a.y);
	}

	void RotateEulerZYX(Vertex a)
	{
		RotateZ(a.z);
		RotateY(a.y);
		RotateX(a.x);
	}

	void RotateX(float a)
	{
		float c = cosf(a);
		float s = sinf(a);

		Matrix m;
		m.mat[1][1] = c;
		m.mat[1][2] = -s;
		m.mat[2][1] = s;
		m.mat[2][2] = c;

		*this = m * *this;
	}

	void RotateY(float a)
	{
		float c = cosf(a);
		float s = sinf(a);

		Matrix m;
		m.mat[0][0] = c;
		m.mat[0][2] = s;
		m.mat[2][0] = -s;
		m.mat[2][2] = c;

		*this = m * *this;
	}

	void RotateZ(float a)
	{
		float c = cosf(a);
		float s = sinf(a);

		Matrix m;
		m.mat[0][0] = c;
		m.mat[0][1] = -s;
		m.mat[1][0] = s;
		m.mat[1][1] = c;

		*this = m * *this;
	}

	void Rotate(Vertex axis, float angle)
	{
		float c = cosf(angle);
		float s = sinf(angle);
		float d = 1.0f - c;

		Matrix m;
		m.mat[0][0] = c + axis.x * axis.x * d;
		m.mat[1][0] = axis.y * axis.x * d + axis.z * s;
		m.mat[2][0] = axis.z * axis.x * d - axis.y * s;
		m.mat[3][0] = 0.0f;

		m.mat[0][1] = axis.x * axis.y * d - axis.z * s;
		m.mat[1][1] = c + axis.y * axis.y * d;
		m.mat[2][1] = axis.z * axis.y * d + axis.x * s;
		m.mat[3][1] = 0.0f;

		m.mat[0][2] = axis.x * axis.z * d + axis.y * s;
		m.mat[1][2] = axis.y * axis.z * d - axis.x * s;
		m.mat[2][2] = c + axis.z * axis.z * d;
		m.mat[3][2] = 0.0f;

		m.mat[0][3] = 0.0f;
		m.mat[1][3] = 0.0f;
		m.mat[2][3] = 0.0f;
		m.mat[3][3] = 1.0f;
		*this = m * *this;
	}

	void ToEulerZYX(Vertex& angle)
	{
		float l = sqrtf(mat[0][0] * mat[0][0] + mat[1][0] * mat[1][0]);
		if (l > 1e-6)
		{
			angle.x = atan2f(mat[2][1], mat[2][2]);
			angle.y = atan2f(-mat[2][0], l);
			angle.z = atan2f(mat[1][0], mat[0][0]);
		}
		else
		{
			angle.x = atan2f(-mat[1][2], mat[1][1]);
			angle.y = atan2f(-mat[2][0], l);
			angle.z = 0.0f;
		}
	}

	void AlignBackUp(Vertex back, Vertex up)
	{
		back.Normalize();
		up.Normalize();
		Vertex right = up.Cross(back);

		Matrix m;
		m.mat[0][0] = right.x;
		m.mat[1][0] = right.y;
		m.mat[2][0] = right.z;
		m.mat[3][0] = 0.0f;

		m.mat[0][1] = up.x;
		m.mat[1][1] = up.y;
		m.mat[2][1] = up.z;
		m.mat[3][1] = 0.0f;

		m.mat[0][2] = back.x;
		m.mat[1][2] = back.y;
		m.mat[2][2] = back.z;
		m.mat[3][2] = 0.0f;

		m.mat[0][3] = 0.0f;
		m.mat[1][3] = 0.0f;
		m.mat[2][3] = 0.0f;
		m.mat[3][3] = 1.0f;
		*this = m * *this;
	}

	void AlignBackRight(Vertex back, Vertex right)
	{
		back.Normalize();
		right.Normalize();
		Vertex up = back.Cross(right);

		Matrix m;
		m.mat[0][0] = right.x;
		m.mat[1][0] = right.y;
		m.mat[2][0] = right.z;
		m.mat[3][0] = 0.0f;

		m.mat[0][1] = up.x;
		m.mat[1][1] = up.y;
		m.mat[2][1] = up.z;
		m.mat[3][1] = 0.0f;

		m.mat[0][2] = back.x;
		m.mat[1][2] = back.y;
		m.mat[2][2] = back.z;
		m.mat[3][2] = 0.0f;

		m.mat[0][3] = 0.0f;
		m.mat[1][3] = 0.0f;
		m.mat[2][3] = 0.0f;
		m.mat[3][3] = 1.0f;

		*this = m * *this;
	}

	Matrix Inverse3x3()
	{
		float d = mat[0][0] * (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2])
			- mat[0][1] * (mat[1][0] * mat[2][2] - mat[1][2] * mat[2][0])
			+ mat[0][2] * (mat[1][0] * mat[2][1] - mat[1][1] * mat[2][0]);

		Matrix m;
		if (d == 0.0f) { m.Zero(); return m; }
		d = 1.0f / d;

		m.mat[0][0] = (mat[1][1] * mat[2][2] - mat[2][1] * mat[1][2]) * d;
		m.mat[0][1] = -(mat[0][1] * mat[2][2] - mat[2][1] * mat[0][2]) * d;
		m.mat[0][2] = (mat[0][1] * mat[1][2] - mat[1][1] * mat[0][2]) * d;

		m.mat[1][0] = -(mat[1][0] * mat[2][2] - mat[2][0] * mat[1][2]) * d;
		m.mat[1][1] = (mat[0][0] * mat[2][2] - mat[2][0] * mat[0][2]) * d;
		m.mat[1][2] = -(mat[0][0] * mat[1][2] - mat[1][0] * mat[0][2]) * d;

		m.mat[2][0] = (mat[1][0] * mat[2][1] - mat[2][0] * mat[1][1]) * d;
		m.mat[2][1] = -(mat[0][0] * mat[2][1] - mat[2][0] * mat[0][1]) * d;
		m.mat[2][2] = (mat[0][0] * mat[1][1] - mat[1][0] * mat[0][1]) * d;

		return m;
	}

	Vertex operator*(Vertex v) const
	{
		float x = v.x * mat[0][0] + v.y * mat[0][1] + v.z * mat[0][2] + mat[0][3];
		float y = v.x * mat[1][0] + v.y * mat[1][1] + v.z * mat[1][2] + mat[1][3];
		float z = v.x * mat[2][0] + v.y * mat[2][1] + v.z * mat[2][2] + mat[2][3];
		return Vertex(x, y, z);
	}

	Matrix operator+(Matrix m) const
	{
		Matrix r;
		for (int i = 0; i < 4; i++)
		{
			r.mat[i][0] = mat[i][0] + m.mat[i][0];
			r.mat[i][1] = mat[i][1] + m.mat[i][1];
			r.mat[i][2] = mat[i][2] + m.mat[i][2];
			r.mat[i][3] = mat[i][3] + m.mat[i][3];
		}
		return r;
	}

	Matrix operator*(Matrix m) const
	{
		Matrix r;
		for (int i = 0; i < 4; i++)
		{
			r.mat[i][0] = mat[i][0] * m.mat[0][0] + mat[i][1] * m.mat[1][0] + mat[i][2] * m.mat[2][0] + mat[i][3] * m.mat[3][0];
			r.mat[i][1] = mat[i][0] * m.mat[0][1] + mat[i][1] * m.mat[1][1] + mat[i][2] * m.mat[2][1] + mat[i][3] * m.mat[3][1];
			r.mat[i][2] = mat[i][0] * m.mat[0][2] + mat[i][1] * m.mat[1][2] + mat[i][2] * m.mat[2][2] + mat[i][3] * m.mat[3][2];
			r.mat[i][3] = mat[i][0] * m.mat[0][3] + mat[i][1] * m.mat[1][3] + mat[i][2] * m.mat[2][3] + mat[i][3] * m.mat[3][3];
		}
		return r;
	}
};

namespace Primitives
{
	const Vertex up = Vertex(0.0f, 1.0f, 0.0f);
	const Vertex down = Vertex(0.0f, -1.0f, 0.0f);
	const Vertex front = Vertex(0.0f, 0.0f, -1.0f);
	const Vertex back = Vertex(0.0f, 0.0f, 1.0f);
	const Vertex left = Vertex(-1.0f, 0.0f, 0.0f);
	const Vertex right = Vertex(1.0f, 0.0f, 0.0f);
	const Vertex zero = Vertex(0.0f, 0.0f, 0.0f);
}
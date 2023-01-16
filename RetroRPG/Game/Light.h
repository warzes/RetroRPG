#pragma once

// Simple light models (point / directional / ambient)

#include "Core.h"
#include "Color.h"
#include "Geometry.h"
#include "Mesh.h"

enum LIGHT_TYPES
{
	LIGHT_POINT = 0,   // A point light (intensity decrease with distance)
	LIGHT_DIRECTIONAL, // A directional light (light intensity depends of incidence)
	LIGHT_AMBIENT,     // An ambient light (light influence all triangles)
};

// Contain and manage a light object
class Light
{
public:
	Light();
	Light(LIGHT_TYPES type, Color color);

	static void Black(Mesh* mesh);
	// Shine a mesh with the light
	void Shine(Mesh* mesh) const;

	static void BlendColors(Color color1, Color color2, float factor, Color& result);

	LIGHT_TYPES type;     // Model of the light
	Axis axis{};          // Axis (source and direction) of the light
	Color color;          // Diffuse color of the light
	float rolloff = 1.0f; // Roll off factor (point model) of the light

private:
	void shinePoint(Mesh* mesh) const;
	void shineDirectional(Mesh* mesh) const;
	void shineAmbient(Mesh* mesh) const;
};
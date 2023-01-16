#include "Light.h"
#include <string.h>
//-----------------------------------------------------------------------------
Light::Light() 
	: type(LIGHT_AMBIENT)
	, color(Color::RGB(0xCCCCCC))
{
}
//-----------------------------------------------------------------------------
Light::Light(LIGHT_TYPES type, Color color) 
	: type(type)
	, color(color)
{
}
//-----------------------------------------------------------------------------
void Light::Black(Mesh* mesh)
{
	if( !mesh->shades ) mesh->AllocateShades();
	memset(mesh->shades, 0, sizeof(Color) * mesh->noTriangles);
}
//-----------------------------------------------------------------------------
void Light::Shine(Mesh* mesh) const
{
	if( !mesh->normals ) mesh->ComputeNormals();
	if( !mesh->shades ) mesh->AllocateShades();

	switch( type )
	{
	case LIGHT_POINT:
		shinePoint(mesh);
		break;
	case LIGHT_DIRECTIONAL:
		shineDirectional(mesh);
		break;
	case LIGHT_AMBIENT:
		shineAmbient(mesh);
		break;
	}
}
//-----------------------------------------------------------------------------
void Light::BlendColors(Color color1, Color color2, float factor, Color& result)
{
	uint32_t f = (uint32_t)(factor * 65536.0f);
	result.r = cmbound(result.r + ((color1.r * color2.r * f) >> 24), 0, 255);
	result.g = cmbound(result.g + ((color1.g * color2.g * f) >> 24), 0, 255);
	result.b = cmbound(result.b + ((color1.b * color2.b * f) >> 24), 0, 255);
	result.a = 0;
}
//-----------------------------------------------------------------------------
inline void Light::shinePoint(Mesh* mesh) const
{
	Vertex o = axis.origin - mesh->pos;
	for( int j = 0; j < mesh->noTriangles; j++ )
	{
		const float third = 1.0f / 3.0f;
		Vertex v1 = mesh->vertexes[mesh->vertexesList[j * 3]];
		Vertex v2 = mesh->vertexes[mesh->vertexesList[j * 3 + 1]];
		Vertex v3 = mesh->vertexes[mesh->vertexesList[j * 3 + 2]];
		Vertex m = (v1 + v2 + v3) * third - o;
		Vertex n = mesh->normals[j];

		float p = n.Dot(m);
		if( p > 0.0f ) continue;
		float d = m.Dot(m);
		float e = cmmax((1.0f - d * rolloff), 0.0f);
		BlendColors(mesh->colors[j], color, e, mesh->shades[j]);
	}
}
//-----------------------------------------------------------------------------
inline void Light::shineDirectional(Mesh* mesh) const
{
	Matrix iv = mesh->view.Inverse3x3();
	Vertex rp = iv * axis.axis;
	rp.Normalize();

	for( int j = 0; j < mesh->noTriangles; j++ )
	{
		float p = -rp.Dot(mesh->normals[j]);
		if( p > 0.0f ) BlendColors(Color::RGB(0xFFFFFF), color, p, mesh->shades[j]);
	}
}
//-----------------------------------------------------------------------------
inline void Light::shineAmbient(Mesh* mesh) const
{
	for( int j = 0; j < mesh->noTriangles; j++ )
		BlendColors(mesh->colors[j], color, 1.0f, mesh->shades[j]);
}
//-----------------------------------------------------------------------------
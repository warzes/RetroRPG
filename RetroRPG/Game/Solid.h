#pragma once

#include "Core.h"
#include "Geometry.h"
#include <stdint.h>

// Physical constants
#define VISCOSITY_OIL		1500fe-3
#define VISCOSITY_WATER		1.0fe-3
#define VISCOSITY_AIR		0.02fe-3

// System coordinate use for calculation
enum SOLID_SYSTEM
{
	SOLID_SYSTEM_ABSOLUTE = 0, // Absolute coordinates (world coordinates)
	SOLID_SYSTEM_RELATIVE = 1, // Relative coordinates (object coordinates)
};

// Basic solid point physics object
class Solid
{
public:
	Solid();

	void Init(const Vertex& pos);
	void Stop();

	void Configure(float mass, float radius);

	void glueMoveAxis(Vertex axis);
	void glueRotateAxis(Vertex axis);

	void collideSolid(Vertex contact, Solid& collider);
	void collideHard(Vertex contact, Vertex normal, float rigidity);

	void applyFriction(Vertex normal, float friction);

	void applyForce(Vertex force);
	void applyForceRight(float force);
	void applyForceUp(float force);
	void applyForceBack(float force);

	void applyTorque(Vertex torque);
	void applyTorqueRight(float torque);
	void applyTorqueUp(float torque);
	void applyTorqueBack(float torque);

	void applyForcePoint(Vertex point, Vertex force);
	void applyTorquePoint(Vertex point, Vertex torque);

	void update(float dt);

private:
	void applyGravity();
	void applyGround();

	void computeVectors();
	void computeFriction(float dt);

public:
	float adhesion;
	float rigidity;
	float gravity;

	float groundLevel;
	float groundFriction;

	SOLID_SYSTEM fluidSystem;
	Vertex fluid1Move;
	Vertex fluid1Rotate;
	Vertex fluid2Move;
	Vertex fluid2Rotate;

	Vertex glueMove;
	Vertex glueRotate;

	Vertex pos;
	Vertex posSpeed;
	Vertex posAccel;

	Vertex rot;
	Vertex rotSpeed;
	Vertex rotAccel;

	Vertex rightVector;
	Vertex upVector;
	Vertex backVector;

private:
	float lastdt;
	Vertex frictionAccel;

	float mass, imass;
	float inertia, iinertia;
	float radius;
};
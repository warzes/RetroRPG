#include "Solid.h"
#include <math.h>
#include <float.h>

Solid::Solid() :
	adhesion(0.5f), rigidity(1.0f),
	gravity(9.8f),
	groundLevel(0.0f), groundFriction(0.0f),
	fluidSystem(SOLID_SYSTEM_RELATIVE),
	fluid1Move(), fluid1Rotate(), fluid2Move(), fluid2Rotate(),
	glueMove(1.0f, 1.0f, 1.0f),
	glueRotate(1.0f, 1.0f, 1.0f),
	pos(), posSpeed(), posAccel(),
	rot(), rotSpeed(), rotAccel(),
	rightVector(), upVector(), backVector()
{
	Init(Vertex());
	Configure(1.0f, 1.0f);
}

void Solid::Init(const Vertex& pos)
{
	Stop();
	this->pos = pos;
	this->rot = Vertex();
	rightVector = Primitives::right;
	upVector = Primitives::up;
	backVector = Primitives::back;
}

void Solid::Stop()
{
	posSpeed = Vertex();
	posAccel = Vertex();
	rotSpeed = Vertex();
	rotAccel = Vertex();
}

void Solid::Configure(float mass, float radius)
{
	if (mass <= 0.0f) return;
	if (radius <= 0.0f) return;

	this->radius = radius;
	this->mass = mass;

	inertia = 2.0f / 5.0f * mass * radius * radius;
	imass = 1.0f / mass;
	iinertia = 1.0f / inertia;
}

void Solid::applyFriction(Vertex normal, float coefficient)
{
	Vertex sn = posSpeed;
	sn.Normalize();
	frictionAccel -= sn * (sn.Dot(Primitives::down) * mass * coefficient);
}

void Solid::applyForce(Vertex force)
{
	posAccel += force * imass;
}

void Solid::applyForceRight(float force)
{
	posAccel += rightVector * (force * imass);
}

void Solid::applyForceUp(float force)
{
	posAccel += upVector * (force * imass);
}

void Solid::applyForceBack(float force)
{
	posAccel += backVector * (force * imass);
}

void Solid::applyForcePoint(Vertex point, Vertex force)
{
	posAccel += force * imass;
	Vertex cv = point - pos;
	rotAccel += cv.Cross(force) * (r2d * iinertia);
}

void Solid::applyTorque(Vertex torque)
{
	rotAccel += torque * iinertia;
}

void Solid::applyTorqueRight(float torque)
{
	rotAccel += rightVector * (torque * iinertia);
}

void Solid::applyTorqueUp(float torque)
{
	rotAccel += upVector * (torque * iinertia);
}

void Solid::applyTorqueBack(float torque)
{
	rotAccel += backVector * (torque * iinertia);
}

void Solid::applyTorquePoint(Vertex point, Vertex torque)
{
	rotAccel += torque * iinertia;
	Vertex cv = point - pos;
	posAccel += cv.Cross(torque) * (d2r * iinertia);
}

void Solid::computeVectors()
{
	Matrix m;
	m.RotateEulerYXZ(rot * d2r);
	rightVector = m * Primitives::right;
	upVector = m * Primitives::up;
	backVector = m * Primitives::back;
}

void Solid::computeFriction(float dt)
{
	Vertex posSpeedRelative;
	if (fluidSystem == SOLID_SYSTEM_RELATIVE) {
		posSpeedRelative.x = posSpeed.Dot(rightVector);
		posSpeedRelative.y = posSpeed.Dot(upVector);
		posSpeedRelative.z = posSpeed.Dot(backVector);
	}
	else posSpeedRelative = posSpeed;
	Vertex psn = posSpeedRelative;
	psn.Normalize();

	Vertex rotSpeedRelative;
	if (fluidSystem == SOLID_SYSTEM_RELATIVE) {
		rotSpeedRelative.x = rotSpeed.Dot(rightVector);
		rotSpeedRelative.y = rotSpeed.Dot(upVector);
		rotSpeedRelative.z = rotSpeed.Dot(backVector);
	}
	else rotSpeedRelative = rotSpeed;

	//printf("Speed %f %f %f\n", posSpeedRelative.x, posSpeedRelative.y, posSpeedRelative.z);

	Vertex rsn = rotSpeedRelative;
	rsn.Normalize();

	// Movement
	Vertex moveAccel = (psn * fluid1Move + posSpeedRelative * fluid2Move + frictionAccel) * imass * dt;
	if (cmabs(moveAccel.x) > cmabs(posSpeedRelative.x)) moveAccel.x = posSpeedRelative.x;
	if (cmabs(moveAccel.y) > cmabs(posSpeedRelative.y)) moveAccel.y = posSpeedRelative.y;
	if (cmabs(moveAccel.z) > cmabs(posSpeedRelative.z)) moveAccel.z = posSpeedRelative.z;
	if (fluidSystem == SOLID_SYSTEM_ABSOLUTE) posSpeed -= moveAccel;
	else posSpeed -= rightVector * moveAccel.x + upVector * moveAccel.y + backVector * moveAccel.z;

	// Rotation
	Vertex rotateAccel = (rsn * fluid1Rotate + rotSpeedRelative * fluid2Rotate) * iinertia * dt;
	if (cmabs(rotateAccel.x) > cmabs(rotSpeedRelative.x)) rotateAccel.x = rotSpeedRelative.x;
	if (cmabs(rotateAccel.y) > cmabs(rotSpeedRelative.y)) rotateAccel.y = rotSpeedRelative.y;
	if (cmabs(rotateAccel.z) > cmabs(rotSpeedRelative.z)) rotateAccel.z = rotSpeedRelative.z;
	if (fluidSystem == SOLID_SYSTEM_ABSOLUTE) rotSpeed -= rotateAccel;
	else rotSpeed -= rightVector * rotateAccel.x + upVector * rotateAccel.y + backVector * rotateAccel.z;
}

void Solid::applyGravity()
{
	posAccel.y -= gravity;
}

void Solid::applyGround()
{
	float rl = groundLevel + radius;
	if (pos.y > rl) return;
	pos.y = rl;
	posSpeed.y = 0.0f;
	posAccel.y = 0.0f;
}

void Solid::update(float dt)
{
	computeVectors();

	applyGravity();
	applyGround();

	pos += posSpeed * dt;
	rot += rotSpeed * dt;
	posSpeed += posAccel * dt;
	posSpeed *= glueMove;
	rotSpeed += rotAccel * dt;
	rotSpeed *= glueRotate;

	computeFriction(dt);

	posAccel = Vertex();
	rotAccel = Vertex();
	frictionAccel = Vertex();

	lastdt = dt;
}

void Solid::collideSolid(Vertex contact, Solid& collider)
{
	Vertex contactForce = posSpeed * mass;
	posAccel -= contactForce * (imass * rigidity);
	collider.posAccel += contactForce * (collider.imass * collider.rigidity);

	Vertex cv = contact - collider.pos;
	if (cv.Dot(contactForce) < 0.0f)
	{
		float mc = r2d * collider.iinertia * collider.adhesion;
		collider.rotAccel += cv.Cross(contactForce) * mc;
	}
}

void Solid::collideHard(Vertex contact, Vertex normal, float rigidity)
{
	Vertex cv = contact - pos;
	Vertex pointForce = (posSpeed * mass + cv.Cross(rotSpeed) * d2r * inertia) / lastdt;
	Vertex normalForce = normal * normal.Dot(pointForce);

	normalForce = normalForce * (1.0f + rigidity);

	/** Apply force on self */
		// rotSpeed -= cv.cross(normalForce) * (r2d * iinertia * lastdt);
	posSpeed -= normalForce * (imass * lastdt);
}
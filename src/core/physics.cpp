//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "core/physics.h"
#include "core/constants.h"
#include "core/ray.h"

float PhysicsModule::radialAcceleration(float r, float L) {
	return L * L / (r * r * r) * (1.f - 1.5f * kRs / r);
}

void PhysicsModule::verletStep(SimulationModule::Ray& ray, float dlambda) {
	// KICK: half-step radial velocity update with the acceleration at the current r
	const float v_r_half = ray.v_r + 0.5f * dlambda * ray.a_r;

	// DRIFT: full-step position update.
	// φ is advanced with dφ/dλ = L/r² at the midpoint radius (second-order accurate).
	const float r_old = ray.r;
	ray.r += dlambda * v_r_half;
	const float r_mid = 0.5f * (r_old + ray.r);
	ray.phi += dlambda * ray.L / (r_mid * r_mid);

	// KICK: complete the radial velocity update with the acceleration at the new r
	ray.a_r = radialAcceleration(ray.r, ray.L);
	ray.v_r = v_r_half + 0.5f * dlambda * ray.a_r;

	// Angular velocity follows from conservation of L (used by updateDeflection)
	ray.v_phi = ray.L / (ray.r * ray.r);
}

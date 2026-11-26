//
// Created by Vladimir Glushkov on 24.11.2026.
//

#pragma once

namespace SimulationModule {
	struct Ray;
}

namespace PhysicsModule {
	// Radial acceleration of a light ray in Schwarzschild spacetime:
	//   d²r/dλ² = L²/r³ · (1 - 3rs/(2r))
	// Derived from the null geodesic equation using conservation of L
	// (dφ/dλ = L/r²), so it depends on position only. That is what makes
	// Velocity Verlet symplectic (and second-order) for this system.
	// It is the negative gradient of the effective potential L²/(2r²)·(1 - rs/r),
	// whose maximum at r = 1.5 rs is the photon sphere.
	float radialAcceleration(float r, float L);


	// Symplectic Velocity Verlet (Stormer-Verlet) integration step, "kick-drift-kick".
	// Symplectic because the radial acceleration depends on r only (see radialAcceleration):
	// energy error stays bounded and L is conserved exactly by construction.
	// One acceleration evaluation per step: the value at the end of a step
	// is cached in ray.a_r and reused at the start of the next one.
	// https://www.algorithm-archive.org/contents/verlet_integration/verlet_integration.html
	void verletStep(SimulationModule::Ray& ray, float dlambda);
} // namespace PhysicsModule

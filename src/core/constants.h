//
// Created by Vladimir Glushkov on 24.11.2026.
//

#pragma once

namespace PhysicsModule {
	// Physical constants are measured in units of rs
	constexpr float kRs = 1.0f;
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
	constexpr float kPi = M_PI;
} // namespace PhysicsModule

namespace SimulationModule {
	// Half-extents of the simulated world, in units of rs.
	// The world spans [-kWorldHalfWidth, kWorldHalfWidth] x [-kWorldHalfHeight, kWorldHalfHeight].
	constexpr float kWorldHalfWidth = 8.f;
	constexpr float kWorldHalfHeight = 6.f;

	// Rays farther than this from the black hole are considered escaped
	constexpr float kMaxDistance = 2.0f * kWorldHalfWidth;

	// Affine parameter step of the integrator per simulation update
	constexpr float kDLambda = 0.05f;
} // namespace SimulationModule

namespace RenderModule {
	constexpr int kDefaultWidth = 800;
	constexpr int kDefaultHeight = 600;
} // namespace RenderModule

namespace ViewModule {
	constexpr int kNumStars = 200;
} // namespace ViewModule

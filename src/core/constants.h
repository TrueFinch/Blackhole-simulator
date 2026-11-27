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

	constexpr float kEventHorizonRadius = kRs;
	constexpr float kPhotonSphereRadius = 1.5f * kRs;
} // namespace PhysicsModule

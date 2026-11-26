//
// Created by Vladimir Glushkov on 24.11.2026.
//

#include "core/simulation.h"

#include <cmath>

namespace {
	// Frame at which each scenario starts
	constexpr int kOrbitingStartFrame = 0;
	constexpr int kPointSourceStartFrame = 180;
	constexpr int kParallelStartFrame = 360;

	// Point source in the top-left corner, far left for dramatic lensing
	constexpr float kSourceX = -0.95f * SimulationModule::kWorldHalfWidth; // 95% left
	constexpr float kSourceY = 0.85f * SimulationModule::kWorldHalfHeight; // 85% up
} // namespace

SimulationModule::Simulation::Simulation() {
	spawnOrbitingRay();
	spawnPointSourceRays();
	spawnParallelRays();
}

void SimulationModule::Simulation::update() {
	for (auto& ray: m_rays) {
		ray.integrate(kDLambda, kMaxDistance, m_frame);
	}
	++m_frame;
}

glm::vec2 SimulationModule::Simulation::pointSourcePosition() {
	return {kSourceX, kSourceY};
}

bool SimulationModule::Simulation::isPointSourceActive() const {
	return m_frame >= kPointSourceStartFrame;
}

// SCENARIO 1: Special orbiting ray near photon sphere (1 ray)
void SimulationModule::Simulation::spawnOrbitingRay() {
	constexpr float orbitStartX = -0.9f * kWorldHalfWidth; // 90% left
	// Start height just above the capture threshold. The critical impact parameter is
	// b = L/E = 3√3/2 rs ≈ 2.598 rs; since the ray starts at a finite distance, E < 1 and
	// b is slightly larger than the start height. The threshold is extremely sensitive
	// (and depends on dλ and float precision): with dλ = 0.05 the ray is captured
	// for y ≤ 2.5786 and escapes after ~2.4 turns for y = 2.5787.
	constexpr float orbitStartY = 2.5787f * PhysicsModule::kRs;
	constexpr float orbitVx = 1.f; // Horizontal toward black hole
	constexpr float orbitVy = 0.f;
	m_rays.emplace_back(orbitStartX, orbitStartY, orbitVx, orbitVy, RayScenario::ORBITING, kOrbitingStartFrame);
}

// SCENARIO 2: Point source from top-left corner (25 rays)
void SimulationModule::Simulation::spawnPointSourceRays() {
	constexpr int numPointRays = 25;
	// Calculate direction from source to black hole
	constexpr float toBlackHoleX = 0.f - kSourceX;
	constexpr float toBlackHoleY = 0.f - kSourceY;
	const float baseAngle{std::atan2(toBlackHoleY, toBlackHoleX)};

	for (auto i = 0; i < numPointRays; ++i) {
		// Spread rays in 60-degree cone around direction to black hole
		constexpr float coneSpread = PhysicsModule::kPi / 3.f; // 60 degrees total (π/3 radians)
		const float angleOffset = -coneSpread / 2.f + coneSpread * static_cast<float>(i) / static_cast<float>(
									numPointRays - 1);
		const float angle = baseAngle + angleOffset;

		// Calculate velocity components (all rays travel at speed c)
		const float vx = std::cos(angle);
		const float vy = std::sin(angle);

		m_rays.emplace_back(kSourceX, kSourceY, vx, vy, RayScenario::POINT_SOURCE, kPointSourceStartFrame);
	}
}

// SCENARIO 3: Parallel rays from left (70 rays)
void SimulationModule::Simulation::spawnParallelRays() {
	constexpr int numParallelRays = 70;
	// Left edge of the world. Must stay below kMaxDistance, otherwise
	// the rays count as escaped right away and are never integrated.
	constexpr float parallelStartX = -kWorldHalfWidth;
	static_assert(
		parallelStartX * parallelStartX + kWorldHalfHeight * kWorldHalfHeight < kMaxDistance * kMaxDistance,
		"Parallel rays must start inside kMaxDistance");
	constexpr float parallelVx = 1.f; // Horizontal, rightward
	constexpr float parallelVy = 0.f; // No vertical component

	for (auto i = 0; i < numParallelRays; ++i) {
		// Calculate interpolation parameter [0, 1]
		const float t{static_cast<float>(i) / static_cast<float>(numParallelRays - 1)};

		// Map to full world height: -kWorldHalfHeight to +kWorldHalfHeight
		const float startY = -kWorldHalfHeight + t * 2.f * kWorldHalfHeight;

		m_rays.emplace_back(parallelStartX, startY, parallelVx, parallelVy, RayScenario::PARALLEL,
							kParallelStartFrame);
	}
}

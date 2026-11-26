//
// Created by Vladimir Glushkov on 24.11.2026.
//

#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "core/constants.h"
#include "core/ray.h"

namespace SimulationModule {
	// Core of the application: owns the simulated objects and advances them in time.
	// Knows nothing about windows, OpenGL or how its objects are drawn;
	// the view layer reads its state through the const accessors.
	class Simulation {
	public:
		static constexpr float kEventHorizonRadius = PhysicsModule::kRs;
		static constexpr float kPhotonSphereRadius = 1.5f * PhysicsModule::kRs;

		Simulation();

		// Advance the simulation by one step (one integration step of every active ray)
		void update();

		[[nodiscard]] const std::vector<Ray>& rays() const { return m_rays; }

		// Number of updates done so far
		[[nodiscard]] int frame() const { return m_frame; }

		[[nodiscard]] bool isActive(const Ray& ray) const { return ray.isActive(m_frame); }

		// Point source of the POINT_SOURCE scenario
		[[nodiscard]] static glm::vec2 pointSourcePosition();

		[[nodiscard]] bool isPointSourceActive() const;

	private:
		std::vector<Ray> m_rays;
		int m_frame = 0;

		void spawnOrbitingRay();

		void spawnPointSourceRays();

		void spawnParallelRays();
	};
} // namespace SimulationModule

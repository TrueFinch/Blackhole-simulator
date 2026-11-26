//
// Created by Vladimir on 25.11.2026.
//

#pragma once
#include <vector>
#include <glm/vec2.hpp>

namespace SimulationModule {
	enum class RayScenario {
		PARALLEL, // Parallel rays from left side (distant starlight)
		POINT_SOURCE, // Rays emanating from a single point (gravitational lensing)
		ORBITING // Special orbiting ray (photon sphere)
	};

	struct Ray {
		// polar coordinates
		float r; // Radial distance from black hole
		float phi; // Angular position (radians)
		// velocity in polar coordinates
		float v_r; // Radial velocity (dr/dλ)
		float v_phi; // Angular velocity (dφ/dλ), always L/r²
		float a_r; // Radial acceleration (d²r/dλ²) at the current r

		float L; // Conserved angular momentum
		float E; // Conserved energy parameter

		float initialVelocityAngle; // Direction ray was heading at start
		float prevVelocityAngle;
		float totalTurn = 0.f;
		float deflection = 0.f; // How much direction has changed

		RayScenario scenario;
		int startFrame;

		std::vector<glm::vec2> trail;

		Ray(float x, float y, float vx, float vy, RayScenario scenario, int startFrame = 0);

		[[nodiscard]] bool isCaptured() const;

		[[nodiscard]] bool hasEscaped(float maxDistance) const;

		void recordPosition();

		void integrate(float dlambda, float maxDistance, int currentFrame);

		void updateDeflection();

		[[nodiscard]] bool isActive(int currentFrame) const;
	};
}

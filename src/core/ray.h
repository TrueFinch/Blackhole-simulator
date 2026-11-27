//
// Created by Vladimir on 25.11.2026.
//

#pragma once
#include <deque>
#include <glm/vec2.hpp>

namespace SimulationModule {
	// One point of a ray trail: where the ray was at which simulation step.
	struct TrailPoint {
		glm::vec2 pos;
		int step;
		float deflection; // of the ray at that step
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

		int emitterId; // EmitterDesc::id of the emitter that fired this ray

		std::deque<TrailPoint> trail; // oldest first

		// `dir` need not be normalized; birthStep is the step of the first trail point.
		Ray(glm::vec2 pos, glm::vec2 dir, int emitterId, int birthStep);

		[[nodiscard]] bool isCaptured() const;

		[[nodiscard]] bool hasEscaped(float maxDistance) const;

		// Captured or escaped: integrate does nothing any more.
		[[nodiscard]] bool isFinished(float maxDistance) const;

		// One integration step that adds a trail point for `step`; does nothing once finished.
		void integrate(float dlambda, float maxDistance, int step);

		// Drops trail points more than fadeSteps older than currentStep.
		void trimTrail(int currentStep, int fadeSteps);

		void updateDeflection();
	};
}

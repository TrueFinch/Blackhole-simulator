//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "core/ray.h"

#include <cmath>

#include "core/constants.h"
#include "core/physics.h"


SimulationModule::Ray::Ray(float x, float y, float vx, float vy, RayScenario scenario,
							int startFrame): scenario(scenario)
											, startFrame(startFrame) {
	using namespace PhysicsModule;
	// Convert Cartesian position to polar coordinates
	r = std::sqrt(x * x + y * y);
	phi = std::atan2(y, x);

	// Convert Cartesian velocity to polar coordinates
	// Using transformation:
	// v_r = vx*cos(φ) + vy*sin(φ)
	// v_φ = (-vx*sin(φ) + vy*cos(φ)) / r
	const auto cos_phi = std::cos(phi);
	const auto sin_phi = std::sin(phi);
	v_r = vx * cos_phi + vy * sin_phi;
	v_phi = (-vx * sin_phi + vy * cos_phi) / r;

	// Angular momentum L = r² * v_φ
	L = r * r * v_phi;

	// Metric coefficient (f = 1 - rs/r)
	const auto f = 1.f - kRs / r;
	// For null geodesics (light rays), the normalization condition gives:
	// dt/dλ = √[(v_r)²/f² + (r²·v_φ)²/f]
	const auto dt_dlambda = std::sqrt((v_r * v_r) / (f * f) + (r * r * v_phi * v_phi) / f);
	// Energy E = f * dt/dλ
	E = f * dt_dlambda;

	// Acceleration at the start position; verletStep keeps it up to date
	a_r = radialAcceleration(r, L);

	trail.emplace_back(x, y);

	// Store initial velocity direction (NOT position angle!)
	prevVelocityAngle = initialVelocityAngle = std::atan2(vy, vx);
}


bool SimulationModule::Ray::isCaptured() const {
	using namespace PhysicsModule;
	return r <= kRs * 1.01f;
}

bool SimulationModule::Ray::hasEscaped(float maxDistance) const {
	return r > maxDistance;
}

void SimulationModule::Ray::recordPosition() {
	const auto x = r * std::cos(phi);
	const auto y = r * std::sin(phi);
	trail.emplace_back(x, y);
}

void SimulationModule::Ray::integrate(float dlambda, float maxDistance, int currentFrame) {
	if (!isActive(currentFrame) || isCaptured() || hasEscaped(maxDistance)) {
		return;
	}
	PhysicsModule::verletStep(*this, dlambda);
	recordPosition();
	updateDeflection();
}

void SimulationModule::Ray::updateDeflection() {
	// Convert current polar velocity back to Cartesian
	const auto vx_current = v_r * std::cos(phi) - r * v_phi * std::sin(phi);
	const auto vy_current = v_r * std::sin(phi) + r * v_phi * std::cos(phi);

	// Calculate current velocity direction
	const auto currentVelocityAngle = std::atan2(vy_current, vx_current);
	// Change since the previous step, wrapped into (-π, π]
	auto delta = currentVelocityAngle - prevVelocityAngle;
	if (delta > M_PI) {
		delta -= 2.f * M_PI;
	}
	if (delta < -M_PI) {
		delta += 2.f * M_PI;
	}

	// Measure how much direction has changed
	totalTurn += delta; // signed, accumulates full loops
	prevVelocityAngle = currentVelocityAngle;
	deflection = std::abs(totalTurn);
}

bool SimulationModule::Ray::isActive(int currentFrame) const {
	return currentFrame >= startFrame;
}

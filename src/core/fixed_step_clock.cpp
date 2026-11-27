//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "core/fixed_step_clock.h"

#include <algorithm>
#include <cmath>

SimulationModule::FixedStepClock::FixedStepClock(int stepsPerSecond, int maxStepsPerFrame)
	: m_stepsPerSecond(stepsPerSecond), m_maxStepsPerFrame(maxStepsPerFrame) {}

int SimulationModule::FixedStepClock::advance(double frameSeconds) {
	// Absorbs the rounding error of sums like 1/120 + 1/120
	constexpr double kEpsilon = 1e-9;
	m_pendingSteps += std::max(frameSeconds, 0.0) * m_stepsPerSecond;
	const double whole = std::floor(m_pendingSteps + kEpsilon);
	m_pendingSteps = std::max(m_pendingSteps - whole, 0.0);
	return static_cast<int>(std::min(whole, static_cast<double>(m_maxStepsPerFrame)));
}

void SimulationModule::FixedStepClock::setStepsPerSecond(int stepsPerSecond) {
	m_stepsPerSecond = stepsPerSecond;
}

void SimulationModule::FixedStepClock::reset() {
	m_pendingSteps = 0.0;
}

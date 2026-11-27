//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

namespace SimulationModule {
	// Turns variable frame durations into a whole number of fixed simulation steps.
	class FixedStepClock {
	public:
		explicit FixedStepClock(int stepsPerSecond, int maxStepsPerFrame = 10);

		// How many steps to make for a frame that took frameSeconds; the fraction is carried over,
		// steps beyond maxStepsPerFrame are dropped
		[[nodiscard]] int advance(double frameSeconds);

		void setStepsPerSecond(int stepsPerSecond);

		// Forgets the carried-over fraction
		void reset();

	private:
		int m_stepsPerSecond;
		int m_maxStepsPerFrame;
		double m_pendingSteps = 0.0;
	};
} // namespace SimulationModule

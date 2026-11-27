//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_test_macros.hpp>

#include "core/fixed_step_clock.h"

using SimulationModule::FixedStepClock;

TEST_CASE("Fixed step clock") {
	SECTION("Accumulates fractional frames") {
		FixedStepClock clock(60);
		CHECK(clock.advance(1.0 / 120.0) == 0);
		CHECK(clock.advance(1.0 / 120.0) == 1);
	}

	SECTION("Caps steps per frame and drops the rest") {
		FixedStepClock clock(60);
		CHECK(clock.advance(10.0) == 10);
		CHECK(clock.advance(0.0) == 0);
	}

	SECTION("Negative frame time counts as zero") {
		FixedStepClock clock(60);
		CHECK(clock.advance(-1.0) == 0);
		CHECK(clock.advance(1.0 / 60.0) == 1);
	}

	SECTION("Follows a changed rate; reset drops the fraction") {
		FixedStepClock clock(60);
		CHECK(clock.advance(1.0 / 120.0) == 0);
		clock.reset();
		CHECK(clock.advance(1.0 / 120.0) == 0);
		clock.setStepsPerSecond(120);
		CHECK(clock.advance(1.0 / 60.0) == 2);
	}
}

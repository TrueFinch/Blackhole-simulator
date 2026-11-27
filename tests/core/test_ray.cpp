//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/ray.h"

using Catch::Approx;
using SimulationModule::Ray;

namespace {
	constexpr float kStep = 0.05f;
	constexpr float kMaxDistance = 50.f;

	Ray flyingRay(int steps) {
		Ray ray({-10.f, 5.f}, {1.f, 0.f}, 3, 0);
		for (int i = 0; i < steps; ++i) {
			ray.integrate(kStep, kMaxDistance, i);
		}
		return ray;
	}
}

TEST_CASE("Ray trail") {
	SECTION("Trail points record step and deflection") {
		const Ray fresh({-10.f, 5.f}, {1.f, 0.f}, 3, 7);
		CHECK(fresh.emitterId == 3);
		REQUIRE(fresh.trail.size() == 1);
		CHECK(fresh.trail.front().step == 7);

		const Ray ray = flyingRay(10);
		REQUIRE(ray.trail.size() == 11);
		CHECK(ray.trail.back().step == 9);
		CHECK(ray.trail.back().deflection == Approx(ray.deflection));
	}

	SECTION("Trimming drops points older than the fade") {
		Ray ray = flyingRay(10);
		ray.trimTrail(9, 3);
		REQUIRE_FALSE(ray.trail.empty());
		CHECK(ray.trail.front().step == 6);
		CHECK(ray.trail.back().step == 9);
	}

	SECTION("Trimming far in the future empties the trail") {
		Ray ray = flyingRay(10);
		ray.trimTrail(1000, 3);
		CHECK(ray.trail.empty());
	}

	SECTION("Finished ray does not grow its trail") {
		Ray ray({-10.f, 2.5f}, {1.f, 0.f}, 0, 0);
		int step = 0;
		for (; step < 2000 && !ray.isFinished(kMaxDistance); ++step) {
			ray.integrate(kStep, kMaxDistance, step);
		}
		REQUIRE(ray.isFinished(kMaxDistance));
		const auto size = ray.trail.size();
		for (int i = 0; i < 10; ++i) {
			ray.integrate(kStep, kMaxDistance, step++);
		}
		CHECK(ray.trail.size() == size);
	}
}

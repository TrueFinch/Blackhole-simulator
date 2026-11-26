//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <cmath>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/constants.h"
#include "core/ray.h"

using Catch::Approx;
using SimulationModule::Ray;
using SimulationModule::RayScenario;

namespace {
	constexpr float kStep = 0.05f;
}

TEST_CASE("Physics") {
	SECTION("Ray constructor converts to polar coordinates")
	{
		const Ray ray(3.f, 4.f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		CHECK(ray.r == Approx(5.f));
		CHECK(ray.phi == Approx(std::atan2(4.f, 3.f)));
		// L = x*vy - y*vx
		CHECK(ray.L == Approx(-4.f));
	}

	SECTION("Angular momentum is conserved exactly by verlet")
	{
		Ray ray(-10.f, 5.f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		const float L = ray.L;
		for (int frame = 0; frame < 2000; ++frame) {
			ray.integrate(kStep, 50.f, frame);
			CHECK(ray.r * ray.r * ray.v_phi == Approx(L).epsilon(1e-5));
		}
	}

	SECTION("Null condition stays bounded")
	{
		Ray ray(-10.f, 5.f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		const auto e2 = [](const Ray& r) {
			return r.v_r * r.v_r + (1.f - PhysicsModule::kRs / r.r) * r.L * r.L / (r.r * r.r);
		};
		const float e2_0 = e2(ray);
		for (int frame = 0; frame < 2000; ++frame) {
			ray.integrate(kStep, 50.f, frame);
			CHECK(std::abs(e2(ray) - e2_0) / e2_0 < 1e-3f);
		}
	}

	SECTION("Ray below critical impact parameter is captured")
	{
		Ray ray(-10.f, 2.5f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		for (int frame = 0; frame < 2000 && !ray.isCaptured(); ++frame) {
			ray.integrate(kStep, 50.f, frame);
		}
		CHECK(ray.isCaptured());
	}

	SECTION("Ray above critical impact parameter escapes")
	{
		Ray ray(-10.f, 2.7f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		for (int frame = 0; frame < 4000 && !ray.isCaptured() && !ray.hasEscaped(50.f); ++frame) {
			ray.integrate(kStep, 50.f, frame);
		}
		CHECK(ray.hasEscaped(50.f));
		CHECK_FALSE(ray.isCaptured());
	}

	SECTION("Weak-field deflection is close to 2 rs / b")
	{
		// The start radius (~400.5) already exceeds 400, so integrate with a larger
		// limit and measure the deflection once the ray is beyond the start distance.
		constexpr float kLimit = 1000.f;
		Ray ray(-400.f, 20.f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		for (int frame = 0; frame < 100000 && !ray.hasEscaped(kLimit); ++frame) {
			ray.integrate(kStep, kLimit, frame);
		}
		REQUIRE(ray.hasEscaped(kLimit));
		// Second-order weak-field prediction: 2 rs/b + 15 pi rs^2 / (16 b^2) ~= 0.10736 for b = 20.
		const float rs = PhysicsModule::kRs;
		const float b = 20.f;
		const float expected = 2.f * rs / b + 15.f * PhysicsModule::kPi * rs * rs / (16.f * b * b);
		CHECK(ray.deflection == Approx(expected).epsilon(0.01));
	}

	SECTION("Ray does not move after capture")
	{
		Ray ray(-10.f, 2.5f, 1.f, 0.f, RayScenario::PARALLEL, 0);
		int frame = 0;
		for (; frame < 2000 && !ray.isCaptured(); ++frame) {
			ray.integrate(kStep, 50.f, frame);
		}
		REQUIRE(ray.isCaptured());
		const auto size = ray.trail.size();
		for (int i = 0; i < 10; ++i) {
			ray.integrate(kStep, 50.f, frame++);
		}
		CHECK(ray.trail.size() == size);
	}
}

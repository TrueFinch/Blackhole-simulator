//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

#include "core/emitter.h"

using Catch::Approx;
using namespace SimulationModule;
using namespace SceneModule;

namespace {
	float angleOf(const RaySpawn& r) {
		return std::atan2(r.dir.y, r.dir.x);
	}

	EmitterDesc emitter(EmitterShape shape, int rays) {
		EmitterDesc e;
		e.shape = shape;
		e.rays = rays;
		return e;
	}
}

TEST_CASE("Emitter volleys") {
	SECTION("Cone spreads rays edge to edge") {
		const auto e = emitter(ConeShape{.pos = {1.f, 2.f}, .angle = 0.f, .spread = 1.f}, 3);
		const auto rays = volley(e);
		REQUIRE(rays.size() == 3);
		CHECK(angleOf(rays[0]) == Approx(-0.5f));
		CHECK(angleOf(rays[1]) == Approx(0.f).margin(1e-6));
		CHECK(angleOf(rays[2]) == Approx(0.5f));
		for (const auto& r: rays) {
			CHECK(r.pos == glm::vec2(1.f, 2.f));
			CHECK(glm::length(r.dir) == Approx(1.f));
		}
	}

	SECTION("Single-ray cone shoots at the sector center") {
		const auto rays = volley(emitter(ConeShape{.angle = 1.f, .spread = 2.f}, 1));
		REQUIRE(rays.size() == 1);
		CHECK(angleOf(rays[0]) == Approx(1.f));
	}

	SECTION("Cone aims at the target point, ignoring angle") {
		const auto rays = volley(emitter(ConeShape{.pos = {-1.f, -1.f}, .aim = glm::vec2{0.f, 0.f}, .angle = 3.f}, 1));
		REQUIRE(rays.size() == 1);
		CHECK(rays[0].dir.x == Approx(0.7071f).margin(1e-4));
		CHECK(rays[0].dir.y == Approx(0.7071f).margin(1e-4));
	}

	SECTION("Cone with aim at its own position falls back to angle") {
		const auto rays = volley(emitter(ConeShape{.pos = {2.f, 3.f}, .aim = glm::vec2{2.f, 3.f}, .angle = 1.f}, 1));
		REQUIRE(rays.size() == 1);
		CHECK(angleOf(rays[0]) == Approx(1.f));
	}

	SECTION("Line distributes positions evenly, ends included") {
		const auto rays = volley(emitter(LineShape{.from = {0.f, -1.f}, .to = {0.f, 1.f}, .angle = 0.f}, 3));
		REQUIRE(rays.size() == 3);
		CHECK(rays[0].pos.y == Approx(-1.f));
		CHECK(rays[1].pos.y == Approx(0.f).margin(1e-6));
		CHECK(rays[2].pos.y == Approx(1.f));
		for (const auto& r: rays) {
			CHECK(r.dir.x == Approx(1.f));
			CHECK(r.dir.y == Approx(0.f).margin(1e-6));
		}
	}

	SECTION("Single-ray line starts at the midpoint") {
		const auto rays = volley(emitter(LineShape{.from = {0.f, -1.f}, .to = {2.f, 3.f}}, 1));
		REQUIRE(rays.size() == 1);
		CHECK(rays[0].pos.x == Approx(1.f));
		CHECK(rays[0].pos.y == Approx(1.f));
	}

	SECTION("No rays, no spawns") {
		CHECK(volley(emitter(ConeShape{}, 0)).empty());
		CHECK(volley(emitter(LineShape{}, -5)).empty());
	}
}

TEST_CASE("Emitter schedule in steps") {
	EmitterDesc e;
	e.start = Seconds{3.f};
	CHECK(startStep(e, 60) == 180);
	CHECK_FALSE(periodSteps(e, 60).has_value());

	e.cooldown = Seconds{1.5f};
	CHECK(periodSteps(e, 60) == 90);

	e.cooldown = Seconds{0.001f}; // shorter than a step: still at least one
	CHECK(periodSteps(e, 60) == 1);

	CHECK(toSteps(Seconds{-2.f}, 60) == 0);
	CHECK(toSteps(Seconds{0.026f}, 60) == 2); // 1.56 steps rounds to 2
}

TEST_CASE("Fade steps of a style") {
	Style style;
	CHECK_FALSE(fadeSteps(style, 60).has_value());
	style.fade = Seconds{0.5f};
	CHECK(fadeSteps(style, 60) == 30);
	// Shorter than a step still fades
	style.fade = Seconds{0.001f};
	CHECK(fadeSteps(style, 60) == 1);
}

//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "core/constants.h"
#include "core/scene_description.h"
#include "view/trail_color.h"

using Catch::Approx;
using namespace ViewModule;

namespace {
	SceneModule::Style greenToYellow() {
		SceneModule::Style style;
		style.trailFrom = {0.f, 1.f, 0.f};
		style.trailTo = {1.f, 1.f, 0.f};
		return style;
	}
} // namespace

TEST_CASE("Point color follows its own deflection") {
	const auto style = greenToYellow();
	const auto at = [&](float deflection) { return trailPointColor(style, deflection, 0, std::nullopt, 0, 2); };

	const auto start = at(0.f);
	CHECK(start.r == Approx(0.f));
	CHECK(start.g == Approx(1.f));

	CHECK(at(PhysicsModule::kPi / 2.f).r == Approx(0.5f));
	CHECK(at(PhysicsModule::kPi).r == Approx(1.f));

	// Past pi the color stays at trailTo
	const auto beyond = at(2.f * PhysicsModule::kPi);
	CHECK(beyond.r == Approx(1.f));
	CHECK(beyond.g == Approx(1.f));
	CHECK(beyond.b == Approx(0.f));
}

TEST_CASE("Alpha fades with age") {
	const SceneModule::Style style;
	const auto alphaAt = [&](int age) { return trailPointColor(style, 0.f, age, 60, 0, 1).a; };

	CHECK(alphaAt(0) == Approx(1.f));
	CHECK(alphaAt(30) == Approx(0.5f));
	CHECK(alphaAt(60) == Approx(0.f));
	CHECK(alphaAt(90) == Approx(0.f));
	CHECK(alphaAt(-5) == Approx(1.f));
}

TEST_CASE("Without fade the trail keeps the gradient from old to new") {
	const SceneModule::Style style;
	const auto alphaAt = [&](std::size_t index) {
		return trailPointColor(style, 0.f, 1000, std::nullopt, index, 5).a;
	};

	CHECK(alphaAt(0) == Approx(0.2f));
	CHECK(alphaAt(2) == Approx(0.6f));
	CHECK(alphaAt(4) == Approx(1.f));
	// A single point is fully opaque
	CHECK(trailPointColor(style, 0.f, 0, std::nullopt, 0, 1).a == Approx(1.f));
}

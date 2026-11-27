//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <utility>

#include "view/view_fit.h"

using Catch::Approx;
using namespace ViewModule;

TEST_CASE("Wide window shows extra horizontal space") {
	const auto fit = fitView(1600, 600, {8.f, 6.f});
	CHECK(fit.halfExtents.x == Approx(16.f));
	CHECK(fit.halfExtents.y == Approx(6.f));
}

TEST_CASE("Tall window shows extra vertical space") {
	const auto fit = fitView(800, 1200, {8.f, 6.f});
	CHECK(fit.halfExtents.x == Approx(8.f));
	CHECK(fit.halfExtents.y == Approx(12.f));
}

TEST_CASE("Minimized window does not produce NaN") {
	for (const auto [w, h]: {std::pair{0, 0}, std::pair{0, 600}}) {
		const auto fit = fitView(w, h, {8.f, 6.f});
		CHECK(std::isfinite(fit.halfExtents.x));
		CHECK(std::isfinite(fit.halfExtents.y));
		CHECK(fit.halfExtents.x == 8.f);
		CHECK(fit.halfExtents.y == 6.f);
	}
}

TEST_CASE("worldToScreen maps corners") {
	const auto fit = fitView(800, 600, {8.f, 6.f});
	const auto topLeft = worldToScreen({-8.f, 6.f}, fit, 800, 600);
	const auto bottomRight = worldToScreen({8.f, -6.f}, fit, 800, 600);
	const auto center = worldToScreen({0.f, 0.f}, fit, 800, 600);
	CHECK(topLeft.x == Approx(0.f));
	CHECK(topLeft.y == Approx(0.f));
	CHECK(bottomRight.x == Approx(800.f));
	CHECK(bottomRight.y == Approx(600.f));
	CHECK(center.x == Approx(400.f));
	CHECK(center.y == Approx(300.f));
}

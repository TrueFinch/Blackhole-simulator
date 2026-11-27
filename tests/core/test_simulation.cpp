//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <algorithm>

#include <catch2/catch_test_macros.hpp>

#include "core/simulation.h"

using namespace SceneModule;
using SimulationModule::Simulation;

namespace {
	// Far enough that no ray escapes during a test
	constexpr float kLargeDistance = 1000.f;

	Settings testSettings(int maxRays = 5000) {
		Settings settings;
		settings.simulation.stepsPerSecond = 60;
		settings.simulation.maxRays = maxRays;
		settings.world.maxDistance = kLargeDistance;
		return settings;
	}

	// Cone at (10, 0) aimed away from the black hole: its rays fly freely and stay alive
	EmitterDesc outwardEmitter(int id, int rays, float start, std::optional<float> cooldown,
	                           std::string style = kDefaultStyleName) {
		EmitterDesc e;
		e.id = id;
		e.shape = ConeShape{.pos = {10.f, 0.f}, .angle = 0.f, .spread = 0.1f};
		e.rays = rays;
		e.start = Seconds{start};
		if (cooldown) {
			e.cooldown = Seconds{*cooldown};
		}
		e.style = std::move(style);
		return e;
	}

	// Cone at (-2, 0) aimed straight at the black hole: its rays are captured within a few dozen steps
	EmitterDesc capturedEmitter(int id, int rays, float start, std::optional<float> cooldown,
	                            std::string style = kDefaultStyleName) {
		EmitterDesc e = outwardEmitter(id, rays, start, cooldown, std::move(style));
		e.shape = ConeShape{.pos = {-2.f, 0.f}, .angle = 0.f, .spread = 0.01f};
		return e;
	}

	SceneDescription sceneOf(std::vector<EmitterDesc> emitters, StyleMap localStyles = {}) {
		SceneDescription scene;
		scene.emitters = std::move(emitters);
		scene.localStyles = std::move(localStyles);
		return scene;
	}

	void run(Simulation& simulation, int steps) {
		for (int i = 0; i < steps; ++i) {
			simulation.step();
		}
	}

	std::size_t raysOf(const Simulation& simulation, int emitterId) {
		return std::ranges::count_if(simulation.rays(), [&](const auto& ray) { return ray.emitterId == emitterId; });
	}

	const StyleMap kNoStyles;
	const Style kFade05{.fade = Seconds{0.5f}}; // 30 steps at 60 steps per second
}

TEST_CASE("Simulation schedule") {
	SECTION("One-shot emitter fires once at its start step") {
		// start 0.5 s = step 30
		Simulation simulation(sceneOf({outwardEmitter(1, 3, 0.5f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 30); // steps 0..29
		REQUIRE(simulation.rays().empty());
		simulation.step(); // step 30
		REQUIRE(simulation.rays().size() == 3);
		for (const auto& ray: simulation.rays()) {
			CHECK(ray.trail.front().step == 30);
		}
		run(simulation, 200);
		CHECK(simulation.rays().size() == 3);
	}

	SECTION("Emitter with cooldown fires periodically") {
		// Volleys at steps 0, 60, 120
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, 1.f)}), kNoStyles, testSettings());
		run(simulation, 120);
		REQUIRE(simulation.rays().size() == 2);
		simulation.step();
		CHECK(simulation.rays().size() == 3);
		CHECK(simulation.stepIndex() == 121);
		CHECK(simulation.time() == Seconds{121.f / 60.f});
	}

	SECTION("Tiny cooldown fires at most once per step") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, 0.0001f)}), kNoStyles, testSettings());
		run(simulation, 10);
		CHECK(simulation.rays().size() == 10);
	}
}

TEST_CASE("Simulation trail fade") {
	SECTION("Fade 0.5 s: a finished ray goes away 30 steps after its last trail point") {
		Simulation simulation(sceneOf({capturedEmitter(1, 1, 0.f, std::nullopt, "fading")}, {{"fading", kFade05}}),
		                      kNoStyles, testSettings());
		simulation.step();
		REQUIRE(simulation.rays().size() == 1);
		while (!simulation.rays().front().isFinished(kLargeDistance) && simulation.stepIndex() < 1000) {
			simulation.step();
		}
		REQUIRE(simulation.rays().front().isCaptured());
		const int lastPointStep = simulation.rays().front().trail.back().step;
		while (simulation.stepIndex() < lastPointStep + 31) {
			simulation.step();
		}
		REQUIRE(simulation.rays().size() == 1); // its last point is exactly 30 steps old
		simulation.step();
		CHECK(simulation.rays().empty());
	}

	SECTION("No fade: a finished ray stays") {
		Simulation simulation(sceneOf({capturedEmitter(1, 1, 0.f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 500);
		REQUIRE(simulation.rays().size() == 1);
		CHECK(simulation.rays().front().isCaptured());
	}

	SECTION("A fade shorter than a step still removes a finished ray") {
		Simulation simulation(sceneOf({capturedEmitter(1, 1, 0.f, std::nullopt, "tiny")},
		                              {{"tiny", Style{.fade = Seconds{0.001f}}}}),
		                      kNoStyles, testSettings());
		run(simulation, 500);
		CHECK(simulation.rays().empty());
	}
}

TEST_CASE("Simulation ray limit") {
	SECTION("Evicts the oldest finished rays first") {
		// 5 rays every 30 steps, each captured long before the next volley; room for two volleys
		Simulation simulation(sceneOf({capturedEmitter(1, 5, 0.f, 0.5f)}), kNoStyles, testSettings(10));
		run(simulation, 300); // volleys at 0, 30, ..., 270
		REQUIRE(simulation.rays().size() == 10);
		simulation.step(); // volley at 300 evicts the volley of step 240
		REQUIRE(simulation.rays().size() == 10);
		CHECK_FALSE(simulation.rayLimitHit());
		for (const auto& ray: simulation.rays()) {
			const int birth = ray.trail.front().step;
			CHECK((birth == 270 || birth == 300));
		}
	}

	SECTION("A volley larger than the limit is skipped, not retried") {
		Simulation simulation(sceneOf({outwardEmitter(1, 5, 0.f, std::nullopt)}), kNoStyles, testSettings(3));
		simulation.step();
		CHECK(simulation.rays().empty());
		CHECK(simulation.rayLimitHit());
		simulation.step();
		CHECK(simulation.rays().empty());
		CHECK_FALSE(simulation.rayLimitHit());
	}

	SECTION("Flying rays are never evicted") {
		// Volleys of 3 at steps 0, 1, 2...; the second does not fit and the schedule moves on
		Simulation simulation(sceneOf({outwardEmitter(1, 3, 0.f, 1.f / 60.f)}), kNoStyles, testSettings(5));
		simulation.step();
		REQUIRE(simulation.rays().size() == 3);
		CHECK_FALSE(simulation.rayLimitHit());
		simulation.step();
		CHECK(simulation.rays().size() == 3);
		CHECK(simulation.rayLimitHit());
		for (const auto& ray: simulation.rays()) {
			CHECK(ray.trail.front().step == 0);
		}
	}
}

TEST_CASE("Simulation time control") {
	SECTION("clearRays keeps the step and the schedule of a cooldown emitter") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, 1.f)}), kNoStyles, testSettings());
		run(simulation, 61); // volleys at 0 and 60
		REQUIRE(simulation.rays().size() == 2);
		simulation.clearRays();
		CHECK(simulation.rays().empty());
		CHECK(simulation.stepIndex() == 61);
		run(simulation, 59); // steps 61..119
		CHECK(simulation.rays().empty());
		simulation.step(); // step 120
		REQUIRE(simulation.rays().size() == 1);
		CHECK(simulation.rays().front().trail.front().step == 120);
	}

	SECTION("clearRays: a one-shot emitter does not fire again") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 5);
		simulation.clearRays();
		run(simulation, 200);
		CHECK(simulation.rays().empty());
	}

	SECTION("restart resets the step and fires again") {
		Simulation simulation(sceneOf({outwardEmitter(1, 2, 0.f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 10);
		REQUIRE(simulation.rays().size() == 2);
		simulation.restart();
		CHECK(simulation.stepIndex() == 0);
		CHECK(simulation.rays().empty());
		simulation.step();
		REQUIRE(simulation.rays().size() == 2);
		CHECK(simulation.rays().front().trail.front().step == 0);
	}

	SECTION("seek is deterministic") {
		// Fading trails, captured rays removed and periodic volleys all depend on the step index
		const SceneDescription scene = sceneOf({capturedEmitter(1, 4, 0.f, 0.25f, "fading"),
		                                        outwardEmitter(2, 3, 0.5f, 1.f)},
		                                       {{"fading", kFade05}});
		Simulation a(scene, kNoStyles, testSettings());
		run(a, 150);
		Simulation b(scene, kNoStyles, testSettings());
		run(b, 37);
		b.seek(150);
		REQUIRE(b.stepIndex() == 150);
		REQUIRE_FALSE(a.rays().empty());
		REQUIRE(a.rays().size() == b.rays().size());
		for (std::size_t i = 0; i < a.rays().size(); ++i) {
			CHECK(a.rays()[i].emitterId == b.rays()[i].emitterId);
			CHECK(a.rays()[i].trail.size() == b.rays()[i].trail.size());
			CHECK(a.rays()[i].trail.front().pos == b.rays()[i].trail.front().pos);
			CHECK(a.rays()[i].trail.back().pos == b.rays()[i].trail.back().pos);
		}
	}

	SECTION("seek to a negative step clamps to zero") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 10);
		simulation.seek(-5);
		CHECK(simulation.stepIndex() == 0);
		CHECK(simulation.rays().empty());
		simulation.step(); // the schedule starts over
		CHECK(simulation.rays().size() == 1);
	}
}

TEST_CASE("Simulation live scene edits") {
	SECTION("An edited cooldown applies after the next volley") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, 1.f)}), kNoStyles, testSettings());
		simulation.step(); // volley at 0, next at 60
		simulation.setScene(sceneOf({outwardEmitter(1, 1, 0.f, 0.5f)}), kNoStyles);
		run(simulation, 59); // steps 1..59
		REQUIRE(simulation.rays().size() == 1);
		simulation.step(); // step 60: the old cooldown still decided this volley; next at 90
		REQUIRE(simulation.rays().size() == 2);
		run(simulation, 29); // steps 61..89
		REQUIRE(simulation.rays().size() == 2);
		simulation.step(); // step 90
		CHECK(simulation.rays().size() == 3);
	}

	SECTION("An edited one-shot does not refire") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 5);
		simulation.setScene(sceneOf({outwardEmitter(1, 3, 0.f, std::nullopt)}), kNoStyles);
		CHECK(simulation.rays().size() == 1); // rays already in flight stay
		run(simulation, 100);
		CHECK(simulation.rays().size() == 1);
		simulation.restart(); // the edit applies to the volley after a restart
		simulation.step();
		CHECK(simulation.rays().size() == 3);
	}

	SECTION("A new emitter with a past start fires now") {
		Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, std::nullopt)}), kNoStyles, testSettings());
		run(simulation, 100);
		simulation.setScene(sceneOf({outwardEmitter(1, 1, 0.f, std::nullopt),
		                             outwardEmitter(2, 2, 0.5f, std::nullopt), // start step 30 has passed
		                             outwardEmitter(3, 4, 2.f, std::nullopt)}), // start step 120 is ahead
		                    kNoStyles);
		simulation.step(); // step 100
		CHECK(raysOf(simulation, 2) == 2);
		CHECK(raysOf(simulation, 3) == 0);
		run(simulation, 19); // steps 101..119
		CHECK(raysOf(simulation, 3) == 0);
		simulation.step(); // step 120
		CHECK(raysOf(simulation, 3) == 4);
		CHECK(raysOf(simulation, 1) == 1);
	}

	SECTION("A removed emitter loses its rays") {
		Simulation simulation(sceneOf({outwardEmitter(1, 2, 0.f, std::nullopt), outwardEmitter(2, 3, 0.f, 1.f)}),
		                      kNoStyles, testSettings());
		simulation.step();
		REQUIRE(simulation.rays().size() == 5);
		simulation.setScene(sceneOf({outwardEmitter(1, 2, 0.f, std::nullopt)}), kNoStyles);
		CHECK(simulation.rays().size() == 2);
		CHECK(raysOf(simulation, 2) == 0);
		run(simulation, 200); // the removed periodic emitter no longer fires
		CHECK(simulation.rays().size() == 2);
		CHECK(simulation.scene().emitters.size() == 1);
	}
}

TEST_CASE("Simulation styles") {
	const StyleMap shared{{"shared", kFade05}};
	const Style local{.width = 3.f};
	const Simulation simulation(sceneOf({outwardEmitter(1, 1, 0.f, std::nullopt, "nope"),
	                                     outwardEmitter(2, 1, 0.f, std::nullopt, "shared"),
	                                     outwardEmitter(3, 1, 0.f, std::nullopt, "local")},
	                                    {{"local", local}}),
	                            shared, testSettings());
	CHECK(simulation.styleOf(1) == Style{});
	CHECK(simulation.styleOf(2) == kFade05);
	CHECK(simulation.styleOf(3) == local);
	CHECK(simulation.styleOf(42) == Style{}); // unknown emitter
}

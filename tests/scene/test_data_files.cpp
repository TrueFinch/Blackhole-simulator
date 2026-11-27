//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

#include "config/data_paths.h"
#include "core/simulation.h"
#include "scene/scene_io.h"

using namespace SceneModule;
using Catch::Approx;

namespace {

	const std::filesystem::path kDataDir = BLACKHOLE_DATA_DIR;

}

TEST_CASE("Shipped data files") {
	SECTION("All shipped scenarios load without errors or warnings") {
		std::vector<std::string> styleWarnings;
		auto styles = loadStyles(kDataDir / "config" / "styles.toml", &styleWarnings);
		REQUIRE(styles.has_value());
		CHECK(styleWarnings.empty());

		int count = 0;
		for (const auto &entry: std::filesystem::directory_iterator(kDataDir / "scenarios")) {
			if (entry.path().extension() != ".toml") continue;
			++count;
			INFO(entry.path().string());
			auto loaded = loadScene(entry.path(), Settings{}.world);
			REQUIRE(loaded.has_value());
			CHECK(loaded->warnings.empty());
			CHECK_FALSE(loaded->scene.name.empty());
			CHECK_FALSE(loaded->scene.emitters.empty());

			std::vector<std::string> resolveWarnings;
			for (const EmitterDesc &e: loaded->scene.emitters)
				static_cast<void>(resolveStyle(e.style, loaded->scene.localStyles, *styles, &resolveWarnings));
			CHECK(resolveWarnings.empty());
		}
		CHECK(count == 5);
	}

	SECTION("Classic scenario matches tutorial") {
		auto loaded = loadScene(kDataDir / "scenarios" / "classic.toml", Settings{}.world);
		REQUIRE(loaded.has_value());
		const auto &emitters = loaded->scene.emitters;
		REQUIRE(emitters.size() == 3);
		CHECK(emitters[0].rays == 1);
		CHECK(emitters[1].rays == 25);
		CHECK(emitters[2].rays == 70);
		CHECK(emitters[0].start.count() == Approx(0.f));
		CHECK(emitters[1].start.count() == Approx(3.f));
		CHECK(emitters[2].start.count() == Approx(6.f));
	}

	SECTION("Shipped point_source scenario fires its cone every 90 steps") {
		auto styles = loadStyles(DataPaths{kDataDir}.styles());
		REQUIRE(styles.has_value());
		const Settings settings;
		auto loaded = loadScene(DataPaths{kDataDir}.scenario("point_source"), settings.world);
		REQUIRE(loaded.has_value());
		REQUIRE(loaded->scene.emitters.size() == 1);
		// The default style (no fade), so that no ray disappears while counting
		loaded->scene.emitters.front().style = kDefaultStyleName;
		const auto perVolley = static_cast<std::size_t>(loaded->scene.emitters.front().rays);
		SimulationModule::Simulation simulation(loaded->scene, *styles, settings);
		auto run = [&](int steps) {
			for (int i = 0; i < steps; ++i) {
				simulation.step();
			}
		};
		run(90); // steps 0..89
		CHECK(simulation.rays().size() == perVolley);
		run(1); // step 90
		CHECK(simulation.rays().size() == 2 * perVolley);
		run(89); // steps 91..179
		CHECK(simulation.rays().size() == 2 * perVolley);
		run(1); // step 180
		CHECK(simulation.rays().size() == 3 * perVolley);
	}
}

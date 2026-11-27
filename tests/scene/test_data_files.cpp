//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <vector>

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
}

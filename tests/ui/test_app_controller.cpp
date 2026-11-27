//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "config/data_paths.h"
#include "ui/app_controller.h"

using SceneModule::DataPaths;
using UiModule::AppController;

namespace {
	namespace fs = std::filesystem;

	void write(const fs::path& path, const std::string& text) {
		fs::create_directories(path.parent_path());
		std::ofstream(path) << text;
	}

	// One ray at step 0, aimed at the black hole
	const char* kCone = R"(name = "x"
[[emitter]]
type = "cone"
pos = [-7, 1]
angle = 0
spread = 0
rays = 1
start = "0s"
style = "default"
)";

	// Fresh data folder per test under <temp>/bh_tests (as the scene_io tests use); removed on destruction
	struct TempData {
		fs::path root;
		DataPaths paths;

		TempData() {
			static std::atomic<int> counter{0};
			root = fs::temp_directory_path() / "bh_tests" /
					("app_controller_" + std::to_string(counter++) + "_" +
					std::to_string(fs::file_time_type::clock::now().time_since_epoch().count()));
			paths = DataPaths{root};
			write(paths.settings(), "[view]\ndefault_scenario = \"a\"\n");
			write(paths.styles(), "");
			write(paths.scenario("a"), kCone);
			write(paths.scenario("b"), kCone);
			write(paths.scenario("broken"), "name = = oops\n[[emitter\n");
		}

		TempData(const TempData&) = delete;

		TempData& operator=(const TempData&) = delete;

		~TempData() {
			std::error_code ec;
			fs::remove_all(root, ec);
		}
	};
}

TEST_CASE("App Controller") {
	SECTION("Scenario list comes from the folder") {
		TempData data;
		AppController controller(data.paths);
		CHECK(controller.scenarioNames() == std::vector<std::string>{"a", "b", "broken"});
		CHECK(controller.currentScenario() == "a");
		CHECK_FALSE(controller.lastError());
	}

	SECTION("Broken scenario keeps the current one") {
		TempData data;
		AppController controller(data.paths);
		REQUIRE_FALSE(controller.selectScenario("broken"));
		CHECK(controller.currentScenario() == "a");
		REQUIRE(controller.lastError());
		CHECK(controller.lastError()->find("broken.toml") != std::string::npos);
		CHECK(controller.selectScenario("b"));
		CHECK(controller.currentScenario() == "b");
		CHECK_FALSE(controller.lastError());
	}

	SECTION("Bad default scenario leaves an empty scene without throwing") {
		TempData data;
		write(data.paths.settings(), "[view]\ndefault_scenario = \"missing\"\n");
		AppController controller(data.paths);
		CHECK(controller.currentScenario().empty());
		CHECK(controller.lastError());
		CHECK(controller.simulation().scene().emitters.empty());
	}

	SECTION("Reload without a loaded scenario reports an error") {
		TempData data;
		write(data.paths.settings(), "[view]\ndefault_scenario = \"missing\"\n");
		AppController controller(data.paths);
		REQUIRE(controller.currentScenario().empty());
		CHECK_FALSE(controller.reloadFromDisk());
		REQUIRE(controller.lastError());
		CHECK(*controller.lastError() == "Нет загруженного сценария");
	}

	SECTION("Broken settings and styles fall back to defaults and stay reported") {
		TempData data;
		write(data.paths.settings(), "[view\n");
		write(data.paths.styles(), "[x\n");
		write(data.paths.scenario("classic"), kCone); // the default scenario
		AppController controller(data.paths);
		CHECK(controller.currentScenario() == "classic");
		REQUIRE(controller.configErrors().size() == 2);
		CHECK(controller.configErrors()[0].find("settings.toml") != std::string::npos);
		CHECK(controller.configErrors()[1].find("styles.toml") != std::string::npos);
		CHECK_FALSE(controller.lastError());
	}

	SECTION("Config load errors stay visible") {
		TempData data;
		write(data.paths.settings(), "[view\n");
		write(data.paths.scenario("classic"), kCone); // the default scenario
		AppController controller(data.paths);
		REQUIRE(controller.configErrors().size() == 1);
		CHECK(controller.configErrors()[0].find("settings.toml") != std::string::npos);
		CHECK(controller.settings() == SceneModule::Settings{});
		REQUIRE(controller.selectScenario("a"));
		CHECK_FALSE(controller.lastError());
		CHECK(controller.configErrors().size() == 1);
	}

	SECTION("Pause stops simulation steps") {
		TempData data;
		AppController controller(data.paths);
		controller.update(0.5);
		const int running = controller.simulation().stepIndex();
		CHECK(running > 0);
		controller.togglePause();
		REQUIRE(controller.paused());
		controller.update(1.0);
		CHECK(controller.simulation().stepIndex() == running);
		controller.togglePause();
		controller.update(0.05);
		CHECK(controller.simulation().stepIndex() > running);
	}

	SECTION("Step forward and back") {
		TempData data;
		AppController controller(data.paths);
		for (int i = 0; i < 3; ++i) controller.stepForward();
		CHECK(controller.simulation().stepIndex() == 3);
		CHECK(controller.paused());
		controller.stepBack();
		CHECK(controller.simulation().stepIndex() == 2);
		controller.stepBack();
		controller.stepBack();
		controller.stepBack();
		CHECK(controller.simulation().stepIndex() == 0);
	}

	SECTION("Seek pauses at the target step") {
		TempData data;
		AppController controller(data.paths);
		controller.seek(25);
		CHECK(controller.paused());
		CHECK(controller.simulation().stepIndex() == 25);
		controller.seek(-4);
		CHECK(controller.simulation().stepIndex() == 0);
	}

	SECTION("Clear keeps the step index") {
		TempData data;
		AppController controller(data.paths);
		controller.seek(10);
		REQUIRE_FALSE(controller.simulation().rays().empty());
		controller.clearRays();
		CHECK(controller.simulation().rays().empty());
		CHECK(controller.simulation().stepIndex() == 10);
	}

	SECTION("Restart resets the step index") {
		TempData data;
		AppController controller(data.paths);
		controller.seek(10);
		controller.restart();
		CHECK(controller.simulation().stepIndex() == 0);
	}

	SECTION("Reload re-reads the current scenario file") {
		TempData data;
		AppController controller(data.paths);
		controller.seek(5);
		write(data.paths.scenario("a"), "name = \"changed\"\n");
		REQUIRE(controller.reloadFromDisk());
		CHECK(controller.simulation().scene().name == "changed");
		CHECK(controller.simulation().stepIndex() == 0);
		write(data.paths.scenario("a"), "name = = bad\n");
		CHECK_FALSE(controller.reloadFromDisk());
		CHECK(controller.simulation().scene().name == "changed");
		CHECK(controller.lastError());
	}
}

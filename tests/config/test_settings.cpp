//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "config/settings.h"
#include "toml/toml_util.h"

using namespace SceneModule;

namespace {
	std::filesystem::path tempPath(const std::string& name) {
		auto dir = std::filesystem::temp_directory_path() / "bh_tests";
		std::filesystem::create_directories(dir);
		return dir / name;
	}

	std::filesystem::path writeTemp(const std::string& content) {
		static std::atomic counter{0};
		auto path = tempPath("settings_" + std::to_string(counter++) + ".toml");
		std::ofstream(path) << content;
		return path;
	}
}

TEST_CASE("Settings schema") {
	SECTION("Missing settings keys use defaults") {
		auto result = loadSettings(writeTemp("[world]\nhalf_width = 10.0"));
		REQUIRE(result.has_value());
		CHECK(result->world.halfWidth == Catch::Approx(10.f));
		Settings expected;
		expected.world.halfWidth = 10.f;
		CHECK(*result == expected);
	}

	SECTION("Wrong type in settings is an error with line") {
		auto result = loadSettings(writeTemp("[simulation]\ndlambda = \"fast\""));
		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().line == 2);
		CHECK(result.error().field == "simulation.dlambda");
	}

	SECTION("Syntax error in settings reports a line") {
		auto result = loadSettings(writeTemp("[world]\nhalf_width = = 1"));
		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().line == 2);
	}

	SECTION("Non-positive values are rejected") {
		CHECK_FALSE(loadSettings(writeTemp("[simulation]\ndlambda = 0")).has_value());
		CHECK_FALSE(loadSettings(writeTemp("[simulation]\nsteps_per_second = 0")).has_value());
		CHECK_FALSE(loadSettings(writeTemp("[world]\nmax_distance = -1")).has_value());
		CHECK_FALSE(loadSettings(writeTemp("[simulation]\nmax_rays = 0")).has_value());
	}

	SECTION("max_distance must cover the world") {
		auto bad = loadSettings(
			writeTemp("[world]\nhalf_width = 8\nhalf_height = 6\nmax_distance = 9"));
		REQUIRE_FALSE(bad.has_value());
		CHECK(bad.error().field == "world.max_distance");
		CHECK(loadSettings(writeTemp("[world]\nhalf_width = 8\nhalf_height = 6\nmax_distance = 11")).has_value());
	}

	SECTION("Settings round trip") {
		Settings s;
		s.window = {1024, 768};
		s.world = {9.5f, 5.25f, 20.f};
		s.simulation = {0.0125f, 120, 3000};
		s.view = {50, "binary"};
		auto path = tempPath("settings_roundtrip.toml");
		REQUIRE(saveSettings(s, path).has_value());
		auto loaded = loadSettings(path);
		REQUIRE(loaded.has_value());
		CHECK(*loaded == s);

		std::stringstream text;
		text << std::ifstream(path).rdbuf();
		CHECK(text.str().find("dlambda = 0.0125\n") != std::string::npos);
		CHECK(text.str().find("half_height = 5.25\n") != std::string::npos);
	}

	SECTION("Hex colors parse and format") {
		auto c = parseHexColor("#80ff00");
		REQUIRE(c.has_value());
		CHECK(c->r == Catch::Approx(128.f / 255.f));
		CHECK(c->g == Catch::Approx(1.f));
		CHECK(c->b == Catch::Approx(0.f));
		CHECK_FALSE(parseHexColor("#12").has_value());
		CHECK_FALSE(parseHexColor("80ff00").has_value());
		CHECK_FALSE(parseHexColor("#zzzzzz").has_value());
		for (const char* text: {"#80ff00", "#0080ff", "#ff0000", "#000000", "#123456"}) {
			CHECK(toHexColor(*parseHexColor(text)) == text);
		}
	}

	SECTION("Unknown settings keys are warnings") {
		std::vector<std::string> warnings;
		auto result = loadSettings(writeTemp("[window]\nwidth = 640\nheigth = 480\n[extra]\n"), &warnings);
		REQUIRE(result.has_value());
		CHECK(result->window.width == 640);
		REQUIRE(warnings.size() == 2);
		// "heigth" is a deliberate typo: that is the unknown key being tested.
		CHECK(warnings[0].ends_with(":3: window.heigth: unknown key"));
		CHECK(warnings[1].ends_with(":4: extra: unknown key"));
	}

	SECTION("Check failures in settings carry line and field") {
		auto result = loadSettings(writeTemp("[view]\nstars = -1"));
		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().line == 2);
		CHECK(result.error().field == "view.stars");
		CHECK(result.error().message == "must not be negative");
	}

	SECTION("Integer fields reject non-integers and out-of-range values") {
		auto fractional = loadSettings(writeTemp("[window]\nwidth = 800.5"));
		REQUIRE_FALSE(fractional.has_value());
		CHECK(fractional.error().field == "window.width");
		CHECK(fractional.error().line == 2);
		CHECK_FALSE(loadSettings(writeTemp("[window]\nwidth = true")).has_value());
		auto big = loadSettings(writeTemp("[window]\nwidth = 99999999999"));
		REQUIRE_FALSE(big.has_value());
		CHECK(big.error().field == "window.width");
		CHECK(big.error().message.find("range") != std::string::npos);
	}

	SECTION("Float fields reject non-finite values and bools") {
		CHECK_FALSE(loadSettings(writeTemp("[simulation]\ndlambda = inf")).has_value());
		CHECK_FALSE(loadSettings(writeTemp("[simulation]\ndlambda = nan")).has_value());
		CHECK_FALSE(loadSettings(writeTemp("[simulation]\ndlambda = true")).has_value());
		CHECK(loadSettings(writeTemp("[simulation]\ndlambda = 1")).has_value()); // integer is a number
	}

	SECTION("Missing settings file is an error") {
		auto result = loadSettings(tempPath("does_not_exist_settings.toml"));
		REQUIRE_FALSE(result.has_value());
		CHECK_FALSE(result.error().message.empty());
		CHECK_FALSE(formatError(result.error()).empty());
	}

	SECTION("Shipped config/settings.toml loads") {
		auto result = loadSettings(std::filesystem::path(BLACKHOLE_DATA_DIR) / "config" / "settings.toml");
		REQUIRE(result.has_value());
		CHECK(*result == Settings{});
	}
}

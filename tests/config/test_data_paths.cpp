//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_test_macros.hpp>

#include "config/data_paths.h"

TEST_CASE("Data paths come from the first argument or the default root") {
	char program[] = "blackhole_simulator";
	char custom[] = "some/dir";
	char* withArg[] = {program, custom};
	char* withoutArg[] = {program};

	const auto fromArg = SceneModule::dataPathsFromArgs(2, withArg);
	const std::filesystem::path root("some/dir");
	CHECK(fromArg.root == root);
	CHECK(fromArg.scenario("classic") == root / "scenarios" / "classic.toml");
	CHECK(fromArg.settings() == root / "config" / "settings.toml");
	CHECK(fromArg.styles() == root / "config" / "styles.toml");

	CHECK(SceneModule::dataPathsFromArgs(1, withoutArg).root == std::filesystem::path(BLACKHOLE_DATA_DIR));
}

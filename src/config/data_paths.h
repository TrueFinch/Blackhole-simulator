//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace SceneModule {
	// Where the config files and scenarios live: <root>/config and <root>/scenarios.
	struct DataPaths {
		std::filesystem::path root;

		[[nodiscard]] std::filesystem::path settings() const { return root / "config" / "settings.toml"; }

		[[nodiscard]] std::filesystem::path styles() const { return root / "config" / "styles.toml"; }

		[[nodiscard]] std::filesystem::path scenarios() const { return root / "scenarios"; }

		[[nodiscard]] std::filesystem::path scenario(std::string_view name) const {
			return scenarios() / (std::string(name) + ".toml");
		}
	};

	// The first command line argument if there is one, else BLACKHOLE_DATA_DIR (set by CMake).
	[[nodiscard]] DataPaths dataPathsFromArgs(int argc, char** argv);
}

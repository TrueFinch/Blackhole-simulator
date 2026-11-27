//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "core/settings.h"
#include "core/scene_description.h"
#include "toml/io_error.h"

namespace SceneModule {
	struct LoadedScene {
		SceneDescription scene;
		std::vector<std::string> warnings;
	};

	// Emitter ids are assigned 1, 2, 3... in file order. Emitter start points must lie closer to the
	// origin than world.maxDistance.
	[[nodiscard]] std::expected<LoadedScene, IoError>
	loadScene(const std::filesystem::path& path, const WorldSettings& world);

	[[nodiscard]] std::expected<void, IoError>
	saveScene(const SceneDescription& scene, const std::filesystem::path& path);

	// Unknown keys are reported in `warnings` (formatted with formatError) if it is given.
	[[nodiscard]] std::expected<StyleMap, IoError>
	loadStyles(const std::filesystem::path& path, std::vector<std::string>* warnings = nullptr);

	[[nodiscard]] std::expected<void, IoError>
	saveStyles(const StyleMap& styles, const std::filesystem::path& path);
}

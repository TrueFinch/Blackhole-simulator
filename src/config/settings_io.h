//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "core/settings.h"
#include "toml/io_error.h"

namespace SceneModule {
	// Missing keys keep their defaults. Unknown keys are not an error: they are reported in
	// `warnings` (formatted with formatError) if it is given.
	[[nodiscard]] std::expected<Settings, IoError>
	loadSettings(const std::filesystem::path& path, std::vector<std::string>* warnings = nullptr);

	[[nodiscard]] std::expected<void, IoError>
	saveSettings(const Settings& settings, const std::filesystem::path& path);
}

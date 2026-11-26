//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

#include <glm/glm.hpp>
#include <toml++/toml.hpp>

#include "toml/io_error.h"

// Low-level TOML helpers shared by the readers/writers in SceneModule.
// Field-by-field reading and writing lives in toml_schema.h.
namespace SceneModule {
	// Parses `path`. A missing file or a syntax error becomes an IoError (with the line if known).
	[[nodiscard]] std::expected<toml::table, IoError> parseTomlFile(const std::filesystem::path &path);

	// Creates parent directories and writes `root` to `path`.
	[[nodiscard]] std::expected<void, IoError>
	writeTomlFile(const toml::table &root, const std::filesystem::path &path);

	// Line of a TOML node (1-based), or 0 if unknown.
	[[nodiscard]] int lineOf(const toml::node &node);
	// Line of `key` in `section`, or 0 if absent.
	[[nodiscard]] int lineOf(const toml::table &section, std::string_view key);

	// Floats are stored as the shortest decimal that round-trips the float, so 0.0125f is
	// written as 0.0125 rather than 0.012500000186264515.
	[[nodiscard]] double toTomlFloat(float value);

	// "#rrggbb" <-> color with components in [0, 1].
	[[nodiscard]] std::expected<glm::vec3, std::string> parseHexColor(std::string_view text);
	[[nodiscard]] std::string toHexColor(glm::vec3 color);

}

//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <expected>
#include <filesystem>
#include <string>
#include <vector>

#include "toml/io_error.h"

namespace SceneModule {
	struct WindowSettings {
		int width = 800;
		int height = 600;

		bool operator==(const WindowSettings&) const = default;
	};

	struct WorldSettings {
		float halfWidth = 8.f;
		float halfHeight = 6.f;
		float maxDistance = 16.f;

		bool operator==(const WorldSettings&) const = default;
	};

	struct SimulationSettings {
		float dlambda = 0.05f;
		int stepsPerSecond = 60;
		int maxRays = 5000;

		bool operator==(const SimulationSettings&) const = default;
	};

	struct ViewSettings {
		int stars = 200;
		std::string defaultScenario = "classic";

		bool operator==(const ViewSettings&) const = default;
	};

	struct Settings {
		WindowSettings window;
		WorldSettings world;
		SimulationSettings simulation;
		ViewSettings view;

		bool operator==(const Settings&) const = default;
	};

	// Missing keys keep their defaults. Unknown keys are not an error: they are reported in
	// `warnings` (formatted with formatError) if it is given.
	[[nodiscard]] std::expected<Settings, IoError>
	loadSettings(const std::filesystem::path& path, std::vector<std::string>* warnings = nullptr);

	[[nodiscard]] std::expected<void, IoError>
	saveSettings(const Settings& settings, const std::filesystem::path& path);
}

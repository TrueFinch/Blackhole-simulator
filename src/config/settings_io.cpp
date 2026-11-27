//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "config/settings_io.h"

#include <cmath>

#include "toml/toml_schema.h"
#include "toml/toml_util.h"

namespace SceneModule {
	static const Schema<WindowSettings>& windowSchema() {
		static const Schema schema = {
			field("width", &WindowSettings::width, positive),
			field("height", &WindowSettings::height, positive),
		};
		return schema;
	}

	static const Schema<WorldSettings>& worldSchema() {
		static const Schema schema = {
			field("half_width", &WorldSettings::halfWidth, positive),
			field("half_height", &WorldSettings::halfHeight, positive),
			field("max_distance", &WorldSettings::maxDistance, positive),
		};
		return schema;
	}

	static const Schema<SimulationSettings>& simulationSchema() {
		static const Schema schema = {
			field("dlambda", &SimulationSettings::dlambda, positive),
			field("steps_per_second", &SimulationSettings::stepsPerSecond, positive),
			field("max_rays", &SimulationSettings::maxRays, positive),
		};
		return schema;
	}

	static const Schema<ViewSettings>& viewSchema() {
		static const Schema schema = {
			field("stars", &ViewSettings::stars, nonNegative),
			field("default_scenario", &ViewSettings::defaultScenario),
		};
		return schema;
	}

	static const Schema<Settings>& settingsSchema() {
		static const Schema schema = {
			section("window", &Settings::window, windowSchema()),
			section("world", &Settings::world, worldSchema()),
			section("simulation", &Settings::simulation, simulationSchema()),
			section("view", &Settings::view, viewSchema()),
		};
		return schema;
	}
}

namespace SceneModule {
	std::expected<Settings, IoError>
	loadSettings(const std::filesystem::path& path, std::vector<std::string>* warnings) {
		auto root = parseTomlFile(path);
		if (!root) {
			return std::unexpected(root.error());
		}

		ReadState state{.file = path.string()};
		auto read = readTable(*root, settingsSchema(), state);
		if (!read) {
			return std::unexpected(read.error());
		}
		const Settings& s = *read;

		// Cross-field checks.
		const float corner = std::hypot(s.world.halfWidth, s.world.halfHeight);
		if (s.world.maxDistance <= corner) {
			const toml::table* world = root->get_as<toml::table>("world");
			return std::unexpected(IoError{
				state.file,
				world ? lineOf(*world, "max_distance") : 0,
				"world.max_distance",
				"must exceed the distance to the world corner (" + std::to_string(corner) + ")"
			});
		}

		if (warnings) {
			warnings->insert(warnings->end(), state.warnings.begin(), state.warnings.end());
		}
		return s;
	}

	std::expected<void, IoError>
	saveSettings(const Settings& settings, const std::filesystem::path& path) {
		toml::table root;
		writeFields(settings, settingsSchema(), root);
		return writeTomlFile(root, path);
	}
}

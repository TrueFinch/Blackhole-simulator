//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "ui/app_controller.h"

#include <algorithm>
#include <filesystem>
#include <utility>

#include "config/settings_io.h"
#include "scene/scene_io.h"
#include "toml/io_error.h"

namespace {
	SceneModule::Settings loadSettingsOrDefault(
		const SceneModule::DataPaths& paths,
		std::vector<std::string>& errors
	) {
		auto settings = SceneModule::loadSettings(paths.settings());
		if (!settings) {
			errors.push_back(SceneModule::formatError(settings.error()));
			return {};
		}
		return std::move(*settings);
	}

	SceneModule::StyleMap loadStylesOrEmpty(
		const SceneModule::DataPaths& paths,
		std::vector<std::string>& errors
	) {
		auto styles = SceneModule::loadStyles(paths.styles());
		if (!styles) {
			errors.push_back(SceneModule::formatError(styles.error()));
			return {};
		}
		return std::move(*styles);
	}
} // namespace

UiModule::AppController::AppController(SceneModule::DataPaths paths)
	: m_paths(std::move(paths))
	, m_settings(loadSettingsOrDefault(m_paths, m_configErrors))
	, m_styles(loadStylesOrEmpty(m_paths, m_configErrors))
	, m_simulation({}, m_styles, m_settings)
	, m_clock(m_settings.simulation.stepsPerSecond) {
	selectScenario(m_settings.view.defaultScenario);
}

void UiModule::AppController::update(double frameSeconds) {
	if (m_paused) {
		m_clock.reset();
		return;
	}
	for (int i = m_clock.advance(frameSeconds); i > 0; --i) {
		m_simulation.step();
	}
}

std::vector<std::string> UiModule::AppController::scenarioNames() const {
	std::vector<std::string> names;
	std::error_code ec;
	for (std::filesystem::directory_iterator it(m_paths.scenarios(), ec), end; !ec && it != end; it.increment(ec)) {
		if (it->is_regular_file(ec) && it->path().extension() == ".toml") {
			names.push_back(it->path().stem().string());
		}
	}
	std::ranges::sort(names);
	return names;
}

bool UiModule::AppController::selectScenario(const std::string& name) {
	auto loaded = SceneModule::loadScene(m_paths.scenario(name), m_settings.world);
	if (!loaded) {
		m_lastError = SceneModule::formatError(loaded.error());
		return false;
	}
	m_simulation = SimulationModule::Simulation(std::move(loaded->scene), m_styles, m_settings);
	m_clock.reset();
	m_currentScenario = name;
	m_warnings = std::move(loaded->warnings);
	m_lastError.reset();
	return true;
}

void UiModule::AppController::restart() {
	m_simulation.restart();
	m_clock.reset();
}

bool UiModule::AppController::reloadFromDisk() {
	return selectScenario(m_currentScenario);
}

void UiModule::AppController::clearRays() {
	m_simulation.clearRays();
}

void UiModule::AppController::togglePause() {
	m_paused = !m_paused;
}

void UiModule::AppController::stepForward() {
	m_paused = true;
	m_simulation.step();
}

void UiModule::AppController::stepBack() {
	m_paused = true;
	m_simulation.seek(std::max(0, m_simulation.stepIndex() - 1));
}

void UiModule::AppController::seek(int step) {
	m_paused = true;
	m_simulation.seek(step);
}

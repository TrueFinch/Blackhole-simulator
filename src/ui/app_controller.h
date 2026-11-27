//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "config/data_paths.h"
#include "core/fixed_step_clock.h"
#include "core/scene_description.h"
#include "core/settings.h"
#include "core/simulation.h"

namespace UiModule {
	// Application logic without GL/ImGui: owns the simulation, picks scenarios, controls time.
	// Never throws on bad data files: errors go to lastError().
	class AppController {
	public:
		explicit AppController(SceneModule::DataPaths paths);

		// Advances the simulation by the frame time unless paused
		void update(double frameSeconds);

		// File names without .toml, sorted; rescans the folder on every call
		[[nodiscard]] std::vector<std::string> scenarioNames() const;

		// Empty if no scenario could be loaded
		[[nodiscard]] const std::string& currentScenario() const { return m_currentScenario; }

		// On failure the previous scene stays and lastError() is set
		bool selectScenario(const std::string& name);

		void restart();

		// Re-reads the current scenario file and restarts
		bool reloadFromDisk();

		void clearRays();

		void togglePause();

		[[nodiscard]] bool paused() const { return m_paused; }

		// Pauses and makes one step
		void stepForward();

		// Pauses and goes one step back (stays at 0)
		void stepBack();

		// Pauses at the target step (a negative target counts as 0)
		void seek(int step);

		[[nodiscard]] const SimulationModule::Simulation& simulation() const {
			return m_simulation;
		}

		[[nodiscard]] const SceneModule::Settings& settings() const {
			return m_settings;
		}

		// The last failed load (formatted with formatError); cleared by a successful selectScenario
		[[nodiscard]] const std::optional<std::string>& lastError() const {
			return m_lastError;
		}

		// Warnings of the current scenario file
		[[nodiscard]] const std::vector<std::string>& warnings() const {
			return m_warnings;
		}

	private:
		SceneModule::DataPaths m_paths;
		// Declared before the loaded data: the constructor records load errors here
		std::optional<std::string> m_lastError;
		SceneModule::Settings m_settings;
		SceneModule::StyleMap m_styles;
		SimulationModule::Simulation m_simulation;
		SimulationModule::FixedStepClock m_clock;
		std::string m_currentScenario;
		bool m_paused = false;
		std::vector<std::string> m_warnings;
	};
} // namespace UiModule

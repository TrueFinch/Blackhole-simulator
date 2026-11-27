//
// Created by Vladimir Glushkov on 24.11.2026.
//

#pragma once

#include <map>
#include <optional>
#include <vector>

#include "core/ray.h"
#include "core/scene_description.h"
#include "core/settings.h"

namespace SimulationModule {
	// Core of the application: owns the simulated rays and advances them in fixed steps,
	// firing the emitters of a scene description on their schedule.
	// Knows nothing about windows, OpenGL or how its objects are drawn;
	// the view layer reads its state through the const accessors.
	class Simulation {
	public:
		Simulation(
			SceneModule::SceneDescription scene,
			SceneModule::StyleMap sharedStyles,
			SceneModule::Settings settings
		);

		// One step: due volleys -> integrate every ray -> trim faded trails -> remove gone rays -> ++step
		void step();

		// Rays and schedule from scratch, step 0, same scene
		void restart();

		// Only the rays; the step index and the schedule stay
		void clearRays();

		// restart() and then targetStep steps (a negative target counts as 0)
		void seek(int targetStep);

		// Live edit: rays of removed emitters go away, new emitters fire from max(start, now),
		// kept emitters keep their next volley step
		void setScene(SceneModule::SceneDescription scene, SceneModule::StyleMap sharedStyles);

		void setSettings(const SceneModule::Settings& settings);

		[[nodiscard]] const std::vector<Ray>& rays() const { return m_rays; }

		[[nodiscard]] int stepIndex() const { return m_stepIndex; }

		[[nodiscard]] SceneModule::Seconds time() const;

		[[nodiscard]] const SceneModule::SceneDescription& scene() const { return m_scene; }

		// Style of the emitter's rays; the default style for an unknown emitter or style name.
		// The reference is valid until the next setScene/setSettings
		[[nodiscard]] const SceneModule::Style& styleOf(int emitterId) const;

		// Whether a volley was skipped on the last step because of maxRays
		[[nodiscard]] bool rayLimitHit() const { return m_rayLimitHit; }

		// Trail fade in steps of the emitter's style (nullopt = never); see SimulationModule::fadeSteps
		[[nodiscard]] std::optional<int> fadeStepsOf(int emitterId) const;

	private:
		SceneModule::SceneDescription m_scene;
		SceneModule::StyleMap m_sharedStyles;
		SceneModule::Settings m_settings;

		std::vector<Ray> m_rays;
		// Emitter id -> step of its next volley (nullopt: a one-shot emitter that has fired)
		std::map<int, std::optional<int>> m_nextFireStep;
		// Emitter id -> trail fade in steps (nullopt = never), cached from the styles
		std::map<int, std::optional<int>> m_fadeSteps;
		int m_stepIndex = 0;
		bool m_rayLimitHit = false;

		void resetSchedule();

		void updateFadeSteps();

		void fire(const SceneModule::EmitterDesc& emitter);

		// Removes the oldest finished rays so that `incoming` more fit into maxRays;
		// returns false (and removes nothing) if they would not fit even then
		bool makeRoom(int incoming);
	};
} // namespace SimulationModule

//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <optional>
#include <vector>

#include <glm/glm.hpp>

#include "core/scene_description.h"

namespace SimulationModule {
	struct RaySpawn {
		glm::vec2 pos;
		glm::vec2 dir; // unit vector
	};

	// Rays of one volley of the emitter.
	[[nodiscard]] std::vector<RaySpawn> volley(const SceneModule::EmitterDesc& emitter);

	// Simulation time to steps: rounded to the nearest step, never negative.
	[[nodiscard]] int toSteps(SceneModule::Seconds time, int stepsPerSecond);

	// Step of the first volley.
	[[nodiscard]] int startStep(const SceneModule::EmitterDesc& emitter, int stepsPerSecond);

	// Steps between volleys (at least 1); nullopt for a single-volley emitter.
	[[nodiscard]] std::optional<int> periodSteps(const SceneModule::EmitterDesc& emitter, int stepsPerSecond);
}

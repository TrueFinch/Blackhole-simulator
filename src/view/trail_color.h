//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <cstddef>
#include <optional>

#include <glm/vec4.hpp>

#include "core/scene_description.h"

namespace ViewModule {
	// Color of one trail point.
	// RGB: mix(trailFrom, trailTo, min(1, deflection / pi)) by the deflection of the point itself.
	// Alpha: with fade -> 1 - ageSteps / fadeSteps (clamped to 0..1);
	//        without fade -> 0.2 + 0.8 * index / (count - 1), the oldest point is the faintest.
	[[nodiscard]] glm::vec4 trailPointColor(const SceneModule::Style& style, float deflection, int ageSteps,
	                                        std::optional<int> fadeSteps, std::size_t index, std::size_t count);
} // namespace ViewModule

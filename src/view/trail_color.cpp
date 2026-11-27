//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "view/trail_color.h"

#include <algorithm>

#include <glm/glm.hpp>

#include "core/constants.h"

glm::vec4 ViewModule::trailPointColor(const SceneModule::Style& style, float deflection, int ageSteps,
                                      std::optional<int> fadeSteps, std::size_t index, std::size_t count) {
	constexpr float kMinAlpha = 0.2f;

	const float t = std::min(1.f, deflection / PhysicsModule::kPi);
	const glm::vec3 rgb = glm::mix(style.trailFrom, style.trailTo, t);

	float alpha = 1.f;
	if (fadeSteps && *fadeSteps > 0) {
		alpha = std::clamp(1.f - static_cast<float>(ageSteps) / static_cast<float>(*fadeSteps), 0.f, 1.f);
	} else if (!fadeSteps && count > 1) {
		alpha = kMinAlpha + (1.f - kMinAlpha) * static_cast<float>(index) / static_cast<float>(count - 1);
	}
	return {rgb, alpha};
}

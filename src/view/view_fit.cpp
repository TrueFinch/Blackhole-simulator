//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "view/view_fit.h"

#include <algorithm>

ViewModule::ViewFit ViewModule::fitView(int fbWidth, int fbHeight, glm::vec2 worldHalf) {
	if (fbWidth <= 0 || fbHeight <= 0) {
		return {worldHalf};
	}
	const auto w = static_cast<float>(fbWidth);
	const auto h = static_cast<float>(fbHeight);
	// Pixels per world unit: the whole world must fit on both axes
	const float scale = std::min(w / (2.f * worldHalf.x), h / (2.f * worldHalf.y));
	return {{w / (2.f * scale), h / (2.f * scale)}};
}

glm::vec2 ViewModule::worldToScreen(glm::vec2 world, const ViewFit& fit, int fbWidth, int fbHeight) {
	return {(world.x / fit.halfExtents.x * 0.5f + 0.5f) * static_cast<float>(fbWidth),
	        (0.5f - world.y / fit.halfExtents.y * 0.5f) * static_cast<float>(fbHeight)};
}

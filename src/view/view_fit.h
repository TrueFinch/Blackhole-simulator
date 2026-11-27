//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <glm/vec2.hpp>

namespace ViewModule {
	struct ViewFit {
		glm::vec2 halfExtents; // visible part of the world
	};

	// Keeps the scale 1:1 on both axes and fits [-worldHalf, worldHalf] into the framebuffer;
	// the extra space of one axis shows more of the world. A zero-size (minimized) window gets worldHalf.
	[[nodiscard]] ViewFit fitView(int fbWidth, int fbHeight, glm::vec2 worldHalf);

	// World point to framebuffer pixels: origin in the top-left corner, y goes down.
	[[nodiscard]] glm::vec2 worldToScreen(glm::vec2 world, const ViewFit& fit, int fbWidth, int fbHeight);
} // namespace ViewModule

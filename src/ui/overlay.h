//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <glm/vec2.hpp>

#include "core/scene_description.h"
#include "view/view_fit.h"

namespace UiModule {
	struct FrameStats {
		float fps = 0.f;
		float frameMs = 0.f;
		int step = 0;
		SceneModule::Seconds simTime{0.f};
		int liveRays = 0;
		bool rayLimitHit = false;
	};

	// Small semi-transparent window in the top-left corner
	void drawOverlay(const FrameStats& stats);

	// Label texts next to their world points, on the foreground draw list.
	// fb is the framebuffer size in pixels.
	void drawLabels(const SceneModule::SceneDescription& scene, const ViewModule::ViewFit& fit, glm::ivec2 fb);
} // namespace UiModule

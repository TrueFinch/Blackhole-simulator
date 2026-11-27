//
// Created by Vladimir Glushkov on 25.11.2026.
//

#pragma once

#include <vector>
#include <glm/vec2.hpp>

#include "../render/render.h"

namespace ViewModule {
	// Decorative twinkling background. Stars take no part in the physics,
	// so they live entirely in the view layer.
	class Starfield {
	public:
		// Stars fill the square [-extent, extent]^2, so they also cover the extra space of a stretched window
		Starfield(int count, float extent);

		void draw(RenderModule::RenderEngine& engine, float time);

	private:
		struct Star {
			glm::vec2 pos;
			float baseBrightness; // average brightness
			float amplitude; // how strong it twinkles
			float frequencyHz; // twinkle rate
			float phase1; // offset so stars don't pulse in sync
			float phase2; // offset so stars don't pulse in sync
		};

		std::vector<Star> m_stars;
		// Reused every frame to avoid reallocations
		std::vector<RenderModule::ColoredPoint> m_points;
	};
} // namespace ViewModule

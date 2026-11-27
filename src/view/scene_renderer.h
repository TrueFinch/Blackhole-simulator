//
// Created by Vladimir Glushkov on 25.11.2026.
//

#pragma once

#include <vector>

#include "core/settings.h"
#include "render/render.h"
#include "view/starfield.h"

namespace SimulationModule {
	class Simulation;
}

namespace ViewModule {
	// View layer: knows both the simulation objects and the render engine,
	// and decides how the objects look (colors, sizes, fading).
	// Reads the simulation state, never changes it.
	class SceneRenderer {
	public:
		SceneRenderer(int starsCount, const SceneModule::WorldSettings& world);

		void draw(RenderModule::RenderEngine& engine, const SimulationModule::Simulation& simulation, float time);

	private:
		Starfield m_starfield;
		// Reused every frame to avoid reallocations
		std::vector<RenderModule::ColoredPoint> m_rayHeads;
		std::vector<RenderModule::ColoredPoint> m_markers;
		std::vector<RenderModule::ColoredPoint> m_trailVertices;

		static void drawBlackHole(RenderModule::RenderEngine& engine);

		void drawRays(RenderModule::RenderEngine& engine, const SimulationModule::Simulation& simulation);

		void drawMarkers(RenderModule::RenderEngine& engine, const SimulationModule::Simulation& simulation);
	};
} // namespace ViewModule

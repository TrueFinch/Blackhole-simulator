//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "view/scene_renderer.h"

#include <algorithm>

#include "../core/constants.h"
#include "../core/ray.h"
#include "../core/simulation.h"

namespace {

	constexpr RenderModule::Color kEventHorizonColor{0.0f, 0.0f, 0.0f, 1.0f};
	constexpr RenderModule::Color kPhotonSphereColor{0.0f, 0.8f, 0.8f, 1.0f};
	constexpr RenderModule::Color kPointSourceColor{0.5f, 1.0f, 0.0f, 1.0f};

	// Trail color: depends on the scenario and on how much the ray has been deflected
	glm::vec3 trailColor(const SimulationModule::Ray& ray) {
		using namespace SimulationModule;
		// Normalize deflection (0 to π radians) to 0–1 scale
		constexpr float maxDeflection = PhysicsModule::kPi;
		const float t = std::min(1.0f, ray.deflection / maxDeflection);

		switch (ray.scenario) {
			case RayScenario::POINT_SOURCE:
				// Green to Yellow gradient based on deflection
				return {0.5f + 0.5f * t, 1.0f, 0.0f};
			case RayScenario::ORBITING:
				// Solid bright magenta (no gradient)
				return {1.0f, 0.2f, 1.0f};
			case RayScenario::PARALLEL:
				// Cyan to Purple to Red gradient
				return {t, 0.5f * (1.0f - t), 1.0f - t};
		}
		return {1.0f, 1.0f, 1.0f};
	}

	// Color of the bright point at the current ray position
	RenderModule::Color headColor(const SimulationModule::Ray& ray) {
		using namespace SimulationModule;
		switch (ray.scenario) {
			case RayScenario::POINT_SOURCE:
				return {0.5f, 1.0f, 0.0f, 1.0f}; // Lime green
			case RayScenario::ORBITING:
				return {1.0f, 0.2f, 1.0f, 1.0f}; // Magenta
			case RayScenario::PARALLEL:
				return {1.0f, 1.0f, 0.0f, 1.0f}; // Yellow
		}
		return {1.0f, 1.0f, 1.0f, 1.0f};
	}
} // namespace

ViewModule::SceneRenderer::SceneRenderer(int starsCount, float halfWidth, float halfHeight)
	: m_starfield(starsCount, halfWidth, halfHeight) {}

void ViewModule::SceneRenderer::draw(RenderModule::RenderEngine& engine,
									const SimulationModule::Simulation& simulation, float time) {
	// Back to front
	m_starfield.draw(engine, time);
	drawBlackHole(engine);
	drawPointSource(engine, simulation);
	drawRays(engine, simulation);
}

void ViewModule::SceneRenderer::drawBlackHole(RenderModule::RenderEngine& engine) {
	using namespace RenderModule;
	using SimulationModule::Simulation;
	constexpr glm::vec2 center{0.f, 0.f};
	// Event horizon (filled black circle)
	engine.drawFilledCircle(center, Simulation::kEventHorizonRadius, kEventHorizonColor);
	// Photon sphere (dashed outline on top)
	engine.drawDashedCircle(center, Simulation::kPhotonSphereRadius, kPhotonSphereColor, 2.0f);
}

void ViewModule::SceneRenderer::drawPointSource(RenderModule::RenderEngine& engine,
												const SimulationModule::Simulation& simulation) {
	using namespace RenderModule;
	// Mark the point source location once its rays are active
	if (simulation.isPointSourceActive()) {
		engine.drawPoint(SimulationModule::Simulation::pointSourcePosition(), 8.0f, kPointSourceColor);
	}
}

void ViewModule::SceneRenderer::drawRays(RenderModule::RenderEngine& engine,
										const SimulationModule::Simulation& simulation) {
	using namespace RenderModule;
	// Trails fade from the oldest point to the newest
	constexpr float kTrailAlphaStart = 0.2f;
	constexpr float kTrailAlphaEnd = 1.0f;
	constexpr float kTrailWidth = 1.5f;
	constexpr float kHeadSize = 3.f;

	m_rayHeads.clear();
	for (const auto& ray: simulation.rays()) {
		if (!simulation.isActive(ray)) {
			continue;
		}
		engine.drawFadingLineStrip(ray.trail, trailColor(ray), kTrailAlphaStart, kTrailAlphaEnd, kTrailWidth);
		if (!ray.trail.empty() && !ray.isCaptured()) {
			m_rayHeads.push_back({ray.trail.back(), headColor(ray)});
		}
	}
	// Current ray positions as bright points on top of all trails
	engine.drawPoints(m_rayHeads, kHeadSize);
}

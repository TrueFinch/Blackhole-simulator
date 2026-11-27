//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "view/scene_renderer.h"

#include <algorithm>

#include <glm/glm.hpp>

#include "core/constants.h"
#include "core/simulation.h"

namespace {
	constexpr RenderModule::Color kEventHorizonColor{0.0f, 0.0f, 0.0f, 1.0f};
	constexpr RenderModule::Color kPhotonSphereColor{0.0f, 0.8f, 0.8f, 1.0f};

	// Where an emitter's marker is drawn: the cone's apex, the middle of the line.
	glm::vec2 markerPosition(const SceneModule::ConeShape& cone) {
		return cone.pos;
	}

	glm::vec2 markerPosition(const SceneModule::LineShape& line) {
		return (line.from + line.to) * 0.5f;
	}
} // namespace

ViewModule::SceneRenderer::SceneRenderer(int starsCount, const SceneModule::WorldSettings& world)
	: m_starfield(starsCount, world.halfWidth, world.halfHeight) {}

void ViewModule::SceneRenderer::draw(RenderModule::RenderEngine& engine,
                                     const SimulationModule::Simulation& simulation, float time) {
	// Back to front
	m_starfield.draw(engine, time);
	drawBlackHole(engine);
	drawMarkers(engine, simulation);
	drawRays(engine, simulation);
}

void ViewModule::SceneRenderer::drawBlackHole(RenderModule::RenderEngine& engine) {
	using namespace PhysicsModule;
	constexpr glm::vec2 center{0.f, 0.f};
	// Event horizon (filled black circle)
	engine.drawFilledCircle(center, kEventHorizonRadius, kEventHorizonColor);
	// Photon sphere (dashed outline on top)
	engine.drawDashedCircle(center, kPhotonSphereRadius, kPhotonSphereColor, 2.0f);
}

void ViewModule::SceneRenderer::drawRays(RenderModule::RenderEngine& engine,
                                         const SimulationModule::Simulation& simulation) {
	using namespace RenderModule;
	// Trails fade from the oldest point to the newest
	constexpr float kTrailAlphaStart = 0.2f;
	constexpr float kTrailAlphaEnd = 1.0f;
	constexpr float kHeadSize = 3.f;

	m_rayHeads.clear();
	for (const auto& ray: simulation.rays()) {
		const SceneModule::Style& style = simulation.styleOf(ray.emitterId);
		// The whole trail takes the color of the current deflection (0..π)
		const float t = std::min(1.f, ray.deflection / PhysicsModule::kPi);
		const glm::vec3 trailColor = glm::mix(style.trailFrom, style.trailTo, t);
		m_trailPositions.clear();
		for (const auto& point: ray.trail) {
			m_trailPositions.push_back(point.pos);
		}
		engine.drawFadingLineStrip(m_trailPositions, trailColor, kTrailAlphaStart, kTrailAlphaEnd, style.width);
		if (!ray.trail.empty() && !ray.isCaptured()) {
			m_rayHeads.push_back({ray.trail.back().pos, Color(style.head, 1.f)});
		}
	}
	// Current ray positions as bright points on top of all trails
	engine.drawPoints(m_rayHeads, kHeadSize);
}

void ViewModule::SceneRenderer::drawMarkers(RenderModule::RenderEngine& engine,
                                            const SimulationModule::Simulation& simulation) {
	using namespace RenderModule;
	constexpr float kMarkerSize = 8.f;

	m_markers.clear();
	for (const auto& emitter: simulation.scene().emitters) {
		if (!SceneModule::showsMarker(emitter)) {
			continue;
		}
		const glm::vec2 pos = std::visit([](const auto& shape) { return markerPosition(shape); }, emitter.shape);
		m_markers.push_back({pos, Color(simulation.styleOf(emitter.id).head, 1.f)});
	}
	engine.drawPoints(m_markers, kMarkerSize);
}

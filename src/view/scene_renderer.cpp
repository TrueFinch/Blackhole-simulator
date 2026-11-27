//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "view/scene_renderer.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>

#include "core/constants.h"
#include "core/simulation.h"
#include "view/trail_color.h"

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

	// settings.view.stars is the density: the number of stars inside the world rectangle.
	// The starfield covers the larger square [-maxDistance, maxDistance]^2, so scale the count by area.
	int starCountForDensity(int starsInWorld, const SceneModule::WorldSettings& world) {
		const float squareArea = 4.f * world.maxDistance * world.maxDistance;
		const float worldArea = 4.f * world.halfWidth * world.halfHeight;
		return std::max(0, static_cast<int>(std::lround(static_cast<float>(starsInWorld) * squareArea / worldArea)));
	}
} // namespace

ViewModule::SceneRenderer::SceneRenderer(int starsCount, const SceneModule::WorldSettings& world)
	: m_starfield(starCountForDensity(starsCount, world), world.maxDistance) {}

void ViewModule::SceneRenderer::draw(RenderModule::RenderEngine& engine,
                                     const SimulationModule::Simulation& simulation, float time) {
	// Back to front
	m_starfield.draw(engine, time);
	drawBlackHole(engine);
	drawMarkers(engine, simulation);
	drawLabels(engine, simulation);
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
	constexpr float kHeadSize = 3.f;

	m_rayHeads.clear();
	for (const auto& ray: simulation.rays()) {
		const SceneModule::Style& style = simulation.styleOf(ray.emitterId);
		const std::optional<int> fadeSteps = simulation.fadeStepsOf(ray.emitterId);
		// Every point has its own color (by its deflection) and alpha (by its age)
		m_trailVertices.clear();
		std::size_t index = 0;
		for (const auto& point: ray.trail) {
			const int age = simulation.stepIndex() - point.step;
			m_trailVertices.push_back(
				{point.pos, trailPointColor(style, point.deflection, age, fadeSteps, index++, ray.trail.size())});
		}
		engine.drawLineStrip(m_trailVertices, style.width);
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

void ViewModule::SceneRenderer::drawLabels(RenderModule::RenderEngine& engine,
                                           const SimulationModule::Simulation& simulation) {
	using namespace RenderModule;
	constexpr float kLabelMarkerSize = 6.f;

	m_labelMarkers.clear();
	for (const auto& label: simulation.scene().labels) {
		m_labelMarkers.push_back({label.pos, Color(label.color, 1.f)});
	}
	engine.drawPoints(m_labelMarkers, kLabelMarkerSize);
}

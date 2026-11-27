//
// Created by Vladimir Glushkov on 24.11.2026.
//

#include "core/simulation.h"

#include <algorithm>
#include <limits>
#include <utility>

#include "core/emitter.h"

SimulationModule::Simulation::Simulation(SceneModule::SceneDescription scene, SceneModule::StyleMap sharedStyles,
                                         SceneModule::Settings settings)
	: m_scene(std::move(scene)), m_sharedStyles(std::move(sharedStyles)), m_settings(std::move(settings)) {
	updateFadeSteps();
	resetSchedule();
}

void SimulationModule::Simulation::step() {
	m_rayLimitHit = false;

	for (const SceneModule::EmitterDesc& emitter: m_scene.emitters) {
		std::optional<int>& next = m_nextFireStep[emitter.id];
		if (next && *next <= m_stepIndex) {
			// A skipped volley (ray limit) still moves the schedule on
			fire(emitter);
			const std::optional<int> period = periodSteps(emitter, m_settings.simulation.stepsPerSecond);
			next = period ? std::optional{m_stepIndex + *period} : std::nullopt;
		}
	}

	const float maxDistance = m_settings.world.maxDistance;
	for (Ray& ray: m_rays) {
		ray.integrate(m_settings.simulation.dlambda, maxDistance, m_stepIndex);
		if (const std::optional<int> fade = fadeStepsOf(ray.emitterId)) {
			ray.trimTrail(m_stepIndex, *fade);
		}
	}
	// A finished ray is gone once its trail has faded away; without fade the trail stays forever
	std::erase_if(m_rays, [&](const Ray& ray) { return ray.isFinished(maxDistance) && ray.trail.empty(); });

	++m_stepIndex;
}

void SimulationModule::Simulation::restart() {
	m_rays.clear();
	m_stepIndex = 0;
	m_rayLimitHit = false;
	resetSchedule();
}

void SimulationModule::Simulation::clearRays() {
	m_rays.clear();
}

void SimulationModule::Simulation::seek(int targetStep) {
	restart();
	for (int i = 0; i < targetStep; ++i) {
		step();
	}
}

void SimulationModule::Simulation::setScene(SceneModule::SceneDescription scene, SceneModule::StyleMap sharedStyles) {
	m_scene = std::move(scene);
	m_sharedStyles = std::move(sharedStyles);

	std::map<int, std::optional<int>> schedule;
	for (const SceneModule::EmitterDesc& emitter: m_scene.emitters) {
		if (auto it = m_nextFireStep.find(emitter.id); it != m_nextFireStep.end()) {
			schedule[emitter.id] = it->second;
		} else {
			schedule[emitter.id] = std::max(startStep(emitter, m_settings.simulation.stepsPerSecond), m_stepIndex);
		}
	}
	m_nextFireStep = std::move(schedule);

	std::erase_if(m_rays, [&](const Ray& ray) { return !m_nextFireStep.contains(ray.emitterId); });
	updateFadeSteps();
}

void SimulationModule::Simulation::setSettings(const SceneModule::Settings& settings) {
	m_settings = settings;
	updateFadeSteps();
}

SceneModule::Seconds SimulationModule::Simulation::time() const {
	return SceneModule::Seconds(static_cast<float>(m_stepIndex)
	                            / static_cast<float>(m_settings.simulation.stepsPerSecond));
}

const SceneModule::Style& SimulationModule::Simulation::styleOf(int emitterId) const {
	const auto emitter = std::ranges::find(m_scene.emitters, emitterId, &SceneModule::EmitterDesc::id);
	if (emitter == m_scene.emitters.end()) {
		static const SceneModule::Style defaultStyle;
		return defaultStyle;
	}
	return SceneModule::resolveStyle(emitter->style, m_scene.localStyles, m_sharedStyles);
}

void SimulationModule::Simulation::resetSchedule() {
	m_nextFireStep.clear();
	for (const SceneModule::EmitterDesc& emitter: m_scene.emitters) {
		m_nextFireStep[emitter.id] = startStep(emitter, m_settings.simulation.stepsPerSecond);
	}
}

void SimulationModule::Simulation::updateFadeSteps() {
	m_fadeSteps.clear();
	for (const SceneModule::EmitterDesc& emitter: m_scene.emitters) {
		const std::optional<SceneModule::Seconds> fade = styleOf(emitter.id).fade;
		// A fade shorter than a step still fades (after one step)
		m_fadeSteps[emitter.id] = fade
			                          ? std::optional{std::max(1, toSteps(*fade, m_settings.simulation.stepsPerSecond))}
			                          : std::nullopt;
	}
}

std::optional<int> SimulationModule::Simulation::fadeStepsOf(int emitterId) const {
	const auto it = m_fadeSteps.find(emitterId);
	return it != m_fadeSteps.end() ? it->second : std::nullopt;
}

void SimulationModule::Simulation::fire(const SceneModule::EmitterDesc& emitter) {
	const std::vector<RaySpawn> spawns = volley(emitter);
	if (!makeRoom(static_cast<int>(spawns.size()))) {
		m_rayLimitHit = true;
		return;
	}
	for (const RaySpawn& spawn: spawns) {
		m_rays.emplace_back(spawn.pos, spawn.dir, emitter.id, m_stepIndex);
	}
}

bool SimulationModule::Simulation::makeRoom(int incoming) {
	const auto maxRays = static_cast<std::size_t>(std::max(m_settings.simulation.maxRays, 0));
	const auto needed = static_cast<std::size_t>(incoming);
	if (m_rays.size() + needed <= maxRays) {
		return true;
	}
	const float maxDistance = m_settings.world.maxDistance;
	// Finished rays, oldest first: an empty trail counts as the oldest
	std::vector<std::pair<int, std::size_t>> finished; // (oldest trail step, ray index)
	for (std::size_t i = 0; i < m_rays.size(); ++i) {
		if (m_rays[i].isFinished(maxDistance)) {
			const int oldest = m_rays[i].trail.empty() ? std::numeric_limits<int>::min() : m_rays[i].trail.front().step;
			finished.emplace_back(oldest, i);
		}
	}
	if (m_rays.size() - finished.size() + needed > maxRays) {
		return false;
	}
	const std::size_t evictCount = m_rays.size() + needed - maxRays;
	std::ranges::sort(finished);
	std::vector<bool> evicted(m_rays.size(), false);
	for (std::size_t k = 0; k < evictCount; ++k) {
		evicted[finished[k].second] = true;
	}
	std::size_t index = 0;
	std::erase_if(m_rays, [&](const Ray&) { return evicted[index++]; });
	return true;
}

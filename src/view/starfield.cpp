//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "view/starfield.h"

#include <algorithm>
#include <cmath>
#include <random>

#include "../core/constants.h"

ViewModule::Starfield::Starfield(int count, float extent) {
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution<float> distX(-extent, extent);
	std::uniform_real_distribution<float> distY(-extent, extent);

	std::uniform_real_distribution baseBrightness(0.5f, 0.9f);
	std::uniform_real_distribution amplitude(0.05f, 0.2f);
	std::uniform_real_distribution frequencyHz(0.2f, 0.5f);
	std::uniform_real_distribution phase1(0.f, 2.f * PhysicsModule::kPi);
	std::uniform_real_distribution phase2(0.f, 2.f * PhysicsModule::kPi);

	m_stars.reserve(count);
	for (auto i = 0; i < count; ++i) {
		m_stars.push_back(Star{
			glm::vec2(distX(gen), distY(gen)),
			baseBrightness(gen), amplitude(gen),
			frequencyHz(gen), phase1(gen), phase2(gen)
		});
	}
	m_points.reserve(count);
}

void ViewModule::Starfield::draw(RenderModule::RenderEngine& engine, float time) {
	constexpr auto kStarSize = 3.f;
	constexpr auto kWeight1 = 0.7f;
	constexpr auto kWeight2 = 0.3f;
	// Frequency ratio of the second sine to the first.
	// With an integer (or simple fraction) ratio the combined pattern repeats
	// every few periods of the main sine. With an irrational ratio the peaks
	// line up differently each time, so the twinkle looks random but stays smooth.
	// The golden ratio is the irrational number worst approximated by fractions,
	// so the pattern doesn't even nearly repeat.
	constexpr auto kSecondSinSpeed = 1.618034f;

	m_points.clear();
	for (const auto& star: m_stars) {
		const auto speed = 2.f * PhysicsModule::kPi * star.frequencyHz;
		const auto wave = kWeight1 * std::sin(speed * time + star.phase1)
						+ kWeight2 * std::sin(speed * kSecondSinSpeed * time + star.phase2);
		const auto b = std::clamp(star.baseBrightness + star.amplitude * wave, 0.f, 1.f);
		m_points.push_back({star.pos, {b, b, b, 1.f}});
	}
	engine.drawPoints(m_points, kStarSize);
}

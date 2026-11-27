//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "core/emitter.h"

#include <algorithm>
#include <cmath>

namespace SimulationModule {
	namespace {
		using SceneModule::ConeShape;
		using SceneModule::LineShape;

		glm::vec2 unit(float angle) {
			return {std::cos(angle), std::sin(angle)};
		}

		// Fraction of the way across a volley for ray i of n: 0..1, or the middle for one ray.
		float fraction(int i, int n) {
			return n > 1 ? static_cast<float>(i) / static_cast<float>(n - 1) : 0.5f;
		}

		// The sector center: towards aim if it is set (and not the emitter's own position), else angle.
		float centerAngle(const ConeShape& cone) {
			if (cone.aim && *cone.aim != cone.pos) {
				const glm::vec2 d = *cone.aim - cone.pos;
				return std::atan2(d.y, d.x);
			}
			return cone.angle;
		}

		// All rays start at pos and fan out over the sector edge to edge.
		std::vector<RaySpawn> spawn(const ConeShape& cone, int n) {
			const float first = centerAngle(cone) - cone.spread / 2.f;
			std::vector<RaySpawn> rays;
			rays.reserve(n);
			for (int i = 0; i < n; ++i) {
				rays.push_back({cone.pos, unit(first + cone.spread * fraction(i, n))});
			}
			return rays;
		}

		// Parallel rays start evenly along from..to, ends included.
		std::vector<RaySpawn> spawn(const LineShape& line, int n) {
			const glm::vec2 dir = unit(line.angle);
			std::vector<RaySpawn> rays;
			rays.reserve(n);
			for (int i = 0; i < n; ++i) {
				rays.push_back({line.from + (line.to - line.from) * fraction(i, n), dir});
			}
			return rays;
		}
	}

	std::vector<RaySpawn> volley(const SceneModule::EmitterDesc& emitter) {
		const int n = std::max(emitter.rays, 0);
		return std::visit([n](const auto& shape) { return spawn(shape, n); }, emitter.shape);
	}

	int toSteps(SceneModule::Seconds time, int stepsPerSecond) {
		return std::max(0, static_cast<int>(std::lround(time.count() * static_cast<float>(stepsPerSecond))));
	}

	std::optional<int> fadeSteps(const SceneModule::Style& style, int stepsPerSecond) {
		if (!style.fade) {
			return std::nullopt;
		}
		return std::max(1, toSteps(*style.fade, stepsPerSecond));
	}

	int startStep(const SceneModule::EmitterDesc& emitter, int stepsPerSecond) {
		return toSteps(emitter.start, stepsPerSecond);
	}

	std::optional<int> periodSteps(const SceneModule::EmitterDesc& emitter, int stepsPerSecond) {
		if (!emitter.cooldown) {
			return std::nullopt;
		}
		return std::max(1, toSteps(*emitter.cooldown, stepsPerSecond));
	}
}

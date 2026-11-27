//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <algorithm>
#include <chrono>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

namespace SceneModule {
	// Simulation time. Fractional: scene files have values like 0.5.
	using Seconds = std::chrono::duration<float>;

	struct Style {
		glm::vec3 trailFrom{1.f};
		glm::vec3 trailTo{1.f};
		glm::vec3 head{1.f};
		float width = 1.5f;
		std::optional<Seconds> fade; // until a trail point vanishes; absent = never
		bool operator==(const Style &) const = default;
	};

	using StyleMap = std::map<std::string, Style>;

	// White style without fade, used for unknown style names.
	inline const std::string kDefaultStyleName = "default";

	// Rays start from `pos` and spread over a sector of `spread` radians.
	struct ConeShape {
		glm::vec2 pos{0.f};
		std::optional<glm::vec2> aim; // if set, the sector is centered on this point...
		float angle = 0.f;            // ...otherwise on this direction, rad
		float spread = 0.f;           // rad
		bool operator==(const ConeShape &) const = default;
	};

	// Parallel rays start evenly along the segment from..to.
	struct LineShape {
		glm::vec2 from{0.f}, to{0.f};
		float angle = 0.f; // ray direction, rad
		bool operator==(const LineShape &) const = default;
	};

	// In files the alternative is the emitter's `type` ("cone", "line").
	using EmitterShape = std::variant<ConeShape, LineShape>;

	struct EmitterDesc {
		int id = 0; // assigned on load/creation, never written to a file
		EmitterShape shape;
		int rays = 1;
		
		Seconds start{0.f};          // first volley
		std::optional<Seconds> cooldown; // between volleys; absent = a single volley
		std::string style = kDefaultStyleName;
		std::optional<bool> marker;    // absent = the shape's default, see showsMarker
		bool operator==(const EmitterDesc &) const = default;
	};

	// Whether the emitter's start point is drawn: cones by default, lines only if asked.
	[[nodiscard]] inline bool showsMarker(const EmitterDesc &e) {
		return e.marker.value_or(std::holds_alternative<ConeShape>(e.shape));
	}

	struct LabelDesc {
		glm::vec2 pos{0.f};
		std::string text;
		glm::vec3 color{1.f};
		bool operator==(const LabelDesc &) const = default;
	};

	struct SceneDescription {
		std::string name, description;
		std::vector<EmitterDesc> emitters;
		std::vector<LabelDesc> labels;
		StyleMap localStyles;
		bool operator==(const SceneDescription &) const = default;

		// max(id) + 1, or 1 for an empty scene.
		[[nodiscard]] int nextEmitterId() const {
			int next = 1;
			for (const EmitterDesc &e: emitters) next = std::max(next, e.id + 1);
			return next;
		}
	};

}

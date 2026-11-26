//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "toml/toml_schema.h"

#include <cmath>
#include <limits>

#include "toml/toml_util.h"

namespace {
	// A finite number (integer or float, not bool) converted to a finite float, or nullopt.
	std::optional<float> numberAsFloat(const toml::node& node) {
		if (!node.is_integer() && !node.is_floating_point()) {
			return std::nullopt;
		}
		const auto value = node.value<double>(); // empty for an integer a double cannot hold exactly
		if (!value || !std::isfinite(*value)) {
			return std::nullopt;
		}
		const auto f = static_cast<float>(*value);
		if (!std::isfinite(f)) {
			return std::nullopt;
		}
		return f;
	}
}

namespace SceneModule {
	std::string ReadContext::fieldName(const std::string_view key) const {
		if (where_.empty()) {
			return std::string(key);
		}
		return where_ + "." + std::string(key);
	}

	void ReadContext::fail(const toml::node& at, const std::string_view key, std::string message) const {
		if (state_->error) {
			return;
		}
		state_->error = IoError{state_->file, lineOf(at), fieldName(key), std::move(message)};
	}

	void ReadContext::warn(const toml::node& at, const std::string_view key, std::string message) const {
		state_->warnings.push_back(formatError(IoError{state_->file, lineOf(at), fieldName(key), std::move(message)}));
	}

	std::expected<int, std::string> Codec<int>::read(const toml::node& node) {
		if (!node.is_integer()) {
			return std::unexpected("expected an integer");
		}
		const std::int64_t v = *node.value<std::int64_t>();
		if (v < std::numeric_limits<int>::min() || v > std::numeric_limits<int>::max()) {
			return std::unexpected("integer is out of range");
		}
		return static_cast<int>(v);
	}

	std::expected<float, std::string> Codec<float>::read(const toml::node& node) {
		if (!node.is_integer() && !node.is_floating_point()) {
			return std::unexpected("expected a number");
		}
		if (auto v = numberAsFloat(node)) {
			return *v;
		}
		return std::unexpected("number must be finite");
	}

	double Codec<float>::write(float value) {
		return toTomlFloat(value);
	}

	std::expected<bool, std::string> Codec<bool>::read(const toml::node& node) {
		if (!node.is_boolean()) {
			return std::unexpected("expected true or false");
		}
		return *node.value<bool>();
	}

	std::expected<std::string, std::string> Codec<std::string>::read(const toml::node& node) {
		if (!node.is_string()) {
			return std::unexpected("expected a string");
		}
		return *node.value<std::string>();
	}

	std::expected<glm::vec2, std::string> Codec<glm::vec2>::read(const toml::node& node) {
		const toml::array* array = node.as_array();
		if (!array || array->size() != 2) {
			return std::unexpected("expected an array of two numbers");
		}
		const auto x = numberAsFloat(*array->get(0));
		const auto y = numberAsFloat(*array->get(1));
		if (!x || !y) {
			return std::unexpected("expected an array of two finite numbers");
		}
		return glm::vec2{*x, *y};
	}

	toml::array Codec<glm::vec2>::write(glm::vec2 value) {
		return toml::array{toTomlFloat(value.x), toTomlFloat(value.y)};
	}

	std::expected<glm::vec3, std::string> HexColor::read(const toml::node& node) {
		if (!node.is_string()) {
			return std::unexpected("expected a color like #rrggbb");
		}
		return parseHexColor(*node.value<std::string>());
	}

	std::string HexColor::write(glm::vec3 value) {
		return toHexColor(value);
	}
}

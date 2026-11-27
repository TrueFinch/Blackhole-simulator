//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "toml/toml_schema.h"

#include <charconv>
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

	std::expected<std::chrono::duration<float>, std::string>
	Codec<std::chrono::duration<float>>::read(const toml::node& node) {
		constexpr auto kExpected = R"(expected a duration like "1.5s" or "500ms")";
		if (!node.is_string()) {
			return std::unexpected(kExpected);
		}
		const std::string& text = node.as_string()->get();
		double number = 0.0;
		const auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), number);
		if (ec != std::errc{}) {
			return std::unexpected(kExpected);
		}
		const std::string_view unit(end, text.data() + text.size());
		double seconds = 0.0;
		if (unit == "s") {
			seconds = number;
		} else if (unit == "ms") {
			seconds = number / 1000.0;
		} else {
			return std::unexpected(kExpected);
		}
		const auto value = static_cast<float>(seconds);
		if (!std::isfinite(value)) {
			return std::unexpected("duration must be finite");
		}
		return std::chrono::duration<float>(value);
	}

	std::string Codec<std::chrono::duration<float>>::write(std::chrono::duration<float> value) {
		const float seconds = value.count();
		char buffer[64];
		// The shortest digits that read back as this float, without an exponent: "0.0125", "3".
		const auto written = std::to_chars(buffer, buffer + sizeof(buffer), seconds, std::chars_format::fixed);
		const std::string text(buffer, written.ptr);
		if (seconds == 0.f || std::abs(seconds) >= 1.f) {
			return text + "s";
		}
		// Below a second: move the decimal point three places right in the text, not in the number
		// (seconds * 1000 in float turns 0.254 into 254.00002).
		const bool negative = text.front() == '-';
		std::string fraction = text.substr(text.find('.') + 1); // "0.0125" -> "0125"
		fraction.resize(std::max<size_t>(fraction.size(), 3), '0'); // "5" -> "500"
		std::string whole = fraction.substr(0, 3);
		const std::string rest = fraction.substr(3);
		whole.erase(0, std::min(whole.find_first_not_of('0'), whole.size() - 1)); // "012" -> "12", "000" -> "0"
		return (negative ? "-" : "") + whole + (rest.empty() ? "" : "." + rest) + "ms";
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

//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "toml/toml_util.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace SceneModule {
	std::expected<std::string, IoError> readTextFile(const std::filesystem::path& path) {
		std::ifstream in(path, std::ios::binary);
		if (!in) {
			return std::unexpected(IoError{path.string(), 0, "", "cannot open file"});
		}
		std::stringstream buffer;
		buffer << in.rdbuf();
		return buffer.str();
	}

	std::expected<toml::table, IoError> parseToml(const std::string& text, const std::string& file) {
		try {
			return toml::parse(text, file);
		} catch (const toml::parse_error& e) {
			return std::unexpected(IoError{
				file, static_cast<int>(e.source().begin.line), "",
				std::string(e.description())
			});
		}
	}

	std::expected<toml::table, IoError> parseTomlFile(const std::filesystem::path& path) {
		auto text = readTextFile(path);
		if (!text) {
			return std::unexpected(text.error());
		}
		return parseToml(*text, path.string());
	}

	std::expected<void, IoError> writeTomlFile(const toml::table& root, const std::filesystem::path& path) {
		const std::string file = path.string();
		if (path.has_parent_path()) {
			// create_directories returns false when the directory already exists, which is fine:
			// only `ec` signals a real failure.
			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec) {
				return std::unexpected(IoError{file, 0, "", "cannot create directory: " + ec.message()});
			}
		}

		std::ofstream out(path);
		if (!out) {
			return std::unexpected(IoError{file, 0, "", "cannot open file for writing"});
		}
		out << root << "\n";
		out.close();
		if (!out) {
			return std::unexpected(IoError{file, 0, "", "failed to write file"});
		}
		return {};
	}

	int lineOf(const toml::node& node) {
		return static_cast<int>(node.source().begin.line);
	}

	int lineOf(const toml::table& section, std::string_view key) {
		const toml::node* node = section.get(key);
		return node ? lineOf(*node) : 0;
	}

	double toTomlFloat(float value) {
		char buffer[64];
		const auto written = std::to_chars(buffer, buffer + sizeof(buffer), value);
		auto result = static_cast<double>(value);
		std::from_chars(buffer, written.ptr, result);
		return result;
	}

	std::expected<glm::vec3, std::string> parseHexColor(std::string_view text) {
		if (text.size() != 7 || text[0] != '#') {
			return std::unexpected("expected a color like #rrggbb");
		}
		glm::vec3 color{};
		for (int i = 0; i < 3; ++i) {
			int value = 0;
			for (int j = 0; j < 2; ++j) {
				const char c = text[1 + i * 2 + j];
				int digit;
				if (c >= '0' && c <= '9') {
					digit = c - '0';
				} else if (c >= 'a' && c <= 'f') {
					digit = c - 'a' + 10;
				} else if (c >= 'A' && c <= 'F') {
					digit = c - 'A' + 10;
				} else {
					return std::unexpected("expected a color like #rrggbb");
				}
				value = value * 16 + digit;
			}
			color[i] = static_cast<float>(value) / 255.f;
		}
		return color;
	}

	std::string toHexColor(glm::vec3 color) {
		char buffer[8];
		auto channel = [](float v) {
			return static_cast<unsigned>(std::lround(std::clamp(v, 0.f, 1.f) * 255.f));
		};
		std::snprintf(buffer, sizeof(buffer), "#%02x%02x%02x", channel(color.r), channel(color.g),
					channel(color.b));
		return buffer;
	}
}

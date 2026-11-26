//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <sstream>

#include "toml/toml_schema.h"

using namespace SceneModule;

namespace {
	struct Inner {
		int count = 1;
		bool flag = false;

		bool operator==(const Inner&) const = default;
	};

	struct Sample {
		float width = 1.5f;
		std::string name = "default";
		glm::vec2 pos{0.f};
		glm::vec3 color{1.f};
		std::optional<float> cooldown;
		Inner inner;

		bool operator==(const Sample&) const = default;
	};

	const Schema<Inner>& innerSchema() {
		static const Schema schema = {
			field("count", &Inner::count, atLeast<1>),
			field("flag", &Inner::flag),
		};
		return schema;
	}

	const Schema<Sample>& sampleSchema() {
		static const Schema schema = {
			field("width", &Sample::width, positive),
			field("name", &Sample::name),
			field("pos", &Sample::pos),
			field<HexColor>("color", &Sample::color),
			field("cooldown", &Sample::cooldown, positive),
			section("inner", &Sample::inner, innerSchema()),
		};
		return schema;
	}

	struct Result {
		Sample sample;
		ReadState state;
	};

	Result read(std::string_view text, const Schema<Sample>& schema = sampleSchema()) {
		Result r{.state = {.file = "test.toml"}};
		const bool ok = readFields(toml::parse(text), schema, r.sample, ReadContext(r.state));
		REQUIRE(ok == !r.state.error.has_value());
		return r;
	}
}

TEST_CASE("Toml (de)serialization") {
	SECTION("Schema reads every codec") {
		auto r = read(R"(
width = 2
name = "beam"
pos = [-7.5, 3]
color = "#ff8000"
cooldown = 0.25
[inner]
count = 3
flag = true
)");
		REQUIRE_FALSE(r.state.error);
		CHECK(r.state.warnings.empty());
		CHECK(r.sample.width == 2.f);
		CHECK(r.sample.name == "beam");
		CHECK(r.sample.pos == glm::vec2{-7.5f, 3.f});
		CHECK(r.sample.color.g == Catch::Approx(128.f / 255.f));
		CHECK(r.sample.cooldown == 0.25f);
		CHECK(r.sample.inner == Inner{3, true});
	}

	SECTION("Schema keeps defaults for absent keys") {
		auto r = read("name = \"x\"");
		REQUIRE_FALSE(r.state.error);
		Sample expected;
		expected.name = "x";
		CHECK(r.sample == expected);
		CHECK_FALSE(r.sample.cooldown.has_value());
	}

	SECTION("Schema type errors carry line and dotted field") {
		auto r = read("width = 1\n[inner]\ncount = 2\nflag = \"yes\"");
		REQUIRE(r.state.error);
		CHECK(r.state.error->line == 4);
		CHECK(r.state.error->field == "inner.flag");
		CHECK(r.state.error->message == "expected true or false");
		CHECK(r.state.error->file == "test.toml");
	}

	SECTION("Schema checks report their message") {
		auto r = read("\n\nwidth = 0");
		REQUIRE(r.state.error);
		CHECK(r.state.error->line == 3);
		CHECK(r.state.error->field == "width");
		CHECK(r.state.error->message == "must be positive");

		auto nested = read("[inner]\ncount = 0");
		REQUIRE(nested.state.error);
		CHECK(nested.state.error->message == "must be at least 1");

		auto optional = read("cooldown = -1");
		REQUIRE(optional.state.error);
		CHECK(optional.state.error->field == "cooldown");
	}

	SECTION("Schema rejects bad numbers") {
		CHECK(read("width = true").state.error->message == "expected a number");
		CHECK(read("width = inf").state.error->message == "number must be finite");
		CHECK(read("width = 1e300").state.error->message == "number must be finite");
		CHECK(read("[inner]\ncount = 1.5").state.error->message == "expected an integer");
		CHECK(read("[inner]\ncount = 3000000000").state.error->message == "integer is out of range");
		CHECK(read("pos = [1]").state.error->message == "expected an array of two numbers");
		CHECK(read("pos = [1, nan]").state.error->message == "expected an array of two finite numbers");
		CHECK(read("color = \"red\"").state.error->message == "expected a color like #rrggbb");
		CHECK(read("inner = 5").state.error->message == "expected a table");
	}

	SECTION("Schema keeps only the first error") {
		auto r = read("width = 0\nname = 5");
		REQUIRE(r.state.error);
		CHECK(r.state.error->field == "width");
	}

	SECTION("Schema warns about unknown keys without failing") {
		auto r = read("width = 2\nwidht = 3\n[inner]\ncolor = 1");
		REQUIRE_FALSE(r.state.error);
		CHECK(r.sample.width == 2.f);
		// Nested tables are checked while their section is read, before the outer table's own keys.
		REQUIRE(r.state.warnings.size() == 2);
		CHECK(r.state.warnings[0] == "test.toml:4: inner.color: unknown key");
		CHECK(r.state.warnings[1] == "test.toml:2: widht: unknown key");
	}

	SECTION("Schema required fields") {
		static const Schema schema = {
			required(field("name", &Sample::name)),
			field("width", &Sample::width),
		};
		auto missing = read("width = 2", schema);
		REQUIRE(missing.state.error);
		CHECK(missing.state.error->field == "name");
		CHECK(missing.state.error->message == "is required");
		CHECK_FALSE(read("name = \"ok\"", schema).state.error);
	}

	SECTION("Schema writes and reads back") {
		Sample s;
		s.width = 0.0125f;
		s.name = "beam";
		s.pos = {-8.f, 2.5787f};
		s.color = {1.f, 0.f, 128.f / 255.f};
		s.cooldown = 3.f;
		s.inner = {7, true};

		toml::table table;
		writeFields(s, sampleSchema(), table);
		std::ostringstream text;
		text << table;
		// Floats are written in their shortest round-tripping form (see TOML_FLOAT_CHARCONV in CMake).
		CHECK(text.str().find("width = 0.0125\n") != std::string::npos);
		CHECK(text.str().find("pos = [ -8.0, 2.5787 ]") != std::string::npos);
		CHECK(text.str().find("cooldown = 3.0\n") != std::string::npos);
		CHECK(text.str().find("color = '#ff0080'") != std::string::npos);

		auto r = read(text.str());
		REQUIRE_FALSE(r.state.error);
		CHECK(r.state.warnings.empty());
		CHECK(r.sample == s);
	}

	SECTION("Schema skips empty optionals when writing") {
		toml::table table;
		writeFields(Sample{}, sampleSchema(), table);
		CHECK_FALSE(table.contains("cooldown"));
		CHECK(table.contains("inner"));
	}

	SECTION("readTable returns the value or the first error") {
		ReadState state{.file = "test.toml"};
		auto ok = readTable(toml::parse("width = 2\nwidht = 3"), sampleSchema(), state);
		REQUIRE(ok.has_value());
		CHECK(ok->width == 2.f);
		CHECK(state.warnings.size() == 1);

		ReadState failed{.file = "test.toml"};
		auto bad = readTable(toml::parse("[inner]\ncount = 0"), sampleSchema(), failed);
		REQUIRE_FALSE(bad.has_value());
		CHECK(bad.error().field == "inner.count");
		CHECK(bad.error().line == 2);
	}

	SECTION("readTable starts from the given initial value") {
		Sample initial;
		initial.name = "preset";
		ReadState state{.file = "test.toml"};
		auto r = readTable(toml::parse("width = 4"), sampleSchema(), state, initial);
		REQUIRE(r.has_value());
		CHECK(r->name == "preset");
		CHECK(r->width == 4.f);
	}
}

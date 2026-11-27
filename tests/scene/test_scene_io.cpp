//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "scene/scene_io.h"
#include "toml/toml_util.h"

using namespace SceneModule;
using Catch::Approx;

namespace {

	std::filesystem::path tempPath(const std::string &name) {
		auto dir = std::filesystem::temp_directory_path() / "bh_tests";
		std::filesystem::create_directories(dir);
		return dir / name;
	}

	std::filesystem::path writeTemp(const std::string &content) {
		static std::atomic<int> counter{0};
		auto path = tempPath("scene_" + std::to_string(counter++) + ".toml");
		std::ofstream(path) << content;
		return path;
	}

	std::string readAll(const std::filesystem::path &path) {
		std::ifstream in(path);
		std::stringstream ss;
		ss << in.rdbuf();
		return ss.str();
	}

	const WorldSettings kWorld{}; // max_distance 16

	std::expected<LoadedScene, IoError> loadText(const std::string &text) {
		return loadScene(writeTemp(text), kWorld);
	}

	// Asserts a load failure on `line` for `field`.
	void checkError(const std::string &text, int line, const std::string &field) {
		INFO(text);
		auto result = loadText(text);
		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().line == line);
		CHECK(result.error().field == field);
		CHECK_FALSE(result.error().message.empty());
	}

	const char *kConeText = R"(
name = "Star"
description = "Observer demo"

[[emitter]]
type = "cone"
pos = [-7.6, 5.1]
aim = [0, 0]
spread = 1.047
rays = 25
start = "500ms"
cooldown = "1.5s"
style = "lime"
)";

}

TEST_CASE("Scene IO") {
	SECTION("Cone emitter with aim and cooldown loads") {
		auto result = loadText(kConeText);
		REQUIRE(result.has_value());
		const SceneDescription &s = result->scene;
		CHECK(s.name == "Star");
		CHECK(s.description == "Observer demo");
		REQUIRE(s.emitters.size() == 1);
		const EmitterDesc &e = s.emitters[0];
		CHECK(e.id == 1);
		const auto *cone = std::get_if<ConeShape>(&e.shape);
		REQUIRE(cone);
		CHECK(cone->pos.x == Approx(-7.6f));
		CHECK(cone->pos.y == Approx(5.1f));
		REQUIRE(cone->aim.has_value());
		CHECK(cone->aim->x == Approx(0.f));
		CHECK(cone->aim->y == Approx(0.f));
		CHECK(cone->spread == Approx(1.047f));
		CHECK(e.rays == 25);
		CHECK(e.start.count() == Approx(0.5f));
		REQUIRE(e.cooldown.has_value());
		CHECK(e.cooldown->count() == Approx(1.5f));
		CHECK(e.style == "lime");
		CHECK_FALSE(e.marker.has_value());
		CHECK(showsMarker(e));
		CHECK(result->warnings.empty());
	}

	SECTION("Line emitter loads with marker false by default") {
		auto result = loadText(R"(
	[[emitter]]
	type = "line"
	from = [-8, -6]
	to = [-8, 6]
	angle = 0.5
	rays = 70
	style = "rainbow"
	)");
		REQUIRE(result.has_value());
		REQUIRE(result->scene.emitters.size() == 1);
		const EmitterDesc &e = result->scene.emitters[0];
		const auto *line = std::get_if<LineShape>(&e.shape);
		REQUIRE(line);
		CHECK(line->from == glm::vec2(-8.f, -6.f));
		CHECK(line->to == glm::vec2(-8.f, 6.f));
		CHECK(line->angle == Approx(0.5f));
		CHECK(e.rays == 70);
		CHECK_FALSE(showsMarker(e));

		auto withMarker = loadText("[[emitter]]\ntype = \"line\"\nfrom = [0, 0]\nto = [1, 1]\nmarker = true");
		REQUIRE(withMarker.has_value());
		CHECK(showsMarker(withMarker->scene.emitters[0]));

		auto coneNoMarker = loadText("[[emitter]]\ntype = \"cone\"\npos = [1, 1]\nmarker = false");
		REQUIRE(coneNoMarker.has_value());
		CHECK_FALSE(showsMarker(coneNoMarker->scene.emitters[0]));
	}

	SECTION("Emitter without cooldown is one-shot") {
		auto result = loadText("[[emitter]]\ntype = \"cone\"\npos = [1, 2]\nangle = 3.0");
		REQUIRE(result.has_value());
		const EmitterDesc &e = result->scene.emitters[0];
		CHECK_FALSE(e.cooldown.has_value());
		const auto *cone = std::get_if<ConeShape>(&e.shape);
		REQUIRE(cone);
		CHECK_FALSE(cone->aim.has_value());
		CHECK(cone->angle == Approx(3.f));
		CHECK(e.rays == 1);
		CHECK(e.style == kDefaultStyleName);
	}

	SECTION("Emitter ids follow file order") {
		auto result = loadText(
			"[[emitter]]\ntype = \"cone\"\npos = [1, 2]\n"
			"[[emitter]]\ntype = \"cone\"\npos = [3, 4]\n"
			"[[emitter]]\ntype = \"cone\"\npos = [5, 6]\n");
		REQUIRE(result.has_value());
		REQUIRE(result->scene.emitters.size() == 3);
		CHECK(result->scene.emitters[0].id == 1);
		CHECK(result->scene.emitters[1].id == 2);
		CHECK(result->scene.emitters[2].id == 3);
		CHECK(result->scene.nextEmitterId() == 4);
		CHECK(SceneDescription{}.nextEmitterId() == 1);
	}

	SECTION("Labels and local styles load") {
		auto result = loadText(R"(
	[[label]]
	pos = [7, -3]
	text = "Observer"
	color = "#ff8000"

	[styles.my_local]
	trail_from = "#ff8800"
	trail_to = "#ff0000"
	head = "#ffaa00"
	width = 2
	)");
		REQUIRE(result.has_value());
		REQUIRE(result->scene.labels.size() == 1);
		const LabelDesc &l = result->scene.labels[0];
		CHECK(l.pos == glm::vec2(7.f, -3.f));
		CHECK(l.text == "Observer");
		CHECK(l.color.r == Approx(1.f));
		CHECK(l.color.g == Approx(128.f / 255.f));
		REQUIRE(result->scene.localStyles.contains("my_local"));
		const Style &st = result->scene.localStyles.at("my_local");
		CHECK(st.trailFrom.g == Approx(136.f / 255.f));
		CHECK(st.trailTo == glm::vec3(1.f, 0.f, 0.f));
		CHECK(st.width == Approx(2.f));
		CHECK_FALSE(st.fade.has_value());
	}

	SECTION("Invalid emitter fields are errors") {
		const std::string cone = "[[emitter]]\ntype = \"cone\"\npos = [1, 1]\n";
		checkError(cone + "rays = 0", 4, "emitter[1].rays");
		checkError(cone + "rays = \"many\"", 4, "emitter[1].rays");
		checkError(cone + "rays = 2.5", 4, "emitter[1].rays");
		checkError(cone + "rays = true", 4, "emitter[1].rays");
		checkError(cone + "rays = 99999999999", 4, "emitter[1].rays");
		checkError(cone + R"(cooldown = "-1s")", 4, "emitter[1].cooldown");
		checkError(cone + R"(cooldown = "0ms")", 4, "emitter[1].cooldown");
		checkError(cone + "spread = -0.1", 4, "emitter[1].spread");
		checkError(cone + R"(start = "-1s")", 4, "emitter[1].start");
		checkError(cone + "spread = inf", 4, "emitter[1].spread");
		checkError("[[emitter]]\ntype = \"cone\"\npos = [1]", 3, "emitter[1].pos");
		checkError("[[emitter]]\ntype = \"cone\"\npos = [1, \"a\"]", 3, "emitter[1].pos");
		checkError("[[emitter]]\ntype = \"spiral\"", 2, "emitter[1].type");
		checkError(cone + "style = 5", 4, "emitter[1].style");
		checkError(cone + "marker = 1", 4, "emitter[1].marker");
		// 1-based index of the failing emitter
		checkError(cone + "[[emitter]]\ntype = \"cone\"\npos = [1, 1]\nrays = 0", 7, "emitter[2].rays");
		// required fields
		checkError("[[emitter]]\ntype = \"cone\"", 1, "emitter[1].pos");
		checkError("[[emitter]]\ntype = \"line\"\nto = [1, 1]", 1, "emitter[1].from");
		checkError("[[emitter]]\ntype = \"line\"\nfrom = [1, 1]", 1, "emitter[1].to");
		// labels and styles
		checkError("[[label]]\ntext = \"a\"\ncolor = \"#12\"", 3, "label[1].color");
		checkError("[[label]]\ntext = 5", 2, "label[1].text");
		checkError("[[label]]\ntext = \"a\"\npos = [0, 0]\n[[label]]\ntext = \"b\"\npos = 3", 6, "label[2].pos");
		checkError("[styles.lime]\nfade = \"-1s\"", 2, "styles.lime.fade");
		checkError("[styles.lime]\nfade = \"0s\"", 2, "styles.lime.fade"); // never = no fade key
		checkError("[styles.lime]\nwidth = 0", 2, "styles.lime.width");
		checkError("[styles.lime]\nhead = \"#12\"", 2, "styles.lime.head");
		checkError("name = 5", 1, "name");
	}

	SECTION("Emitter outside max_distance is an error") {
		checkError("[[emitter]]\ntype = \"cone\"\npos = [20, 0]", 3, "emitter[1].pos");
		checkError("[[emitter]]\ntype = \"line\"\nfrom = [0, 0]\nto = [0, 17]", 4, "emitter[1].to");
		checkError("[[emitter]]\ntype = \"line\"\nfrom = [-17, 0]\nto = [0, 0]", 3, "emitter[1].from");
		CHECK(loadText("[[emitter]]\ntype = \"cone\"\npos = [15, 0]").has_value());
	}

	SECTION("Unknown keys produce warnings") {
		auto result = loadText("foo = 1\nname = \"a\"");
		REQUIRE(result.has_value());
		CHECK(result->warnings.size() == 1);

		auto nested = loadText(
			"[[emitter]]\ntype = \"cone\"\npos = [1, 1]\nbar = 1\n"
			"[[label]]\ntext = \"a\"\nbaz = 1\n"
			"[styles.s]\nqux = 1\n");
		REQUIRE(nested.has_value());
		CHECK(nested->warnings.size() == 3);
	}

	SECTION("A key of another emitter type is unknown") {
		auto cone = loadText("[[emitter]]\ntype = \"cone\"\npos = [1, 1]\nfrom = [0, 0]");
		REQUIRE(cone.has_value());
		REQUIRE(cone->warnings.size() == 1);
		CHECK(cone->warnings[0].find("emitter[1].from: unknown key") != std::string::npos);

		auto line = loadText("[[emitter]]\ntype = \"line\"\nfrom = [0, 0]\nto = [1, 1]\nspread = 1");
		REQUIRE(line.has_value());
		REQUIRE(line->warnings.size() == 1);
		CHECK(line->warnings[0].find("emitter[1].spread: unknown key") != std::string::npos);
	}

	SECTION("Emitter without type is a cone") {
		auto result = loadText("[[emitter]]\npos = [1, 1]");
		REQUIRE(result.has_value());
		CHECK(std::holds_alternative<ConeShape>(result->scene.emitters[0].shape));
	}

	SECTION("Scene round trip") {
		SceneDescription s;
		s.name = "Round trip";
		s.description = "text";
		EmitterDesc cone;
		cone.id = 1;
		cone.shape = ConeShape{.pos = {-7.6f, 5.1f}, .aim = glm::vec2(0.f, 0.f), .spread = 1.047f};
		cone.rays = 25;
		cone.start = Seconds{0.5f};
		cone.cooldown = Seconds{1.5f};
		cone.style = "lime";
		EmitterDesc cone2;
		cone2.id = 2;
		cone2.shape = ConeShape{.pos = {3.f, -4.f}, .angle = 2.5f};
		cone2.marker = false;
		EmitterDesc line;
		line.id = 3;
		line.shape = LineShape{.from = {-8.f, -6.f}, .to = {-8.f, 6.f}, .angle = 0.25f};
		line.rays = 70;
		line.style = "rainbow";
		line.marker = true;
		EmitterDesc line2;
		line2.id = 4;
		line2.shape = LineShape{.from = {1.f, 2.f}, .to = {3.f, 4.f}};
		line2.marker = false;
		line2.cooldown = Seconds{2.f};
		s.emitters = {cone, cone2, line, line2};
		s.labels = {{{7.f, -3.f}, "Observer", *parseHexColor("#ff8000")}};
		Style st;
		st.trailFrom = *parseHexColor("#ff8800");
		st.trailTo = *parseHexColor("#ff0000");
		st.head = *parseHexColor("#ffaa00");
		st.width = 2.f;
		st.fade = Seconds{3.f};
		s.localStyles["my_local"] = st;

		auto path = tempPath("scene_roundtrip.toml");
		REQUIRE(saveScene(s, path).has_value());
		auto loaded = loadScene(path, kWorld);
		REQUIRE(loaded.has_value());
		CHECK(loaded->scene == s);
		CHECK(loaded->warnings.empty());

		const std::string text = readAll(path);
		CHECK(text.find("7.6") != std::string::npos);
		CHECK(text.find("5.1") != std::string::npos);
		CHECK(text.find("5.09999") == std::string::npos);
		CHECK(text.find("id =") == std::string::npos);
	}

	SECTION("Saved scene writes only relevant fields") {
		SceneDescription s;
		EmitterDesc cone;
		cone.id = 1;
		cone.shape = ConeShape{.pos = {1.f, 1.f}, .angle = 1.f};
		EmitterDesc line;
		line.id = 2;
		line.shape = LineShape{.from = {0.f, 0.f}, .to = {1.f, 1.f}}; // marker absent: not written
		s.emitters = {cone, line};
		auto path = tempPath("scene_fields.toml");
		REQUIRE(saveScene(s, path).has_value());
		const std::string text = readAll(path);
		CHECK(text.find("aim") == std::string::npos);
		CHECK(text.find("cooldown") == std::string::npos);
		CHECK(text.find("marker") == std::string::npos);
		CHECK(text.find("from") != std::string::npos);

		auto loaded = loadScene(path, kWorld);
		REQUIRE(loaded.has_value());
		CHECK(loaded->scene == s);
	}

	SECTION("Styles round trip") {
		StyleMap styles;
		Style lime;
		lime.trailFrom = *parseHexColor("#80ff00");
		lime.trailTo = *parseHexColor("#ffff00");
		lime.head = *parseHexColor("#80ff00");
		lime.width = 1.5f;
		lime.fade = Seconds{3.f};
		styles["lime"] = lime;
		Style magenta;
		magenta.trailFrom = magenta.trailTo = magenta.head = *parseHexColor("#ff00ff");
		magenta.width = 0.0125f;
		styles["magenta"] = magenta;

		auto path = tempPath("styles_roundtrip.toml");
		REQUIRE(saveStyles(styles, path).has_value());
		auto loaded = loadStyles(path);
		REQUIRE(loaded.has_value());
		CHECK(*loaded == styles);
		const std::string text = readAll(path);
		CHECK(text.find("0.0125") != std::string::npos);
		CHECK(text.find("0.01250000") == std::string::npos);
	}

	SECTION("Styles file errors") {
		auto bad = loadStyles(writeTemp("[styles.x]\nwidth = \"wide\""));
		REQUIRE_FALSE(bad.has_value());
		CHECK(bad.error().line == 2);
		CHECK(bad.error().field == "styles.x.width");
		CHECK_FALSE(loadStyles(tempPath("does_not_exist_styles.toml")).has_value());
		auto empty = loadStyles(writeTemp(""));
		REQUIRE(empty.has_value());
		CHECK(empty->empty());
	}

	SECTION("Local style overrides shared, unknown falls back") {
		StyleMap shared, local;
		shared["a"].width = 1.f;
		shared["b"].width = 2.f;
		local["a"].width = 9.f;

		CHECK(resolveStyle("a", local, shared).width == Approx(9.f));
		CHECK(resolveStyle("b", local, shared).width == Approx(2.f));

		std::vector<std::string> warnings;
		const Style &fallback = resolveStyle("nope", local, shared, &warnings);
		CHECK(fallback == Style{});
		CHECK_FALSE(fallback.fade.has_value());
		CHECK(warnings.size() == 1);
		CHECK(&resolveStyle("nope", local, shared) == &fallback);
		// the default name itself is never an error
		warnings.clear();
		(void) resolveStyle(kDefaultStyleName, local, shared, &warnings);
		CHECK(warnings.empty());
	}

	SECTION("Invalid TOML syntax reports line") {
		auto result = loadText("name = ");
		REQUIRE_FALSE(result.has_value());
		CHECK(result.error().line == 1);
		CHECK_FALSE(loadScene(tempPath("does_not_exist_scene.toml"), kWorld).has_value());
	}

	SECTION("Integer that a double cannot hold exactly is an error, not a crash") {
		checkError("[[emitter]]\ntype = \"cone\"\npos = [1, 1]\nspread = 9007199254740993", 4, "emitter[1].spread");
		checkError("[[emitter]]\ntype = \"cone\"\npos = [9007199254740993, 1]", 3, "emitter[1].pos");
	}

	SECTION("Cone with both aim and angle keeps aim and warns") {
		auto result = loadText("[[emitter]]\ntype = \"cone\"\npos = [1, 1]\naim = [0, 0]\nangle = 1.0");
		REQUIRE(result.has_value());
		REQUIRE(result->scene.emitters.size() == 1);
		CHECK(std::get<ConeShape>(result->scene.emitters[0].shape).aim.has_value());
		REQUIRE(result->warnings.size() == 1);
		CHECK(result->warnings[0].find("angle") != std::string::npos);
		CHECK(result->warnings[0].find("aim") != std::string::npos);

		// A line emitter uses angle as its direction and has no aim: no warning.
		auto line = loadText("[[emitter]]\ntype = \"line\"\nfrom = [0, 0]\nto = [0, 1]\nangle = 1.0");
		REQUIRE(line.has_value());
		CHECK(line->warnings.empty());
	}

	SECTION("loadStyles reports unknown keys when given a warnings vector") {
		const auto path = writeTemp("[styles.x]\nwidth = 2.0\nbogus = 1\n");
		std::vector<std::string> warnings;
		auto styles = loadStyles(path, &warnings);
		REQUIRE(styles.has_value());
		REQUIRE(warnings.size() == 1);
		CHECK(warnings[0].find("styles.x.bogus") != std::string::npos);
		CHECK(loadStyles(path).has_value()); // the warnings parameter is optional
	}
}

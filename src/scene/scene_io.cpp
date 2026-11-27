//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "scene/scene_io.h"

#include "toml/toml_schema.h"
#include "toml/toml_util.h"

namespace SceneModule {
	// ---- Styles and labels --------------------------------------------------------------------

	static const Schema<Style>& styleSchema() {
		static const Schema schema = {
			field<HexColor>("trail_from", &Style::trailFrom),
			field<HexColor>("trail_to", &Style::trailTo),
			field<HexColor>("head", &Style::head),
			field("width", &Style::width, positive),
			field("fade", &Style::fade, positive),
		};
		return schema;
	}

	static const Schema<LabelDesc>& labelSchema() {
		static const Schema schema = {
			field("pos", &LabelDesc::pos),
			field("text", &LabelDesc::text),
			field<HexColor>("color", &LabelDesc::color),
		};
		return schema;
	}

	// ---- Emitters -----------------------------------------------------------------------------

	// An emitter is a table with the fields of its shape and the fields every emitter has, side by
	// side; `type` says which shape. Each shape gets a full schema (type + shape + common fields),
	// so a key of another shape (e.g. `from` on a cone) produces an "unknown key" warning.

	// `type` is read by readEmitter before the schema is picked, so here it is only written.
	static FieldDesc<EmitterDesc> typeField(std::string type) {
		return {
			.key = "type",
			.read = [](const toml::node&, EmitterDesc&, const ReadContext&) {},
			.write = [type](const EmitterDesc&, toml::table& out) { out.insert_or_assign("type", type); },
		};
	}

	template<class Shape>
	static Schema<EmitterDesc> emitterSchema(std::string type, const Schema<Shape>& shapeSchema) {
		Schema<EmitterDesc> schema = {typeField(std::move(type))};
		const Schema<EmitterDesc> shape =
			embed<EmitterDesc>(shapeSchema, [](auto& e) -> auto& { return std::get<Shape>(e.shape); });
		schema.insert(schema.end(), shape.begin(), shape.end());
		schema.insert(schema.end(), {
			field("rays", &EmitterDesc::rays, atLeast<1>),
			field("start", &EmitterDesc::start, nonNegative),
			field("cooldown", &EmitterDesc::cooldown, positive),
			field("style", &EmitterDesc::style),
			field("marker", &EmitterDesc::marker),
		});
		return schema;
	}

	static const Schema<EmitterDesc>& schemaFor(const ConeShape&) {
		static const Schema<EmitterDesc> schema = emitterSchema<ConeShape>("cone", {
			required(field("pos", &ConeShape::pos)),
			field("aim", &ConeShape::aim),
			field("angle", &ConeShape::angle),
			field("spread", &ConeShape::spread, nonNegative),
		});
		return schema;
	}

	static const Schema<EmitterDesc>& schemaFor(const LineShape&) {
		static const Schema<EmitterDesc> schema = emitterSchema<LineShape>("line", {
			required(field("from", &LineShape::from)),
			required(field("to", &LineShape::to)),
			field("angle", &LineShape::angle),
		});
		return schema;
	}

	// A new EmitterShape alternative without a schemaFor overload does not compile.
	static const Schema<EmitterDesc>& schemaFor(const EmitterShape& shape) {
		return std::visit([](const auto& s) -> const Schema<EmitterDesc>& { return schemaFor(s); }, shape);
	}

	// "cone" | "line" -> an empty shape of that type.
	static std::expected<EmitterShape, std::string> shapeOfType(const toml::node& node) {
		auto type = Codec<std::string>::read(node);
		if (!type) {
			return std::unexpected(type.error());
		}
		if (*type == "cone") {
			return ConeShape{};
		}
		if (*type == "line") {
			return LineShape{};
		}
		return std::unexpected(R"(expected "cone" or "line")");
	}

	// Checks beyond single field values.
	struct ShapeChecker {
		const toml::table& table;
		const ReadContext& ctx;
		const WorldSettings& world;

		void insideWorld(std::string_view key, glm::vec2 point) const {
			if (glm::length(point) >= world.maxDistance) {
				ctx.fail(*table.get(key), key,
						"must be closer to the origin than world.max_distance ("
						+ std::to_string(world.maxDistance) + ")");
			}
		}

		void operator()(const ConeShape& cone) const {
			if (const toml::node* angle = table.get("angle"); angle && cone.aim) {
				ctx.warn(*angle, "angle", "ignored because 'aim' is set");
			}
			insideWorld("pos", cone.pos);
		}

		void operator()(const LineShape& line) const {
			insideWorld("from", line.from);
			insideWorld("to", line.to);
		}
	};

	static void readEmitter(
		const toml::table& table, int index, EmitterDesc& e, const ReadContext& ctx, const WorldSettings& world
	) {
		e.id = index;
		// The type picks the shape and so the schema; without one the emitter is a cone.
		if (const toml::node* node = table.get("type")) {
			auto shape = shapeOfType(*node);
			if (!shape) {
				return ctx.fail(*node, "type", std::move(shape.error()));
			}
			e.shape = std::move(*shape);
		}
		if (!readFields(table, schemaFor(e.shape), e, ctx)) {
			return;
		}
		std::visit(ShapeChecker{table, ctx, world}, e.shape);
	}

	static void writeEmitter(const EmitterDesc& e, toml::table& table) {
		writeFields(e, schemaFor(e.shape), table);
		if (const auto* cone = std::get_if<ConeShape>(&e.shape); cone && cone->aim) {
			table.erase("angle"); // ignored when aim is set
		}
	}

	// ---- Files --------------------------------------------------------------------------------

	// `world` is only used for reading (emitter positions are checked against it).
	static Schema<SceneDescription> sceneSchema(const WorldSettings& world) {
		return {
			field("name", &SceneDescription::name),
			field("description", &SceneDescription::description),
			tableArray(
				"emitter", &SceneDescription::emitters,
				[world](const toml::table& table, int index, EmitterDesc& e, const ReadContext& ctx) {
					readEmitter(table, index, e, ctx, world);
				},
				writeEmitter),
			tableArray("label", &SceneDescription::labels, labelSchema()),
			tableMap("styles", &SceneDescription::localStyles, styleSchema()),
		};
	}

	struct StylesFile {
		StyleMap styles;
	};

	static const Schema<StylesFile>& stylesFileSchema() {
		static const Schema schema = {
			tableMap("styles", &StylesFile::styles, styleSchema()),
		};
		return schema;
	}

	std::expected<LoadedScene, IoError> loadScene(const std::filesystem::path& path, const WorldSettings& world) {
		auto root = parseTomlFile(path);
		if (!root) {
			return std::unexpected(root.error());
		}

		ReadState state{.file = path.string()};
		auto scene = readTable(*root, sceneSchema(world), state);
		if (!scene) {
			return std::unexpected(scene.error());
		}
		return LoadedScene{
			.scene = std::move(*scene),
			.warnings = std::move(state.warnings),
		};
	}

	std::expected<void, IoError> saveScene(const SceneDescription& scene, const std::filesystem::path& path) {
		toml::table root;
		writeFields(scene, sceneSchema(WorldSettings{}), root);
		if (scene.description.empty()) {
			root.erase("description");
		}
		return writeTomlFile(root, path);
	}

	std::expected<StyleMap, IoError>
	loadStyles(const std::filesystem::path& path, std::vector<std::string>* warnings) {
		auto root = parseTomlFile(path);
		if (!root) {
			return std::unexpected(root.error());
		}
		ReadState state{.file = path.string()};
		auto read = readTable(*root, stylesFileSchema(), state);
		if (!read) {
			return std::unexpected(read.error());
		}
		if (warnings) {
			warnings->insert(warnings->end(), state.warnings.begin(), state.warnings.end());
		}
		return std::move(read->styles);
	}

	std::expected<void, IoError> saveStyles(const StyleMap& styles, const std::filesystem::path& path) {
		toml::table root;
		writeFields(StylesFile{styles}, stylesFileSchema(), root);
		return writeTomlFile(root, path);
	}
}

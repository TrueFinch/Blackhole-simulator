//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

// Declarative TOML (de)serialization: each struct is described once by a Schema, a list of
// fields; readFields/writeFields walk that list.
//
//   const Schema<Style> schema = {
//       field<HexColor>("head", &Style::head),
//       field("width", &Style::width, positive),
//       section("window", &Settings::window, windowSchema),   // nested table
//   };
//
// Reading keeps the diagnostics of the handwritten readers: every error and warning carries the
// file, the line of the offending node and the dotted field name ("window.width").

#include <algorithm>
#include <chrono>
#include <expected>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <glm/glm.hpp>
#include <toml++/toml.hpp>

#include "toml/io_error.h"

namespace SceneModule {
	// ---- Reading context ----------------------------------------------------------------------

	// Outcome of reading one file: the first error (reading stops there) and any warnings.
	struct ReadState {
		std::string file;
		std::optional<IoError> error;
		std::vector<std::string> warnings; // formatted with formatError
	};

	// Where in the file we are ("", "window", "emitter[2]", "styles.lime"). Cheap to copy;
	// all copies share one ReadState.
	class ReadContext {
	public:
		explicit ReadContext(ReadState& state, std::string where = {})
			: state_(&state), where_(std::move(where)) {}

		[[nodiscard]] ReadContext nested(std::string_view name) const {
			return ReadContext(*state_, fieldName(name));
		}

		[[nodiscard]] bool failed() const {
			return state_->error.has_value();
		}

		[[nodiscard]] const std::string& file() const {
			return state_->file;
		}

		[[nodiscard]] const std::string& where() const {
			return where_;
		}

		// "key" at the top level, "where.key" inside a table.
		[[nodiscard]] std::string fieldName(std::string_view key) const;

		// Records an error at `at` for `key`; only the first error is kept.
		void fail(const toml::node& at, std::string_view key, std::string message) const;

		void warn(const toml::node& at, std::string_view key, std::string message) const;

	private:
		ReadState* state_;
		std::string where_;
	};

	// ---- Codecs -------------------------------------------------------------------------------

	// How a C++ value looks in TOML:
	//   static std::expected<T, std::string> read(const toml::node &);  // error = message
	//   static <insertable into toml::table> write(const T &);
	// `field` picks Codec<member type> unless a codec is given explicitly (field<HexColor>(...)).
	template<class T>
	struct Codec;

	// TOML integer that fits into int.
	template<>
	struct Codec<int> {
		static std::expected<int, std::string> read(const toml::node& node);

		static std::int64_t write(int value) { return value; }
	};

	// Finite TOML integer or float (not bool), representable as a finite float.
	template<>
	struct Codec<float> {
		static std::expected<float, std::string> read(const toml::node& node);

		static double write(float value);
	};

	template<>
	struct Codec<bool> {
		static std::expected<bool, std::string> read(const toml::node& node);

		static bool write(bool value) { return value; }
	};

	template<>
	struct Codec<std::string> {
		static std::expected<std::string, std::string> read(const toml::node& node);

		static const std::string& write(const std::string& value) { return value; }
	};

	// A duration: a string of a number and a unit, "1.5s" or "500ms". A bare number is an error,
	// so a file never leaves the unit to guess. Written in ms below a second, else in s.
	template<>
	struct Codec<std::chrono::duration<float>> {
		static std::expected<std::chrono::duration<float>, std::string> read(const toml::node& node);

		static std::string write(std::chrono::duration<float> value);
	};

	// [x, y]: two finite numbers.
	template<>
	struct Codec<glm::vec2> {
		static std::expected<glm::vec2, std::string> read(const toml::node& node);

		static toml::array write(glm::vec2 value);
	};

	// glm::vec3 has no default codec on purpose: a color and a 3D vector look different in TOML.
	// "#rrggbb" <-> color with components in [0, 1].
	struct HexColor {
		static std::expected<glm::vec3, std::string> read(const toml::node& node);

		static std::string write(glm::vec3 value);
	};

	// ---- Checks -------------------------------------------------------------------------------

	// A value check: nullopt if fine, otherwise the message ("must be positive").
	template<class V>
	using Check = std::optional<std::string> (*)(const V&);

	template<class T>
	std::optional<std::string> positive(const T& value) {
		if (value > T{}) {
			return std::nullopt;
		}
		return "must be positive";
	}

	template<class T>
	std::optional<std::string> nonNegative(const T& value) {
		if (value >= T{}) {
			return std::nullopt;
		}
		return "must not be negative";
	}

	template<auto Min, class T>
	std::optional<std::string> atLeast(const T& value) {
		if (value >= static_cast<T>(Min)) {
			return std::nullopt;
		}
		return "must be at least " + std::to_string(Min);
	}

	// ---- Fields and schemas -------------------------------------------------------------------

	template<class S>
	struct FieldDesc {
		std::string key;
		bool required = false;
		std::function<void(const toml::node&, S&, const ReadContext&)> read;
		std::function<void(const S&, toml::table&)> write;
	};

	template<class S>
	using Schema = std::vector<FieldDesc<S>>;

	// Reads the fields of `schema` present in `table` into `out`. Absent keys keep their values in
	// `out` (so defaults come from the struct); absent required keys are an error; keys not in
	// the schema produce an "unknown key" warning. Does nothing once ctx has failed.
	// Returns false if ctx has failed (the error is in its ReadState; `out` may be partly filled).
	// Prefer readTable for a whole file; use readFields when `out` is a part of a bigger object.
	template<class S>
	[[nodiscard]] bool readFields(const toml::table& table, const Schema<S>& schema, S& out, const ReadContext& ctx);

	// Writes every field of `schema` (an empty std::optional is skipped).
	template<class S>
	void writeFields(const S& in, const Schema<S>& schema, toml::table& out);

	namespace detail {
		template<class T>
		struct OptionalTraits {
			using value_type = T;
			static constexpr bool isOptional = false;
		};

		template<class T>
		struct OptionalTraits<std::optional<T>> {
			using value_type = T;
			static constexpr bool isOptional = true;
		};
	}

	// Value type of member: T for both T and std::optional<T>.
	template<class M>
	using FieldValue = detail::OptionalTraits<M>::value_type;

	// A plain field. For std::optional<T> members: absent key -> nullopt, nullopt -> not written,
	// and `check` applies to the contained T.
	template<class C = void, class S, class M>
	FieldDesc<S> field(std::string key, M S::* member, Check<FieldValue<M>> check = nullptr) {
		using Codec_ = std::conditional_t<std::is_void_v<C>, Codec<FieldValue<M>>, C>;

		FieldDesc<S> desc{.key = key};
		desc.read = [key, member, check](const toml::node& node, S& out, const ReadContext& ctx) {
			auto value = Codec_::read(node);
			if (!value) {
				return ctx.fail(node, key, std::move(value.error()));
			}
			if (check) {
				if (auto problem = check(*value); problem.has_value()) {
					return ctx.fail(node, key, std::move(*problem));
				}
			}
			out.*member = std::move(*value);
		};
		desc.write = [key, member](const S& in, toml::table& out) {
			if constexpr (detail::OptionalTraits<M>::isOptional) {
				if (in.*member) {
					out.insert_or_assign(key, Codec_::write(*(in.*member)));
				}
			} else {
				out.insert_or_assign(key, Codec_::write(in.*member));
			}
		};
		return desc;
	}

	// Marks a field as required: a missing key is an error ("is required").
	template<class S>
	FieldDesc<S> required(FieldDesc<S> desc) {
		desc.required = true;
		return desc;
	}

	// A nested table described by its own schema. `schema` must outlive the returned descriptor.
	template<class S, class Sub>
	FieldDesc<S> section(const std::string& key, Sub S::* member, const Schema<Sub>& schema) {
		const Schema<Sub>* sub = &schema;
		FieldDesc<S> desc{.key = key};
		desc.read = [key, member, sub](const toml::node& node, S& out, const ReadContext& ctx) {
			const toml::table* table = node.as_table();
			if (!table) {
				return ctx.fail(node, key, "expected a table");
			}
			// On failure the error is already in ctx; the caller's readFields reports it.
			static_cast<void>(readFields(*table, *sub, out.*member, ctx.nested(key)));
		};
		desc.write = [key, member, sub](const S& in, toml::table& out) {
			toml::table table;
			writeFields(in.*member, *sub, table);
			out.insert_or_assign(key, std::move(table));
		};
		return desc;
	}

	// The fields of a part of S, read from and written to the same table as S's own fields (no
	// nested table). `part` maps S& to Sub& and const S& to const Sub&, e.g.
	//   [](auto &e) -> auto & { return std::get<ConeShape>(e.shape); }
	template<class S, class Sub, class Part>
	Schema<S> embed(const Schema<Sub>& schema, Part part) {
		Schema<S> result;
		result.reserve(schema.size());
		for (const FieldDesc<Sub>& f: schema) {
			result.push_back({
				.key = f.key,
				.required = f.required,
				.read = [read = f.read, part](const toml::node& node, S& out, const ReadContext& ctx) {
					read(node, part(out), ctx);
				},
				.write = [write = f.write, part](const S& in, toml::table& out) { write(part(in), out); },
			});
		}
		return result;
	}

	// Reads one element of a table array or table map into `out` (which starts default-constructed).
	// `index` is 1-based for arrays and 0 for maps. Problems are reported only through ctx.
	template<class E>
	using ElementReader = std::function<void(const toml::table&, int index, E&, const ReadContext&)>;
	template<class E>
	using ElementWriter = std::function<void(const E&, toml::table&)>;

	// An array of tables ([[key]] in TOML) with a custom element reader/writer. Elements are
	// named "key[1]", "key[2]"... in diagnostics. An empty vector is not written.
	template<class S, class E>
	FieldDesc<S> tableArray(const std::string& key, std::vector<E> S::* member,
							std::type_identity_t<ElementReader<E>> readElement,
							std::type_identity_t<ElementWriter<E>> writeElement) {
		FieldDesc<S> desc{.key = key};
		desc.read = [key, member, readElement](const toml::node& node, S& out, const ReadContext& ctx) {
			const toml::array* array = node.as_array();
			if (!array) {
				return ctx.fail(node, key, "expected an array of tables ([[" + key + "]])");
			}
			std::vector<E> result;
			for (size_t i = 0; i < array->size(); ++i) {
				const std::string name = key + "[" + std::to_string(i + 1) + "]";
				const toml::table* table = array->get(i)->as_table();
				if (!table) {
					return ctx.fail(*array->get(i), name, "expected a table");
				}
				E element{};
				readElement(*table, static_cast<int>(i + 1), element, ctx.nested(name));
				if (ctx.failed()) {
					return;
				}
				result.push_back(std::move(element));
			}
			out.*member = std::move(result);
		};
		desc.write = [key, member, writeElement](const S& in, toml::table& out) {
			if ((in.*member).empty()) {
				return;
			}
			toml::array array;
			for (const E& element: in.*member) {
				toml::table table;
				writeElement(element, table);
				array.push_back(std::move(table));
			}
			out.insert_or_assign(key, std::move(array));
		};
		return desc;
	}

	// An array of tables whose elements are described by `schema` (which must outlive the result).
	template<class S, class E>
	FieldDesc<S> tableArray(const std::string& key, std::vector<E> S::* member, const Schema<E>& schema) {
		const Schema<E>* sub = &schema;
		return tableArray<S, E>(
			key, member,
			[sub](const toml::table& table, int, E& out, const ReadContext& ctx) {
				static_cast<void>(readFields(table, *sub, out, ctx));
			},
			[sub](const E& in, toml::table& out) { writeFields(in, *sub, out); });
	}

	// A table of named sub-tables ([key.<name>] in TOML), each described by `schema` (which must
	// outlive the result). Elements are named "key.<name>". An empty map is not written.
	template<class S, class E>
	FieldDesc<S> tableMap(const std::string& key, std::map<std::string, E> S::* member, const Schema<E>& schema) {
		const Schema<E>* sub = &schema;
		FieldDesc<S> desc{.key = key};
		desc.read = [key, member, sub](const toml::node& node, S& out, const ReadContext& ctx) {
			const toml::table* table = node.as_table();
			if (!table) {
				return ctx.fail(node, key, "expected a table");
			}
			const ReadContext inner = ctx.nested(key);
			std::map<std::string, E> result;
			for (const auto& [name, value]: *table) {
				const toml::table* element = value.as_table();
				if (!element) {
					return inner.fail(value, name.str(), "expected a table");
				}
				E e{};
				if (!readFields(*element, *sub, e, inner.nested(name.str()))) {
					return;
				}
				result.emplace(std::string(name.str()), std::move(e));
			}
			out.*member = std::move(result);
		};
		desc.write = [key, member, sub](const S& in, toml::table& out) {
			if ((in.*member).empty()) {
				return;
			}
			toml::table table;
			for (const auto& [name, element]: in.*member) {
				toml::table elementTable;
				writeFields(element, *sub, elementTable);
				table.insert_or_assign(name, std::move(elementTable));
			}
			out.insert_or_assign(key, std::move(table));
		};
		return desc;
	}

	template<class S>
	bool readFields(const toml::table& table, const Schema<S>& schema, S& out, const ReadContext& ctx) {
		for (const FieldDesc<S>& f: schema) {
			if (const toml::node* node = table.get(f.key)) {
				// Field readers report problems only through ctx, so check it after each one.
				f.read(*node, out, ctx);
				if (ctx.failed()) {
					return false;
				}
			} else if (f.required) {
				ctx.fail(table, f.key, "is required");
				return false;
			}
		}
		for (const auto& [key, value]: table) {
			const bool known = std::ranges::any_of(schema, [&](const FieldDesc<S>& f) {
				return f.key == key.str();
			});
			if (!known) {
				ctx.warn(value, key.str(), "unknown key");
			}
		}
		return true;
	}

	// Reads `table` as a whole S, starting from `initial` (defaults by default). On failure returns
	// the first error; either way, warnings are collected in `state.warnings`.
	template<class S>
	[[nodiscard]] std::expected<S, IoError>
	readTable(const toml::table& table, const Schema<S>& schema, ReadState& state, S initial = S{}) {
		if (!readFields(table, schema, initial, ReadContext(state))) {
			return std::unexpected(*state.error);
		}
		return initial;
	}

	template<class S>
	void writeFields(const S& in, const Schema<S>& schema, toml::table& out) {
		for (const FieldDesc<S>& f: schema) {
			f.write(in, out);
		}
	}
}

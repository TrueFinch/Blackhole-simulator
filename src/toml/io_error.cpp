//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "toml/io_error.h"

namespace SceneModule {
	std::string formatError(const IoError& error) {
		std::string text = error.file;
		if (error.line > 0) {
			text += ":" + std::to_string(error.line);
		}
		text += ": ";
		if (!error.field.empty()) {
			text += error.field + ": ";
		}
		return text + error.message;
	}
}

//
// Created by Vladimir Glushkov on 26.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <string>

namespace SceneModule {
	// An error (or warning) found while reading or writing a data file.
	struct IoError {
		std::string file;
		int line = 0; // 0 = unknown
		std::string field;
		std::string message;
	};

	// "file:line: field: message" (line and field omitted when unknown)
	[[nodiscard]] std::string formatError(const IoError& error);
}

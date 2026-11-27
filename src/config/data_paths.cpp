//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "config/data_paths.h"

SceneModule::DataPaths SceneModule::dataPathsFromArgs(int argc, char** argv) {
	if (argc > 1) {
		return {argv[1]};
	}
	return {BLACKHOLE_DATA_DIR};
}

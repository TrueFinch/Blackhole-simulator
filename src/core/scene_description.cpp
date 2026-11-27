//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "core/scene_description.h"

namespace SceneModule {
	const Style& resolveStyle(const std::string& name, const StyleMap& local, const StyleMap& shared,
	                          std::vector<std::string>* warnings) {
		if (auto it = local.find(name); it != local.end()) {
			return it->second;
		}
		if (auto it = shared.find(name); it != shared.end()) {
			return it->second;
		}
		if (warnings && name != kDefaultStyleName) {
			warnings->push_back("unknown style '" + name + "', using the default style");
		}
		static const Style defaultStyle;
		return defaultStyle;
	}
}

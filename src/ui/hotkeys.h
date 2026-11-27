//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include "ui/app_controller.h"

namespace UiModule {
	// R restart, Shift+R reload from disk, C clear, Space pause, Left/Right step back/forward.
	// Does nothing while a widget wants the keyboard.
	void handleHotkeys(AppController& controller, bool imguiWantsKeyboard);
} // namespace UiModule

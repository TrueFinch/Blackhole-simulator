//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include "ui/app_controller.h"

namespace UiModule {
	// "Управление" window: scenario choice, restart/clear/pause, stepping, seek, error and warning text
	void drawControlPanel(AppController& controller);
} // namespace UiModule

//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "ui/hotkeys.h"

#include <imgui.h>

void UiModule::handleHotkeys(AppController& controller, bool imguiWantsKeyboard) {
	if (imguiWantsKeyboard) {
		return;
	}
	if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
		if (ImGui::GetIO().KeyShift) {
			controller.reloadFromDisk();
		} else {
			controller.restart();
		}
	}
	if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
		controller.clearRays();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
		controller.togglePause();
	}
	// Holding the arrows keeps stepping
	if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow, true)) {
		controller.stepBack();
	}
	if (ImGui::IsKeyPressed(ImGuiKey_RightArrow, true)) {
		controller.stepForward();
	}
}

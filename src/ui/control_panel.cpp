//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "ui/control_panel.h"

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>
#include <imgui.h>

namespace {
	constexpr ImVec4 kErrorColor{1.f, 0.35f, 0.35f, 1.f};
	constexpr ImVec4 kWarningColor{1.f, 0.85f, 0.3f, 1.f};
	constexpr float kPanelMargin = 8.f;
	constexpr float kPanelWidth = 300.f;
	constexpr std::size_t kWarningsCollapseThreshold = 3;

	// Button with a tooltip naming its hotkey; true when clicked
	bool buttonWithKey(const char* label, const char* key) {
		const bool clicked = ImGui::Button(label);
		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("%s", key);
		}
		return clicked;
	}

	void drawScenarioCombo(UiModule::AppController& controller) {
		const std::string& current = controller.currentScenario();
		ImGui::SetNextItemWidth(-1.f);
		if (ImGui::BeginCombo("##scenario", current.empty() ? "(нет сценария)" : current.c_str())) {
			const std::vector<std::string> names = controller.scenarioNames();
			for (const std::string& name: names) {
				if (ImGui::Selectable(name.c_str(), name == current) && name != current) {
					controller.selectScenario(name);
				}
			}
			ImGui::EndCombo();
		}
	}

	void drawMessages(const UiModule::AppController& controller) {
		for (const std::string& error: controller.configErrors()) {
			ImGui::TextColored(kErrorColor, "%s", error.c_str());
		}
		if (controller.lastError()) {
			ImGui::TextColored(kErrorColor, "%s", controller.lastError()->c_str());
		}
		const std::vector<std::string>& warnings = controller.warnings();
		if (warnings.empty()) {
			return;
		}
		const auto drawWarnings = [&] {
			for (const std::string& warning: warnings) {
				ImGui::TextColored(kWarningColor, "%s", warning.c_str());
			}
		};
		if (warnings.size() > kWarningsCollapseThreshold) {
			const std::string header = "Предупреждения (" + std::to_string(warnings.size()) + ")";
			if (ImGui::CollapsingHeader(header.c_str())) {
				drawWarnings();
			}
		} else {
			drawWarnings();
		}
	}
} // namespace

void UiModule::drawControlPanel(AppController& controller) {
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(
		{viewport->WorkPos.x + viewport->WorkSize.x - kPanelMargin, viewport->WorkPos.y + kPanelMargin},
		ImGuiCond_FirstUseEver,
		{1.f, 0.f}
	);
	ImGui::SetNextWindowSize({kPanelWidth, 0.f}, ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Управление", nullptr, ImGuiWindowFlags_NoSavedSettings)) {
		drawScenarioCombo(controller);

		if (buttonWithKey("Перезапуск", "R")) {
			controller.restart();
		}
		ImGui::SameLine();
		if (buttonWithKey("Сбросить и перезапустить", "Shift+R")) {
			controller.reloadFromDisk();
		}
		if (buttonWithKey("Очистить", "C")) {
			controller.clearRays();
		}
		ImGui::SameLine();
		if (buttonWithKey(controller.paused() ? "Продолжить" : "Пауза", "Пробел")) {
			controller.togglePause();
		}

		if (buttonWithKey("◀ шаг", "←")) {
			controller.stepBack();
		}
		ImGui::SameLine();
		if (buttonWithKey("шаг ▶", "→")) {
			controller.stepForward();
		}

		static int seekStep = 0;
		ImGui::SetNextItemWidth(100.f);
		ImGui::InputInt("Кадр", &seekStep, 0);
		ImGui::SameLine();
		if (ImGui::Button("Перейти")) {
			controller.seek(std::max(0, seekStep));
		}
		ImGui::Text("Текущий кадр: %d", controller.simulation().stepIndex());

		drawMessages(controller);
	}
	ImGui::End();
}

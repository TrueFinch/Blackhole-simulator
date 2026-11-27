//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "ui/overlay.h"

#include <imgui.h>

namespace {
	constexpr float kOverlayMargin = 8.f;
	constexpr float kOverlayAlpha = 0.45f;
	constexpr ImVec2 kLabelOffset{6.f, -8.f}; // pixels from the anchor point
} // namespace

void UiModule::drawOverlay(const FrameStats& stats) {
	constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
										ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
										ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowPos({kOverlayMargin, kOverlayMargin});
	ImGui::SetNextWindowBgAlpha(kOverlayAlpha);
	if (ImGui::Begin("##overlay", nullptr, flags)) {
		ImGui::Text("FPS: %.0f", stats.fps);
		ImGui::Text("Frame: %.2f ms", stats.frameMs);
		ImGui::Text("Step: %d", stats.step);
		ImGui::Text("Time: %.2f s", stats.simTime.count());
		ImGui::Text("Rays: %d", stats.liveRays);
		if (stats.rayLimitHit) {
			ImGui::TextColored({1.f, 0.25f, 0.25f, 1.f}, "Ray limit reached");
		}
	}
	ImGui::End();
}

void UiModule::drawLabels(const SceneModule::SceneDescription& scene, const ViewModule::ViewFit& fit,
						glm::ivec2 fb) {
	const ImGuiIO& io = ImGui::GetIO();
	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	for (const auto& label: scene.labels) {
		// worldToScreen is in framebuffer pixels, ImGui works in display coordinates
		const glm::vec2 px = ViewModule::worldToScreen(label.pos, fit, fb.x, fb.y);
		const ImVec2 pos{
			px.x / io.DisplayFramebufferScale.x + kLabelOffset.x,
			px.y / io.DisplayFramebufferScale.y + kLabelOffset.y
		};
		const ImU32 color = ImGui::ColorConvertFloat4ToU32({label.color.r, label.color.g, label.color.b, 1.f});
		drawList->AddText(pos, color, label.text.c_str());
	}
}

//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#include "ui/imgui_layer.h"

#include <iostream>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl2.h>

namespace {
	constexpr float kFontSize = 16.f;

	// Latin and Cyrillic as in GetGlyphRangesCyrillic(), plus the arrows used in the control panel.
	// ImGui keeps the pointer until the font atlas is built, so the array is static.
	constexpr ImWchar kGlyphRanges[] = {
		0x0020, 0x00FF, // Basic Latin + Latin-1 Supplement
		0x0400, 0x052F, // Cyrillic + Cyrillic Supplement
		0x2190, 0x2193, // ← ↑ → ↓
		0x25B6, 0x25B6, // ▶
		0x25C0, 0x25C0, // ◀
		0x2DE0, 0x2DFF, // Cyrillic Extended-A
		0xA640, 0xA69F, // Cyrillic Extended-B
		0,
	};
} // namespace

UiModule::ImGuiLayer::ImGuiLayer(GLFWwindow* window, const std::filesystem::path& fontPath) {
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr; // no imgui.ini next to the executable
	ImGui::StyleColorsDark();

	std::error_code ec;
	if (std::filesystem::exists(fontPath, ec)) {
		if (io.Fonts->AddFontFromFileTTF(fontPath.string().c_str(), kFontSize, nullptr,
										kGlyphRanges) == nullptr) {
			std::cerr << "Failed to load the UI font: " << fontPath.string() << ", using the default font\n";
			io.Fonts->AddFontDefault();
		}
	} else {
		std::cerr << "UI font not found: " << fontPath.string() << ", using the default font\n";
		io.Fonts->AddFontDefault();
	}

	// install_callbacks = true: chains the callbacks already set on the window
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL2_Init();
}

UiModule::ImGuiLayer::~ImGuiLayer() {
	ImGui_ImplOpenGL2_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void UiModule::ImGuiLayer::beginFrame() { // NOLINT(*-convert-member-functions-to-static)
	ImGui_ImplOpenGL2_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void UiModule::ImGuiLayer::endFrame() { // NOLINT(*-convert-member-functions-to-static)
	ImGui::Render();
	ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
}

bool UiModule::ImGuiLayer::wantsKeyboard() const { // NOLINT(*-convert-member-functions-to-static)
	const ImGuiIO& io = ImGui::GetIO();
	return io.WantCaptureKeyboard || io.WantTextInput;
}

float UiModule::ImGuiLayer::framerate() const { // NOLINT(*-convert-member-functions-to-static)
	return ImGui::GetIO().Framerate;
}

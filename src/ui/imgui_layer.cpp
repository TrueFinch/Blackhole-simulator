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
										io.Fonts->GetGlyphRangesCyrillic()) == nullptr) {
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

//
// Created by Vladimir Glushkov on 27.11.2026.
// Co-Authored-By: Claude Opus 5.5
//

#pragma once

#include <filesystem>

struct GLFWwindow;

namespace UiModule {
	// RAII wrapper over the Dear ImGui context with the GLFW + OpenGL2 backends.
	// Per frame: beginFrame(), widgets, endFrame() (renders the draw data, call before swapping buffers).
	class ImGuiLayer {
	public:
		// Loads the font with Cyrillic glyphs; falls back to the default font if the file is missing
		ImGuiLayer(GLFWwindow* window, const std::filesystem::path& fontPath);

		ImGuiLayer(const ImGuiLayer&) = delete;

		ImGuiLayer(ImGuiLayer&&) = delete;

		ImGuiLayer& operator=(const ImGuiLayer&) = delete;

		ImGuiLayer& operator=(ImGuiLayer&&) = delete;

		~ImGuiLayer();

		void beginFrame();

		void endFrame();

		// True while a widget wants keyboard input, so the app should not handle hotkeys
		[[nodiscard]] bool wantsKeyboard() const;
	};
} // namespace UiModule

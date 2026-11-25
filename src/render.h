//
// Created by Vladimir Glushkov on 25.11.2026.
//

#pragma once

#include <memory>
#include <span>
#include <string_view>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

struct GLFWwindow;

namespace RenderModule {
	using Color = glm::vec4; // RGBA, each component in 0..1

	struct ColoredPoint {
		glm::vec2 pos;
		Color color;
	};

	// Low-level rendering backend: owns the window and the OpenGL context and
	// draws generic primitives in world coordinates. Knows nothing about
	// the simulation and its objects.
	class RenderEngine {
	public:
		RenderEngine(int width, int height, std::string_view title);

		RenderEngine(const RenderEngine&) = delete;

		RenderEngine(RenderEngine&&) = delete;

		RenderEngine& operator=(const RenderEngine&) = delete;

		RenderEngine& operator=(RenderEngine&&) = delete;

		~RenderEngine();

		// Clear the screen and map [-halfWidth, halfWidth] x [-halfHeight, halfHeight] onto the window
		void beginFrame(float halfWidth, float halfHeight);

		// Swap buffers and poll events
		void endFrame() const;

		// Check if window should close
		[[nodiscard]] bool shouldClose() const;

		// Seconds since the engine was created
		[[nodiscard]] static float time();

		void drawFilledCircle(glm::vec2 center, float radius, const Color& color, int segments = 100);

		void drawCircleOutline(glm::vec2 center, float radius, const Color& color, float lineWidth,
								int segments = 100);

		// Draws every other segment of the circle
		void drawDashedCircle(glm::vec2 center, float radius, const Color& color, float lineWidth,
							int segments = 100);

		void drawPoint(glm::vec2 pos, float size, const Color& color);

		void drawPoints(std::span<const ColoredPoint> points, float size);

		// Polyline whose alpha goes linearly from alphaStart (first vertex) to alphaEnd (last vertex)
		void drawFadingLineStrip(std::span<const glm::vec2> vertices, const glm::vec3& rgb, float alphaStart,
								float alphaEnd, float lineWidth);

	private:
		static constexpr Color kBackgroundColor{0.06f, 0.06f, 0.08f, 1.0f};

		struct GlfwContext {
			GlfwContext();

			GlfwContext(const GlfwContext&) = delete;

			GlfwContext& operator=(const GlfwContext&) = delete;

			~GlfwContext();
		};

		// Declared before m_window so glfwTerminate runs after the window is destroyed
		// Do not change order
		GlfwContext m_glfw;
		std::unique_ptr<GLFWwindow, void(*)(GLFWwindow*)> m_window;
	};
} // namespace RenderModule

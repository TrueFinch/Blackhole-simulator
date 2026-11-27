//
// Created by Vladimir Glushkov on 25.11.2026.
//

#include "render/render.h"

#include <cmath>
#include <iostream>
#include <stdexcept>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "core/constants.h"

namespace {
	glm::vec2 pointOnCircle(glm::vec2 center, float radius, int i, int segments) {
		const auto angle = static_cast<float>(i) * 2.0f * PhysicsModule::kPi / static_cast<float>(segments);
		return {center.x + radius * std::cos(angle), center.y + radius * std::sin(angle)};
	}

	void setColor(const RenderModule::Color& color) {
		glColor4f(color.r, color.g, color.b, color.a);
	}
} // namespace

RenderModule::RenderEngine::GlfwContext::GlfwContext() {
	glfwSetErrorCallback([](const int error_code, const char* error_message) {
		std::cerr << "GLFW error: " << error_code << ": " << error_message << std::endl;
	});
	if (glfwInit() == GLFW_FALSE) {
		throw std::runtime_error("GLFW initialization failed");
	}
}

RenderModule::RenderEngine::GlfwContext::~GlfwContext() {
	glfwTerminate();
}

RenderModule::RenderEngine::RenderEngine(int width, int height, std::string_view title)
	: m_window(nullptr, &glfwDestroyWindow) {
	m_window.reset(glfwCreateWindow(width, height, title.data(), nullptr, nullptr));

	if (!m_window) {
		throw std::runtime_error("GLFW window creation failed");
	}

	// Make the OpenGL context current for this window
	// Only one context can be current per thread
	// This binds all OpenGL call to this specific window
	glfwMakeContextCurrent(m_window.get());

	// Enable VSync to limit frame rate to monitor refresh rate
	// glfwSwapInterval sets minimum count of the screen updates
	// between buffers swap
	glfwSwapInterval(1);

	// Set glewExperimental to GL_TRUE first
	glewExperimental = GL_TRUE;
	// Then call glewInit and check if it returns GLEW_OK
	if (const auto err_code = glewInit(); err_code != GLEW_OK) {
		std::cerr << "GLEW error: " << glewGetErrorString(err_code) << '\n';
		throw std::runtime_error("GLEW initialization failed");
	}

	// Sets the rendering area within the window
	int fbWidth, fbHeight;
	glfwGetFramebufferSize(m_window.get(), &fbWidth, &fbHeight);
	glViewport(0, 0, fbWidth, fbHeight);

	// todo move parameter to settings
	// Enable visual polish features (GL_LINE_SMOOTH, GL_BLEND, GL_NICEST hint)
	// Generates partially transparent edge pixels
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	// GL_BLEND with GL_SRC_ALPHA blends those pixels with the background,
	// it also makes the fading ray trails transparent
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

RenderModule::RenderEngine::~RenderEngine() = default;

void RenderModule::RenderEngine::beginFrame(float halfWidth, float halfHeight) { // NOLINT(*-convert-member-functions-to-static)
	// Clear screen with the background color
	glClearColor(kBackgroundColor.r, kBackgroundColor.g, kBackgroundColor.b, kBackgroundColor.a);
	glClear(GL_COLOR_BUFFER_BIT);

	// todo glMatrixMode, glOrtho, glBegin/glVertex/glColor is legacy API -> replace later
	// Map world coordinates onto the viewport
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void RenderModule::RenderEngine::endFrame() const {
	// Swap buffers (double buffering)
	glfwSwapBuffers(m_window.get());
	// Process the user input and window events
	glfwPollEvents();
}

bool RenderModule::RenderEngine::shouldClose() const {
	return glfwWindowShouldClose(m_window.get()) == GLFW_TRUE;
}

float RenderModule::RenderEngine::time() {
	return static_cast<float>(glfwGetTime());
}

void RenderModule::RenderEngine::drawFilledCircle( // NOLINT(*-convert-member-functions-to-static)
	glm::vec2 center, float radius, const Color& color, int segments
) {
	setColor(color);
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(center.x, center.y);
	for (auto i = 0; i <= segments; ++i) {
		const auto p = pointOnCircle(center, radius, i, segments);
		glVertex2f(p.x, p.y);
	}
	glEnd();
}

void RenderModule::RenderEngine::drawCircleOutline( // NOLINT(*-convert-member-functions-to-static)
	glm::vec2 center, float radius, const Color& color, float lineWidth, int segments
) {
	setColor(color);
	glLineWidth(lineWidth);
	glBegin(GL_LINE_LOOP);
	for (auto i = 0; i < segments; ++i) {
		const auto p = pointOnCircle(center, radius, i, segments);
		glVertex2f(p.x, p.y);
	}
	glEnd();
}

void RenderModule::RenderEngine::drawDashedCircle( // NOLINT(*-convert-member-functions-to-static)
	glm::vec2 center, float radius, const Color& color, float lineWidth, int segments
) {
	setColor(color);
	glLineWidth(lineWidth);
	glBegin(GL_LINES);
	// Draw every other segment to create dashes
	for (auto i = 0; i < segments; i += 2) {
		const auto p1 = pointOnCircle(center, radius, i, segments);
		const auto p2 = pointOnCircle(center, radius, i + 1, segments);
		glVertex2f(p1.x, p1.y);
		glVertex2f(p2.x, p2.y);
	}
	glEnd();
}

void RenderModule::RenderEngine::drawPoint(glm::vec2 pos, float size, const Color& color) {
	const ColoredPoint point{pos, color};
	drawPoints({&point, 1}, size);
}

void RenderModule::RenderEngine::drawPoints(std::span<const ColoredPoint> points, float size) { // NOLINT(*-convert-member-functions-to-static)
	glPointSize(size);
	glBegin(GL_POINTS);
	for (const auto& [pos, color]: points) {
		setColor(color);
		glVertex2f(pos.x, pos.y);
	}
	glEnd();
}

void RenderModule::RenderEngine::drawLineStrip(std::span<const ColoredPoint> vertices, float lineWidth) { // NOLINT(*-convert-member-functions-to-static)
	if (vertices.size() < 2) {
		return;
	}
	glLineWidth(lineWidth);
	glBegin(GL_LINE_STRIP);
	for (const auto& [pos, color]: vertices) {
		setColor(color);
		glVertex2f(pos.x, pos.y);
	}
	glEnd();
}

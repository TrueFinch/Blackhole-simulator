//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <cmath>
#include <iostream>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
	// automatically calls glfwTerminate in destructor
	struct GLFWTerminateCaller {
		GLFWTerminateCaller() = default;

		GLFWTerminateCaller(const GLFWTerminateCaller&) = delete;

		GLFWTerminateCaller(GLFWTerminateCaller&&) = delete;

		GLFWTerminateCaller& operator=(const GLFWTerminateCaller&) = delete;

		GLFWTerminateCaller& operator=(GLFWTerminateCaller&&) = delete;

		~GLFWTerminateCaller() {
			glfwTerminate();
		}
	};
}

void drawCircle(float center_x, float center_y, float radius, int segments) {
	glBegin(GL_TRIANGLE_FAN);
	glVertex2f(center_x, center_y);

	for (auto i = 0; i <= segments; ++i) {
		const auto angle = (static_cast<float>(i) * 2.0f * static_cast<float>(M_PI)) / static_cast<float>(segments);
		const auto px = center_x + radius * std::cos(angle);
		const auto py = center_y + radius * std::sin(angle);
		glVertex2f(px, py);
	}

	glEnd();
}

int main() {
	glfwSetErrorCallback([](const int error_code, const char* error_message) {
		std::cerr << "GLFW error: " << error_code << ": " << error_message << std::endl;
	});
	if (glfwInit() == GLFW_FALSE) {
		std::cerr << "GLFW error: initialization failed" << std::endl;
		return 1;
	}

	// destroyed last before program exit
	GLFWTerminateCaller guard;
	constexpr int kWidth = 800;
	constexpr int kHeight = 600;
	std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window{
		glfwCreateWindow(kWidth, kHeight, "Black hole simulator", nullptr, nullptr),
		&glfwDestroyWindow
	};
	if (!window) {
		std::cerr << "GLFW error: window creation failed" << std::endl;
		return 1;
	}

	// Make the OpenGL context current for this window
	// Only one context can be current per thread
	// This binds all OpenGL call to this specific window
	glfwMakeContextCurrent(window.get());


	// Enable VSync to limit frame rate to monitor refresh rate
	// glfwSwapInterval sets minimum count of the screen updates
	// between buffers swap
	glfwSwapInterval(1);

	// Set glewExperimental to GL_TRUE first
	glewExperimental = GL_TRUE;
	// Then call glewInit and check if it returns GLEW_OK
	if (const auto err_code = glewInit(); err_code != GLEW_OK) {
		std::cerr << "GLEW error: " << glewGetErrorString(err_code) << '\n';
		return 1;
	}

	// frame buffer size
	int fbWidth, fbHeight;
	// Sets the rendering area within the window
	{
		glfwGetFramebufferSize(window.get(), &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);
	}

	// Set up projection matrix with glOrtho()
	{
		// todo glMatrixMode, glOrtho, glBegin/glVertex/glColor is legacy API -> replace later
		// Switch to projection mode
		glMatrixMode(GL_PROJECTION);
		// Reset matrix
		glLoadIdentity();
		// Set orthographic projection from -10 to 10 in X and Y
		const double aspect = double(fbWidth) / fbHeight;
		glOrtho(-10.0 * aspect, 10.0 * aspect, -10.0, 10.0, -1.0, 1.0);

		// Switch back to modelview matrix
		glMatrixMode(GL_MODELVIEW);
		// Reset matrix
		glLoadIdentity();
	}

	// render loop
	while (!glfwWindowShouldClose(window.get())) {
		// 1. clear screen with the background color (RGBA, each component in 0..1)
		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);


		// Large blue circle at origin
		glColor3f(0.2f, 0.4f, 0.8f);
		drawCircle(0.0f, 0.0f, 3.0f, 100);
		// Small orange circle to the right
		glColor3f(1.0f, 0.5f, 0.0f);
		drawCircle(5.0f, 0.0f, 1.5f, 100);

		// 3. swap buffers (double buffering)
		glfwSwapBuffers(window.get());

		// 4. Process the user input and window events
		glfwPollEvents();
	}

	return 0;
}

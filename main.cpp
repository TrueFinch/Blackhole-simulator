//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <iostream>
#include <memory>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

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

	std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)> window{
		glfwCreateWindow(800, 600, "Black hole simulator", nullptr, nullptr),
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

	// render loop
	while (!glfwWindowShouldClose(window.get())) {
		// 1. clear screen with the background color (RGBA, each component in 0..1)
		glClearColor(0.f, 0.f, 0.f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 2. todo call draw

		// 3. swap buffers (double buffering)
		glfwSwapBuffers(window.get());

		// 4. Process the user input and window events
		glfwPollEvents();
	}

	return 0;
}

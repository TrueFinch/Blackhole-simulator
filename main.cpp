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
	// physical constants
	constexpr double c = 299792458.0;      // Speed of light (m/s)
	constexpr double G = 6.67430e-11;      // Gravitational constant (m³/kg·s²)
	// todo: make parameter customizable in runtime
	constexpr double BH_MASS = 8.54e36;    // Blackhole mass in kg
	constexpr double rs = 2.0 * G * BH_MASS / (c * c);  // Event horizon radius

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

	struct Ray {
		Ray(double x, double y, double vx, double vy) {
			r = std::sqrt(x * x + y * y);
			phi = std::atan2(y, x);

			const auto cos_phi = std::cos(phi);
			const auto sin_phi = std::sin(phi);
			v_r = vx * cos_phi + vy * sin_phi;
			v_phi = (-vx * sin_phi + vy * cos_phi) / r;

			L = r * r * v_phi;

			const auto f = 1.0 - rs / r;
			const auto dt_dlambda = std::sqrt((v_r * v_r) / (f * f) + (r * r * v_phi * v_phi) / f);
			E = f * dt_dlambda;

			std::cout << "\nRay initialized:\n";
			std::cout << "  Position: r = " << (r / rs) << " rs\n";
			std::cout << "  Energy: E = " << E << "\n";
			std::cout << "  Angular momentum: L = " << L << "\n\n";
		}

		void displayConservedQuantities() const {
			std::cout << "Conserved quantities:\n";
			std::cout << "  E = " << E << " (energy)\n";
			std::cout << "  L = " << L << " (angular momentum)\n";

			// Show that these define the ray's behavior
			double impactParameter{L / E};
			std::cout << "  Impact parameter b = " << impactParameter / rs << " rs\n";
		}
		// polar coordinates
		double r;     // Radial distance from black hole
		double phi;   // Angular position (radians)
		// velocity in polar coordinates
		double v_r;   // Radial velocity (dr/dλ)
		double v_phi; // Angular velocity (dφ/dλ)

		double L;     // Conserved angular momentum
		double E;     // Conserved energy parameter
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

void drawCircleOutline(float center_x, float center_y, float radius, int segments) {
	glBegin(GL_LINE_LOOP);

	for (auto i = 0; i < segments; ++i) {
		const auto angle = (static_cast<float>(i) * 2.0f * static_cast<float>(M_PI)) / static_cast<float>(segments);
		const auto ox = center_x + radius * std::cos(angle);
		const auto oy = center_y + radius * std::sin(angle);
		glVertex2f(ox, oy);
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

	// Display black hole information
	std::cout << "\n=== Black Hole Properties ===\n";
	std::cout << "Mass: " << BH_MASS << " kg (Sagittarius A*)\n";
	std::cout << "Schwarzschild radius: " << rs << " m\n";
	std::cout << "  = " << rs / 1000.0 << " km\n";
	std::cout << "  = " << rs / 1e9 << " million km\n";
	std::cout << "Photon sphere: " << 1.5 * rs / 1e9 << " million km\n\n";

	// Create test ray - horizontal ray approaching from left
	constexpr double initialX = -1e11;
	constexpr double initialY = -3.27606302719999999e10;
	constexpr double velocityX = c;
	constexpr double velocityY = 0.0;

	Ray testRay{initialX, initialY, velocityX, velocityY};
	testRay.displayConservedQuantities();

	// render loop
	while (!glfwWindowShouldClose(window.get())) {
		// 1. clear screen with the background color (RGBA, each component in 0..1)
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// 2.1 Draw event horizon (filled black circle)
		glColor3f(0.0f, 0.0f, 0.0f);
		drawCircle(0.0f, 0.0f, 1.0f, 100);

		// 2.2 Draw photon sphere (cyan outline on top)
		glColor3f(0.0f, 0.8f, 0.8f);
		glLineWidth(2.0f);
		drawCircleOutline(0.0f, 0.0f, 1.5f, 100);

		// 3. swap buffers (double buffering)
		glfwSwapBuffers(window.get());

		// 4. Process the user input and window events
		glfwPollEvents();
	}

	return 0;
}

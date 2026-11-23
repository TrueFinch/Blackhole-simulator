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
	constexpr double c = 299792458.0; // Speed of light (m/s)
	constexpr double G = 6.67430e-11; // Gravitational constant (m³/kg·s²)
	// todo: make parameter customizable in runtime
	constexpr double BH_MASS = 8.54e36; // Blackhole mass in kg
	constexpr double rs = 2.0 * G * BH_MASS / (c * c); // Event horizon radius

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
		double r; // Radial distance from black hole
		double phi; // Angular position (radians)
		// velocity in polar coordinates
		double v_r; // Radial velocity (dr/dλ)
		double v_phi; // Angular velocity (dφ/dλ)

		double L; // Conserved angular momentum
		double E; // Conserved energy parameter
	};


	// This computes the right-hand side of the geodesic differential equations
	// Parameters:
	//   ray - the current state
	//   rhs - output array for [dr/dλ, dφ/dλ, dv_r/dλ, dv_phi/dλ]
	void geodesicRHS(const Ray& ray, double rhs[4]) {
		// Extract variables from ray
		double r = ray.r;
		double v_r = ray.v_r;
		double v_phi = ray.v_phi;
		double E = ray.E;

		double f_rhs{1.0 - rs / r};
		double dt_dlambda{E / f_rhs};

		// rhs[0] = dr/dλ (position derivative = velocity)
		rhs[0] = v_r;

		// rhs[1] = dφ/dλ (angular position derivative = angular velocity)
		rhs[1] = v_phi;


		// Gravitational attraction (negative):
		// * This is the inward pull from gravity
		// * Negative sign means acceleration toward the black hole
		// * Depends on how fast time is passing (dt/dλ)
		// * Strongest contribution near the black hole
		const auto t1 = -(rs / (2.0 * r * r)) * f_rhs * (dt_dlambda * dt_dlambda);
		// Radial Velocity Correction (Positive)
		// * Relativistic correction for radial motion
		// * Always positive (outward effect)
		// * Important when moving quickly in the radial direction
		// * The 1/f_rhs factor makes it large near the horizon
		const auto t2 = (rs / (2.0 * r * r * f_rhs)) * (v_r * v_r);
		// Centrifugal Effect (Positive)
		// * Like centrifugal force: angular motion creates outward acceleration
		// * Proportional to (angular velocity)²
		// * The (r - rs) factor is the effective centrifugal radius
		// * Can cause light to orbit the black hole
		const auto t3 = (r - rs) * (v_phi * v_phi);
		// rhs[2] = dv_r/dλ (radial acceleration from geodesic equation)
		// Exact Schwarzschild null geodesic equation:
		// dv_r/dλ = -(rs/(2r²))f(dt/dλ)² + (rs/(2r²f))(v_r)² + (r - rs)(v_φ)²
		rhs[2] = t1 + t2 + t3;

		// rhs[3] = dv_phi/dλ (angular acceleration from geodesic equation)
		// dv_φ/dλ = -2v_r·v_φ/r
		rhs[3] = -2.0 * v_r * v_phi / r;
	}

	// This adds two state vectors: out = a + b * factor
	// Parameters:
	//   a[4] - first state vector
	//   b[4] - second state vector
	//   factor - scaling factor for b
	//   out[4] - result
	void addState(const double a[4], const double b[4], double factor, double out[4]) {
		for (int i = 0; i < 4; ++i) {
			out[i] = a[i] + b[i] * factor;
		}
	}

	// This performs one RK4 integration step
	// Parameters:
	//   ray - the ray to update (modified in place)
	//   dlambda - step size along affine parameter
	void rk4Step(Ray& ray, double dlambda) {
		// Current state
		double y0[4]{ray.r, ray.phi, ray.v_r, ray.v_phi};
		double k1[4], k2[4], k3[4], k4[4], temp[4];

		// k1: Slope at current point
		geodesicRHS(ray, k1);

		// k2: Slope at midpoint using k1
		addState(y0, k1, dlambda / 2.0, temp);
		Ray r2 = ray;
		r2.r = temp[0];
		r2.phi = temp[1];
		r2.v_r = temp[2];
		r2.v_phi = temp[3];
		geodesicRHS(r2, k2);

		// k3: Slope at midpoint using k2 (better estimate)
		addState(y0, k2, dlambda / 2.0, temp);
		Ray r3 = ray;
		r3.r = temp[0];
		r3.phi = temp[1];
		r3.v_r = temp[2];
		r3.v_phi = temp[3];
		geodesicRHS(r3, k3);

		// k4: Slope at endpoint using k3
		addState(y0, k3, dlambda, temp);
		Ray r4 = ray;
		r4.r = temp[0];
		r4.phi = temp[1];
		r4.v_r = temp[2];
		r4.v_phi = temp[3];
		geodesicRHS(r4, k4);

		// Final weighted average: (k1 + 2*k2 + 2*k3 + k4) / 6
		ray.r += (dlambda / 6.0) * (k1[0] + 2.0 * k2[0] + 2.0 * k3[0] + k4[0]);
		ray.phi += (dlambda / 6.0) * (k1[1] + 2.0 * k2[1] + 2.0 * k3[1] + k4[1]);
		ray.v_r += (dlambda / 6.0) * (k1[2] + 2.0 * k2[2] + 2.0 * k3[2] + k4[2]);
		ray.v_phi += (dlambda / 6.0) * (k1[3] + 2.0 * k2[3] + 2.0 * k3[3] + k4[3]);
	}

	// This function tests the RK4 integration and verifies conserved quantities
	// Parameters:
	//   ray - the ray to integrate (passed by value, so original is not modified)
	//   num_steps - number of integration steps to perform
	//   dlambda - step size for each integration step
	void debugRK4Integration(Ray ray, int num_steps, double dlambda) {
		std::cout << "\n=== RK4 Integration Test ===\n";
		std::cout << "Step size (dlambda): " << dlambda << "\n";
		std::cout << "Number of steps: " << num_steps << "\n\n";

		// Store initial conserved quantities
		double initial_L = ray.L;
		double initial_E = ray.E;

		std::cout << "Initial state:\n";
		std::cout << "  r = " << (ray.r / rs) << " rs\n";
		std::cout << "  φ = " << ray.phi << " rad\n";
		std::cout << "  v_r = " << ray.v_r << "\n";
		std::cout << "  v_φ = " << ray.v_phi << "\n\n";

		// Run integration steps
		for (int i{0}; i < num_steps; ++i) {
			rk4Step(ray, dlambda);
		}

		std::cout << "After " << num_steps << " RK4 steps:\n";
		std::cout << "  r = " << (ray.r / rs) << " rs\n";
		std::cout << "  φ = " << ray.phi << " rad\n";
		std::cout << "  v_r = " << ray.v_r << "\n";
		std::cout << "  v_φ = " << ray.v_phi << "\n\n";

		// Recompute conserved quantities
		double current_L = ray.r * ray.r * ray.v_phi;
		double f = 1.0 - rs / ray.r;
		double dt_dlambda = std::sqrt((ray.v_r * ray.v_r) / (f * f) + (ray.r * ray.r * ray.v_phi * ray.v_phi) / f);
		double current_E = f * dt_dlambda;

		std::cout << "Conserved quantity check:\n";
		std::cout << "  Initial L = " << initial_L << "\n";
		std::cout << "  Current L = " << current_L << "\n";
		std::cout << "  Initial E = " << initial_E << "\n";
		std::cout << "  Current E = " << current_E << "\n";

		// Calculate drift percentages
		double L_drift{std::abs(current_L - initial_L) / std::abs(initial_L) * 100.0};
		double E_drift{std::abs(current_E - initial_E) / std::abs(initial_E) * 100.0};

		std::cout << "  L drift: " << L_drift << "%\n";
		std::cout << "  E drift: " << E_drift << "%\n";
	}
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

	// Run integration test
	const double dlambda = 1.0;
	const int num_steps = 10;
	debugRK4Integration(testRay, num_steps, dlambda);

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

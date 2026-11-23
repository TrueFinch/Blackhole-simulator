//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <cmath>
#include <iostream>
#include <memory>
#include <random>
#include <vector>
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

	//Physical viewport: ±100 billion meters horizontally,
	//±75 billion meters vertically.
	//This shows a region much larger than the Schwarzschild radius (~26 million meters).
	constexpr double viewWidth = 1e11;
	constexpr double viewHeight = 7.5e10;
	// Derive maxDistance from viewport width
	constexpr double maxDistance = 2.0 * viewWidth;

	constexpr int kNumStars = 200;

	// Forward declarations (needed for Ray::integrate method)
	struct Ray;

	void rk4Step(Ray& ray, double dlambda);

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

			trail.emplace_back(static_cast<float>(x), static_cast<float>(y));

			// Store initial velocity direction (NOT position angle!)
			prevVelocityAngle = initialVelocityAngle = std::atan2(vy, vx);
		}

		[[nodiscard]] bool isCaptured() const {
			return r <= rs * 1.01;
		}

		[[nodiscard]] bool hasEscaped(double maxDistance) const {
			return r > maxDistance;
		}

		void recordPosition() {
			const float x{static_cast<float>(r * std::cos(phi))};
			const float y{static_cast<float>(r * std::sin(phi))};
			trail.emplace_back(x, y);
		}

		void integrate(double dlambda, double maxDistance) {
			if (isCaptured() || hasEscaped(maxDistance)) {
				return;
			}
			rk4Step(*this, dlambda);
			recordPosition();
			updateDeflection();
		}

		void updateDeflection() {
			// Convert current polar velocity back to Cartesian
			const double vx_current = v_r * std::cos(phi) - r * v_phi * std::sin(phi);
			const double vy_current = v_r * std::sin(phi) + r * v_phi * std::cos(phi);

			// Calculate current velocity direction
			const double currentVelocityAngle = std::atan2(vy_current, vx_current);
			// Change since the previous step, wrapped into (-π, π]
			double delta = currentVelocityAngle - prevVelocityAngle;
			if (delta > M_PI) {
				delta -= 2.0 * M_PI;
			}
			if (delta < -M_PI) {
				delta += 2.0 * M_PI;
			}

			// Measure how much direction has changed
			totalTurn += delta; // signed, accumulates full loops
			prevVelocityAngle = currentVelocityAngle;
			deflection = std::abs(totalTurn);
		}

		// polar coordinates
		double r; // Radial distance from black hole
		double phi; // Angular position (radians)
		// velocity in polar coordinates
		double v_r; // Radial velocity (dr/dλ)
		double v_phi; // Angular velocity (dφ/dλ)

		double L; // Conserved angular momentum
		double E; // Conserved energy parameter

		double initialVelocityAngle; // Direction ray was heading at start
		double prevVelocityAngle;
		double totalTurn = 0.0;
		double deflection = 0.0; // How much direction has changed

		std::vector<glm::vec2> trail;
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

void drawDashedCircle(float x, float y, float radius, int segments) {
	// Draw every other segment to create dashes
	for (auto i = 0; i < segments; i += 2) {
		const float theta1{(static_cast<float>(i) * 2.0f * static_cast<float>(M_PI)) / static_cast<float>(segments)};
		const float theta2{
			(static_cast<float>(i + 1) * 2.0f * static_cast<float>(M_PI)) / static_cast<float>(segments)
		};

		glBegin(GL_LINES);
		glVertex2f(x + radius * std::cos(theta1), y + radius * std::sin(theta1));
		glVertex2f(x + radius * std::cos(theta2), y + radius * std::sin(theta2));
		glEnd();
	}
}

void drawRays(const std::vector<Ray>& rays) {
	// Enable transparency for fading effect
	glEnable(GL_BLEND);
	// Configure how transparent colors blend with background
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glLineWidth(1.5f);

	// Draw trails with fading effect
	for (const auto& ray: rays) {
		const auto n = ray.trail.size();
		if (n < 2) {
			continue;
		}
		// Normalize deflection (0 to π radians) to 0–1 scale
		constexpr float maxDeflection = M_PI;
		const float t = std::min(1.0f, static_cast<float>(ray.deflection) / maxDeflection);

		// Cyan → Purple → Red gradient
		const float r = t; // 0 → 1 (increases with deflection)
		const float g = 0.5f * (1.0f - t); // 0.5 → 0 (decreases from mid-value)
		const float b = 1.0f - t; // 1 → 0 (decreases with deflection)

		glBegin(GL_LINE_STRIP);
		for (auto i = 0; i < n; ++i) {
			const auto alpha = 0.2f + 0.8f * static_cast<float>(i) / static_cast<float>(n - 1);
			glColor4f(r, g, b, alpha);
			glVertex2f(ray.trail[i].x, ray.trail[i].y);
		}
		glEnd();
	}

	// Draw current ray positions as bright points
	glPointSize(3.f);
	glColor3f(1.f, 1.f, 0.f);

	glBegin(GL_POINTS);
	for (const auto& ray: rays) {
		if (!ray.trail.empty() && !ray.isCaptured()) {
			glVertex2f(ray.trail.back().x, ray.trail.back().y);
		}
	}
	glEnd();

	// Disable transparency for fading effect
	glDisable(GL_BLEND);
}

std::vector<glm::vec2> generateStars(int count) {
	std::vector<glm::vec2> stars;
	std::random_device rd;
	std::mt19937 gen(rd());

	// Match your viewport dimensions: ±100 billion meters X, ±75 billion meters Y
	std::uniform_real_distribution<float> distX(-viewWidth, viewWidth);
	std::uniform_real_distribution<float> distY(-viewHeight, viewHeight);

	for (int i{0}; i < count; ++i) {
		stars.emplace_back(distX(gen), distY(gen));
	}
	return stars;
}

//todo: add different colors and sizes of stars
void drawStars(const std::vector<glm::vec2>& stars) {
	glPointSize(2.f);
	glColor3f(1.f, 1.f, 1.f);
	glBegin(GL_POINTS);
	for (const auto& star: stars) {
		glVertex2f(star.x, star.y);
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

	{
		// todo glMatrixMode, glOrtho, glBegin/glVertex/glColor is legacy API -> replace later
		// Switch to projection mode
		glMatrixMode(GL_PROJECTION);
		// Reset matrix
		glLoadIdentity();
		// Update the viewport to use physical coordinates
		glOrtho(-viewWidth, viewWidth, -viewHeight, viewHeight, -1.0, 1.0);

		// Switch back to modelview matrix
		glMatrixMode(GL_MODELVIEW);
		// Reset matrix
		glLoadIdentity();
	}

	const auto stars = generateStars(kNumStars);

	std::vector<Ray> rays;
	constexpr int numRays = 10;
	constexpr double startX = -viewWidth;
	constexpr double vx = c;
	constexpr double vy = 0.0;

	for (auto i = 0; i < numRays; ++i) {
		const double startY = (2.5 + i * 0.1) * rs;
		rays.emplace_back(startX, startY, vx, vy);
	}

	// render loop
	while (!glfwWindowShouldClose(window.get())) {
		// 1. clear screen with the background color (RGBA, each component in 0..1)
		glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw background stars first
		drawStars(stars);

		// 2.1 Draw event horizon (filled black circle)
		glColor3f(0.0f, 0.0f, 0.0f);
		constexpr float eventRadius = rs;
		drawCircle(0.0f, 0.0f, eventRadius, 100);

		// 2.2 Draw photon sphere (cyan outline on top)
		glColor3f(0.0f, 0.8f, 0.8f);
		glLineWidth(2.0f);
		constexpr float photonRadius = 1.5 * rs;
		drawDashedCircle(0.0f, 0.0f, photonRadius, 100);

		for (auto& ray: rays) {
			ray.integrate(1.0, maxDistance);
		}
		drawRays(rays);

		// 3. swap buffers (double buffering)
		glfwSwapBuffers(window.get());

		// 4. Process the user input and window events
		glfwPollEvents();
	}

	return 0;
}

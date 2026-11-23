//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <algorithm>
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

	// Lengths are measured in units of rs
	constexpr float kRs = 1.0f;

	constexpr float viewWidth = 8.f;
	constexpr float viewHeight = 6.f;
	// Derive maxDistance from viewport width
	constexpr float kMaxDistance = 2.0 * viewWidth;

	constexpr int kNumStars = 200;

	// Forward declarations (needed for Ray::integrate method)
	struct Ray;

	void verletStep(Ray& ray, float dlambda);

	// Radial acceleration of a light ray in Schwarzschild spacetime:
	//   d²r/dλ² = L²/r³ · (1 - 3rs/(2r))
	// Derived from the null geodesic equation using conservation of L
	// (dφ/dλ = L/r²), so it depends on position only. That is what makes
	// Velocity Verlet symplectic (and second-order) for this system.
	// It is the negative gradient of the effective potential L²/(2r²)·(1 - rs/r),
	// whose maximum at r = 1.5 rs is the photon sphere.
	float radialAcceleration(float r, float L) {
		return L * L / (r * r * r) * (1.f - 1.5f * kRs / r);
	}

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
		Ray(float x, float y, float vx, float vy) {
			// Convert Cartesian position to polar coordinates
			r = std::sqrt(x * x + y * y);
			phi = std::atan2(y, x);

			// Convert Cartesian velocity to polar coordinates
			// Using transformation:
			// v_r = vx*cos(φ) + vy*sin(φ)
			// v_φ = (-vx*sin(φ) + vy*cos(φ)) / r
			const auto cos_phi = std::cos(phi);
			const auto sin_phi = std::sin(phi);
			v_r = vx * cos_phi + vy * sin_phi;
			v_phi = (-vx * sin_phi + vy * cos_phi) / r;

			// Angular momentum L = r² * v_φ
			L = r * r * v_phi;

			// Metric coefficient (f = 1 - rs/r)
			const auto f = 1.f - kRs / r;
			// For null geodesics (light rays), the normalization condition gives:
			// dt/dλ = √[(v_r)²/f² + (r²·v_φ)²/f]
			const auto dt_dlambda = std::sqrt((v_r * v_r) / (f * f) + (r * r * v_phi * v_phi) / f);
			// Energy E = f * dt/dλ
			E = f * dt_dlambda;

			// Acceleration at the start position; verletStep keeps it up to date
			a_r = radialAcceleration(r, L);

			trail.emplace_back(x, y);

			// Store initial velocity direction (NOT position angle!)
			prevVelocityAngle = initialVelocityAngle = std::atan2(vy, vx);
		}

		[[nodiscard]] bool isCaptured() const {
			return r <= kRs * 1.01f;
		}

		[[nodiscard]] bool hasEscaped(float maxDistance) const {
			return r > maxDistance;
		}

		void recordPosition() {
			const auto x = r * std::cos(phi);
			const auto y = r * std::sin(phi);
			trail.emplace_back(x, y);
		}

		void integrate(float dlambda, float maxDistance) {
			if (isCaptured() || hasEscaped(maxDistance)) {
				return;
			}
			verletStep(*this, dlambda);
			recordPosition();
			updateDeflection();
		}

		void updateDeflection() {
			// Convert current polar velocity back to Cartesian
			const auto vx_current = v_r * std::cos(phi) - r * v_phi * std::sin(phi);
			const auto vy_current = v_r * std::sin(phi) + r * v_phi * std::cos(phi);

			// Calculate current velocity direction
			const auto currentVelocityAngle = std::atan2(vy_current, vx_current);
			// Change since the previous step, wrapped into (-π, π]
			auto delta = currentVelocityAngle - prevVelocityAngle;
			if (delta > M_PI) {
				delta -= 2.f * M_PI;
			}
			if (delta < -M_PI) {
				delta += 2.f * M_PI;
			}

			// Measure how much direction has changed
			totalTurn += delta; // signed, accumulates full loops
			prevVelocityAngle = currentVelocityAngle;
			deflection = std::abs(totalTurn);
		}

		// polar coordinates
		float r; // Radial distance from black hole
		float phi; // Angular position (radians)
		// velocity in polar coordinates
		float v_r; // Radial velocity (dr/dλ)
		float v_phi; // Angular velocity (dφ/dλ), always L/r²
		float a_r; // Radial acceleration (d²r/dλ²) at the current r

		float L; // Conserved angular momentum
		float E; // Conserved energy parameter

		float initialVelocityAngle; // Direction ray was heading at start
		float prevVelocityAngle;
		float totalTurn = 0.f;
		float deflection = 0.f; // How much direction has changed

		std::vector<glm::vec2> trail;
	};

	// Symplectic Velocity Verlet (Stormer-Verlet) integration step, "kick-drift-kick".
	// Symplectic because the radial acceleration depends on r only (see radialAcceleration):
	// energy error stays bounded and L is conserved exactly by construction.
	// One acceleration evaluation per step: the value at the end of a step
	// is cached in ray.a_r and reused at the start of the next one.
	// https://www.algorithm-archive.org/contents/verlet_integration/verlet_integration.html
	void verletStep(Ray& ray, float dlambda) {
		// KICK: half-step radial velocity update with the acceleration at the current r
		const float v_r_half = ray.v_r + 0.5f * dlambda * ray.a_r;

		// DRIFT: full-step position update.
		// φ is advanced with dφ/dλ = L/r² at the midpoint radius (second-order accurate).
		const float r_old = ray.r;
		ray.r += dlambda * v_r_half;
		const float r_mid = 0.5f * (r_old + ray.r);
		ray.phi += dlambda * ray.L / (r_mid * r_mid);

		// KICK: complete the radial velocity update with the acceleration at the new r
		ray.a_r = radialAcceleration(ray.r, ray.L);
		ray.v_r = v_r_half + 0.5f * dlambda * ray.a_r;

		// Angular velocity follows from conservation of L (used by updateDeflection)
		ray.v_phi = ray.L / (ray.r * ray.r);
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

struct Star {
	glm::vec2 pos;
	float baseBrightness; // average brightness
	float amplitude; // how strong it twinkles
	float frequencyHz; // twinkle rate
	float phase1; // offset so stars don't pulse in sync
	float phase2; // offset so stars don't pulse in sync
};

std::vector<Star> generateStars(int count) {
	std::random_device rd;
	std::mt19937 gen(rd());

	// Match your viewport dimensions: ±100 billion meters X, ±75 billion meters Y
	std::uniform_real_distribution<float> distX(-viewWidth, viewWidth);
	std::uniform_real_distribution<float> distY(-viewHeight, viewHeight);

	std::uniform_real_distribution baseBrightness(0.5f, 0.9f);
	std::uniform_real_distribution amplitude(0.05f, 0.2f);
	std::uniform_real_distribution frequencyHz(0.2f, 0.5f);
	std::uniform_real_distribution phase1(0.f, 2.f * static_cast<float>(M_PI));
	std::uniform_real_distribution phase2(0.f, 2.f * static_cast<float>(M_PI));

	std::vector<Star> stars;
	for (auto i = 0; i < count; ++i) {
		stars.emplace_back(Star{
			glm::vec2(distX(gen), distY(gen)),
			baseBrightness(gen), amplitude(gen),
			frequencyHz(gen), phase1(gen), phase2(gen)
		});
	}
	return stars;
}

//todo: add different colors and sizes of stars
void drawStars(const std::vector<Star>& stars, float time) {
	constexpr auto kWeight1 = 0.7f;
	constexpr auto kWeight2 = 0.3f;
	// Frequency ratio of the second sine to the first.
	// With an integer (or simple fraction) ratio the combined pattern repeats
	// every few periods of the main sine. With an irrational ratio the peaks
	// line up differently each time, so the twinkle looks random but stays smooth.
	// The golden ratio is the irrational number worst approximated by fractions,
	// so the pattern doesn't even nearly repeat.
	constexpr auto kSecondSinSpeed = 1.618034f;

	glBegin(GL_POINTS);
	for (const auto& star: stars) {
		const auto speed = 2.f * static_cast<float>(M_PI) * star.frequencyHz;
		const auto wave = kWeight1 * std::sin(speed * time + star.phase1)
						+ kWeight2 * std::sin(speed * kSecondSinSpeed * time + star.phase2);
		const auto b = std::clamp(star.baseBrightness + star.amplitude * wave, 0.f, 1.f);
		glColor3f(b, b, b);
		glVertex2f(star.pos.x, star.pos.y);
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

	// todo move parameter to settings
	// Enable visual polish features (GL_LINE_SMOOTH, GL_BLEND, GL_NICEST hint)
	// Generates partially transparent edge pixels
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
	// GL_BLEND with GL_SRC_ALPHA blends those pixels with the background
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Display black hole information
	std::cout << "\n=== Black Hole Properties ===\n";
	std::cout << "Schwarzschild radius: " << kRs << " (geometric units)\n";

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
	constexpr int numRays = 30;
	constexpr float startX = -viewWidth;
	constexpr float vx = 1.f;
	constexpr float vy = 0.f;

	for (auto i = 0; i < numRays; ++i) {
		const float startY = -viewHeight + static_cast<float>(i) * viewHeight * 2.f / numRays;
		rays.emplace_back(startX, startY, vx, vy);
	}

	// render loop
	while (!glfwWindowShouldClose(window.get())) {
		// 1. clear screen with the background color (RGBA, each component in 0..1)
		glClearColor(0.06f, 0.06f, 0.08f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		// Draw background stars first
		drawStars(stars, static_cast<float>(glfwGetTime()));

		// 2.1 Draw event horizon (filled black circle)
		glColor3f(0.0f, 0.0f, 0.0f);
		constexpr float eventRadius = kRs;
		drawCircle(0.0f, 0.0f, eventRadius, 100);

		// 2.2 Draw photon sphere (cyan outline on top)
		glColor3f(0.0f, 0.8f, 0.8f);
		glLineWidth(2.0f);
		constexpr float photonRadius = 1.5 * kRs;
		drawDashedCircle(0.0f, 0.0f, photonRadius, 100);

		for (auto& ray: rays) {
			ray.integrate(0.05f, kMaxDistance);
		}
		drawRays(rays);

		// 3. swap buffers (double buffering)
		glfwSwapBuffers(window.get());

		// 4. Process the user input and window events
		glfwPollEvents();
	}

	return 0;
}

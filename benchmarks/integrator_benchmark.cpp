//
// Compares the three light-ray integrators tried in this project:
//
//   1. RK4          - classic Runge-Kutta on the full geodesic equations
//                     (accelerations depend on velocity, 4 evaluations per step)
//   2. Verlet naive - Velocity Verlet on the same velocity-dependent equations
//                     (first-order and not symplectic, 2 evaluations per step)
//   3. Verlet r(r)  - Velocity Verlet on d²r/dλ² = L²/r³·(1 - 1.5 rs/r) with dφ/dλ = L/r²
//                     (symplectic, second-order, 1 cached evaluation per step;
//                     this is what main.cpp uses)
//
// The physics is copied from main.cpp on purpose, so this file builds without OpenGL.
// Units are geometric with rs = 1. Rays start at x = -8 moving in +x with impact parameter b,
// like the rays in the simulator. All three methods run in float, as in the simulator;
// the reference solution is RK4 in double with a 100x smaller step.
//
// Build in Release: timings from a Debug build are meaningless.
//

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <iterator>
#include <vector>

namespace {
	constexpr double kRs = 1.0;
	constexpr double kStartX = -8.0;
	constexpr double kCaptureRadius = 1.01 * kRs;

	template <typename T>
	struct State {
		T r, phi; // polar position
		T v_r, v_phi; // dr/dλ, dφ/dλ
		T E, L; // conserved energy and angular momentum (values at the start)
		T a_r; // cached radial acceleration, used by "Verlet r(r)" only
	};

	template <typename T>
	T energyOf(const State<T>& s) {
		const T f = T(1) - T(kRs) / s.r;
		return f * std::sqrt(s.v_r * s.v_r / (f * f) + s.r * s.r * s.v_phi * s.v_phi / f);
	}

	template <typename T>
	T radialAccelerationPositionOnly(T r, T L) {
		return L * L / (r * r * r) * (T(1) - T(1.5) * T(kRs) / r);
	}

	template <typename T>
	State<T> makeRay(T impactParameter) {
		const T x = T(kStartX), y = impactParameter, vx = 1, vy = 0;
		State<T> s{};
		s.r = std::sqrt(x * x + y * y);
		s.phi = std::atan2(y, x);
		s.v_r = vx * std::cos(s.phi) + vy * std::sin(s.phi);
		s.v_phi = (-vx * std::sin(s.phi) + vy * std::cos(s.phi)) / s.r;
		s.L = s.r * s.r * s.v_phi;
		s.E = energyOf(s);
		s.a_r = radialAccelerationPositionOnly(s.r, s.L);
		return s;
	}

	// Full geodesic accelerations (depend on velocity), as in the RK4 version of main.cpp
	template <typename T>
	void geodesicAccelerations(T r, T v_r, T v_phi, T E, T& a_r, T& a_phi) {
		const T rs = T(kRs);
		const T f = T(1) - rs / r;
		const T dt_dlambda = E / f;
		a_r = -(rs / (T(2) * r * r)) * f * dt_dlambda * dt_dlambda
			+ (rs / (T(2) * r * r * f)) * v_r * v_r
			+ (r - rs) * v_phi * v_phi;
		a_phi = T(-2) * v_r * v_phi / r;
	}

	// 1. RK4
	template <typename T>
	void stepRk4(State<T>& s, T h) {
		auto derivative = [&](T r, T v_r, T v_phi, T k[4]) {
			k[0] = v_r;
			k[1] = v_phi;
			geodesicAccelerations(r, v_r, v_phi, s.E, k[2], k[3]);
		};
		T k1[4], k2[4], k3[4], k4[4];
		derivative(s.r, s.v_r, s.v_phi, k1);
		derivative(s.r + h / 2 * k1[0], s.v_r + h / 2 * k1[2], s.v_phi + h / 2 * k1[3], k2);
		derivative(s.r + h / 2 * k2[0], s.v_r + h / 2 * k2[2], s.v_phi + h / 2 * k2[3], k3);
		derivative(s.r + h * k3[0], s.v_r + h * k3[2], s.v_phi + h * k3[3], k4);
		s.r += h / 6 * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]);
		s.phi += h / 6 * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]);
		s.v_r += h / 6 * (k1[2] + 2 * k2[2] + 2 * k3[2] + k4[2]);
		s.v_phi += h / 6 * (k1[3] + 2 * k2[3] + 2 * k3[3] + k4[3]);
	}

	// 2. Velocity Verlet on velocity-dependent accelerations (commit 3c6f76d)
	template <typename T>
	void stepVerletNaive(State<T>& s, T h) {
		T a_r, a_phi;
		geodesicAccelerations(s.r, s.v_r, s.v_phi, s.E, a_r, a_phi);
		const T v_r_half = s.v_r + T(0.5) * h * a_r;
		const T v_phi_half = s.v_phi + T(0.5) * h * a_phi;
		s.r += h * v_r_half;
		s.phi += h * v_phi_half;
		geodesicAccelerations(s.r, v_r_half, v_phi_half, s.E, a_r, a_phi);
		s.v_r = v_r_half + T(0.5) * h * a_r;
		s.v_phi = v_phi_half + T(0.5) * h * a_phi;
	}

	// 3. Velocity Verlet on the position-only radial equation (current main.cpp)
	template <typename T>
	void stepVerletRadial(State<T>& s, T h) {
		const T v_r_half = s.v_r + T(0.5) * h * s.a_r;
		const T r_old = s.r;
		s.r += h * v_r_half;
		const T r_mid = T(0.5) * (r_old + s.r);
		s.phi += h * s.L / (r_mid * r_mid);
		s.a_r = radialAccelerationPositionOnly(s.r, s.L);
		s.v_r = v_r_half + T(0.5) * h * s.a_r;
		s.v_phi = s.L / (s.r * s.r);
	}

	using FloatStep = void (*)(State<float>&, float);

	struct Method {
		const char* name;
		FloatStep step;
		int evaluationsPerStep;
	};

	const Method kMethods[] = {
		{"RK4", stepRk4<float>, 4},
		{"Verlet naive", stepVerletNaive<float>, 2},
		{"Verlet r(r)", stepVerletRadial<float>, 1},
	};

	struct Outcome {
		bool captured = false;
		double x = 0, y = 0; // Cartesian position at the end
		double maxEnergyError = 0; // max |E - E0| / E0 over the run
		double angularMomentumError = 0; // |L - L0| / L0 at the end
	};

	// Integrates for a fixed affine length λ_end = steps * h, or until the ray is captured
	template <typename T, typename Step>
	Outcome integrate(T impactParameter, T h, int steps, Step step) {
		State<T> s = makeRay(impactParameter);
		const T E0 = s.E, L0 = s.L;
		Outcome out;
		for (int i = 0; i < steps; ++i) {
			step(s, h);
			if (!(s.r > T(kCaptureRadius))) { // also catches NaN
				out.captured = true;
				return out;
			}
			out.maxEnergyError = std::max(out.maxEnergyError, double(std::abs((energyOf(s) - E0) / E0)));
		}
		out.x = double(s.r) * std::cos(double(s.phi));
		out.y = double(s.r) * std::sin(double(s.phi));
		out.angularMomentumError = double(std::abs((s.r * s.r * s.v_phi - L0) / L0));
		return out;
	}

	Outcome reference(double impactParameter, double h, int steps) {
		constexpr int kRefinement = 100;
		return integrate<double>(impactParameter, h / kRefinement, steps * kRefinement, stepRk4<double>);
	}

	double positionError(const Outcome& a, const Outcome& ref) {
		return std::hypot(a.x - ref.x, a.y - ref.y);
	}

	void printAccuracyTable() {
		constexpr float kStep = 0.05f;
		constexpr float kLambdaEnd = 20.f;
		const int steps = static_cast<int>(kLambdaEnd / kStep);
		const float impactParameters[] = {5.f, 3.f, 2.8f, 2.62f, 2.6f, 2.598f, 2.5f};

		std::printf("=== Accuracy (float, dλ = %.3g, λ_end = %.0f) ===\n", kStep, kLambdaEnd);
		std::printf("Critical impact parameter b_c = 3√3/2 rs ≈ 2.598 rs; rays with b < b_c are captured.\n");
		std::printf("Position error = distance to the reference solution at λ_end, in rs.\n\n");
		std::printf("%-8s %-14s %-10s %16s %14s %14s\n", "b (rs)", "method", "result", "position error", "max E err %", "L drift %");

		for (const float b : impactParameters) {
			const Outcome ref = reference(double(b), double(kStep), steps);
			for (const auto& m : kMethods) {
				const Outcome o = integrate<float>(b, kStep, steps, m.step);
				if (o.captured || ref.captured) {
					const bool agrees = o.captured == ref.captured;
					std::printf("%-8.4g %-14s %-10s %16s %14s %14s\n", b, m.name,
					            o.captured ? "captured" : "escaped", agrees ? "ok" : "WRONG FATE", "-", "-");
				} else {
					std::printf("%-8.4g %-14s %-10s %16.3e %14.3e %14.3e\n", b, m.name, "escaped",
					            positionError(o, ref), o.maxEnergyError * 100, o.angularMomentumError * 100);
				}
			}
			std::printf("\n");
		}
	}

	void printConvergenceTable() {
		constexpr float kImpactParameter = 2.8f;
		constexpr float kLambdaEnd = 20.f;
		const float stepSizes[] = {0.1f, 0.05f, 0.025f, 0.0125f};

		std::printf("=== Convergence (b = %.2g rs, λ_end = %.0f) ===\n", kImpactParameter, kLambdaEnd);
		std::printf("Order ≈ log2(error(2h) / error(h)): 4 for RK4, 2 for a correct Verlet.\n");
		std::printf("Errors stop shrinking at accumulated float round-off (~1e-5..1e-4 rs over thousands of steps):\n"
		            "there the order column is meaningless (RK4 is already at that floor for every dλ here).\n\n");
		std::printf("%-10s", "dλ");
		for (const auto& m : kMethods) {
			std::printf(" %14s %6s", m.name, "order");
		}
		std::printf("\n");

		double previous[std::size(kMethods)] = {};
		for (const float h : stepSizes) {
			const int steps = static_cast<int>(std::lround(kLambdaEnd / h));
			const Outcome ref = reference(double(kImpactParameter), double(h), steps);
			std::printf("%-10.4g", h);
			for (std::size_t i = 0; i < std::size(kMethods); ++i) {
				const double err = positionError(integrate<float>(kImpactParameter, h, steps, kMethods[i].step), ref);
				if (previous[i] > 0) {
					std::printf(" %14.3e %6.2f", err, std::log2(previous[i] / err));
				} else {
					std::printf(" %14.3e %6s", err, "-");
				}
				previous[i] = err;
			}
			std::printf("\n");
		}
		std::printf("\n");
	}

	void printPerformanceTable() {
		constexpr int kRays = 1000;
		constexpr int kSteps = 2000;
		constexpr int kRepeats = 5;
		constexpr float kStep = 0.05f;

		// Escaping rays only (b from 3 to 8 rs), so every ray runs all steps
		std::vector<State<float>> initial;
		initial.reserve(kRays);
		for (int i = 0; i < kRays; ++i) {
			initial.push_back(makeRay(3.f + 5.f * float(i) / float(kRays - 1)));
		}

		std::printf("=== Performance (%d rays x %d steps, best of %d runs) ===\n\n", kRays, kSteps, kRepeats);
		std::printf("%-14s %12s %12s %10s\n", "method", "evals/step", "ns/step", "speed-up");

		volatile float sink = 0; // keeps the compiler from removing the work
		double baseline = 0;
		for (const auto& m : kMethods) {
			double best = 1e300;
			for (int rep = 0; rep < kRepeats; ++rep) {
				std::vector<State<float>> rays = initial;
				const auto start = std::chrono::steady_clock::now();
				for (auto& ray : rays) {
					for (int s = 0; s < kSteps; ++s) {
						m.step(ray, kStep);
					}
				}
				const auto end = std::chrono::steady_clock::now();
				float checksum = 0;
				for (const auto& ray : rays) {
					checksum += ray.r;
				}
				sink = sink + checksum;
				const double ns = std::chrono::duration<double, std::nano>(end - start).count();
				best = std::min(best, ns / (double(kRays) * kSteps));
			}
			if (baseline == 0) {
				baseline = best;
			}
			std::printf("%-14s %12d %12.2f %9.2fx\n", m.name, m.evaluationsPerStep, best, baseline / best);
		}
		std::printf("\n");
	}
}

int main() {
#ifndef NDEBUG
	std::printf("WARNING: built without NDEBUG (Debug build). Timings are not representative,\n"
	            "build the integrator_benchmark target in Release.\n\n");
#endif
	printAccuracyTable();
	printConvergenceTable();
	printPerformanceTable();
	return 0;
}

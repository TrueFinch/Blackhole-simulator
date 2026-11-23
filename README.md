# blackhole_simulator

A 2D black hole visualisation simulator written in C++23 with modern OpenGL.

## Dependencies

| Library | Purpose                                 |
|---------|-----------------------------------------|
| OpenGL  | Graphics API for rendering              |
| GLFW    | Creates windows and handles input       |
| GLEW    | Loads modern OpenGL functions           |
| GLM     | Math library for vectors and matrices   |

You also need a C++23 compiler (GCC 13+) and CMake 4.4 or newer.

## Setup (Debian/Ubuntu, WSL)

Install the dependencies:

```bash
./prepare.sh
```

The script uses `sudo` when it is not run as root.

## Integrator benchmark

`benchmarks/integrator_benchmark.cpp` compares the ray integrators tried in this
project (RK4, a naive Velocity Verlet and the Velocity Verlet used now): accuracy
against a high-precision reference, convergence order and time per step.
It doesn't need OpenGL. Build it in Release, otherwise the timings mean nothing:

```bash
cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --target integrator_benchmark
./build-release/integrator_benchmark
```

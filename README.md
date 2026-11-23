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

cmake_minimum_required(VERSION 4.4)
project(blackhole_simulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

set(DEPS glfw GLEW::GLEW glm::glm OpenGL::GL)

include(FetchContent)

FetchContent_Declare(
		tomlplusplus
		GIT_REPOSITORY https://github.com/marzer/tomlplusplus
		GIT_TAG v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)

# src/ layout (includes are written relative to src/, e.g. "core/ray.h"):
#   core/    physics, rays, simulation — depends only on glm
#   toml/    generic TOML layer: IoError, helpers, field schemas
#   config/  application settings (config/settings.toml)
#   scene/   scene model, scenario and style files (scenarios/*.toml, config/styles.toml)
#   render/  OpenGL primitives
#   view/    scene drawing: rays, black hole, starfield
#   app/     entry point

# Pure simulation core: depends only on glm, never on OpenGL/GLFW/GLEW or TOML.
add_library(blackhole_core STATIC
		src/core/constants.h
		src/core/physics.cpp
		src/core/physics.h
		src/core/ray.cpp
		src/core/ray.h
		src/core/simulation.cpp
		src/core/simulation.h
)
target_include_directories(blackhole_core PUBLIC src)
target_link_libraries(blackhole_core PUBLIC glm::glm)

# Data files: the TOML layer and the settings and scenes built on it. No OpenGL.
add_library(blackhole_io STATIC
		src/toml/io_error.cpp
		src/toml/io_error.h
		src/toml/toml_schema.cpp
		src/toml/toml_schema.h
		src/toml/toml_util.cpp
		src/toml/toml_util.h
		src/config/settings.cpp
		src/config/settings.h
		src/scene/scene.h
		src/scene/scene_io.cpp
		src/scene/scene_io.h
)
target_include_directories(blackhole_io PUBLIC src)
target_link_libraries(blackhole_io PUBLIC glm::glm tomlplusplus::tomlplusplus)
# toml++ turns off std::to_chars for floats on GCC/Clang (written for 2020 standard libraries) and
# falls back to printing max_digits10 digits: 0.0125 becomes 0.012500000000000001. Our toolchains
# have floating-point <charconv> (toTomlFloat already relies on it), so get the shortest form.
target_compile_definitions(blackhole_io PUBLIC TOML_FLOAT_CHARCONV=1)
# Root directory that holds config/ and scenarios/.
target_compile_definitions(blackhole_io PUBLIC BLACKHOLE_DATA_DIR="${CMAKE_SOURCE_DIR}")

add_executable(blackhole_simulator
		src/app/main.cpp
		src/render/render.cpp
		src/render/render.h
		src/view/scene_renderer.cpp
		src/view/scene_renderer.h
		src/view/starfield.cpp
		src/view/starfield.h
)
target_link_libraries(blackhole_simulator PRIVATE blackhole_core blackhole_io ${DEPS})

# Accuracy and performance comparison of the ray integrators (no OpenGL needed).
# Build it in Release for meaningful timings.
add_executable(integrator_benchmark benchmarks/integrator_benchmark.cpp)

# Tests
FetchContent_Declare(
		Catch2
		GIT_REPOSITORY https://github.com/catchorg/Catch2
		GIT_TAG v3.7.1
)
FetchContent_MakeAvailable(Catch2)

enable_testing()
add_executable(tests
		tests/core/test_physics.cpp
		tests/toml/test_toml_schema.cpp
		tests/config/test_settings.cpp
		tests/scene/test_scene_io.cpp
		tests/scene/test_data_files.cpp
)
target_link_libraries(tests PRIVATE blackhole_core blackhole_io Catch2::Catch2WithMain)
add_test(NAME tests COMMAND tests)

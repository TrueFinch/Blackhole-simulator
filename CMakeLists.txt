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

add_executable(blackhole_simulator
		src/app/main.cpp
		src/render/render.cpp
		src/render/render.h
		src/view/scene_renderer.cpp
		src/view/scene_renderer.h
		src/view/starfield.cpp
		src/view/starfield.h
)
target_link_libraries(blackhole_simulator PRIVATE blackhole_core ${DEPS})

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
)
target_link_libraries(tests PRIVATE blackhole_core Catch2::Catch2WithMain)
target_compile_definitions(tests PRIVATE BLACKHOLE_DATA_DIR="${CMAKE_SOURCE_DIR}")
add_test(NAME tests COMMAND tests)

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

# Pure simulation core: depends only on glm, never on OpenGL/GLFW/GLEW.
add_library(blackhole_core STATIC
		# sources
		src/physics.cpp
		src/ray.cpp
		src/simulation.cpp
		# headers
		src/constants.h
		src/physics.h
		src/ray.h
		src/simulation.h
)
target_include_directories(blackhole_core PUBLIC src)
target_link_libraries(blackhole_core PUBLIC glm::glm)

set(SOURCES
		src/main.cpp
		src/render.cpp
		src/scene_renderer.cpp
		src/starfield.cpp
)

set(HEADERS
		src/render.h
		src/scene_renderer.h
		src/starfield.h
)

add_executable(blackhole_simulator
		${SOURCES}
		${HEADERS}
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
		tests/test_physics.cpp
)
target_link_libraries(tests PRIVATE blackhole_core Catch2::Catch2WithMain)
add_test(NAME tests COMMAND tests)

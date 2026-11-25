cmake_minimum_required(VERSION 4.4)
project(blackhole_simulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

set(DEPS glfw GLEW::GLEW glm::glm OpenGL::GL)

set(SOURCES
		src/main.cpp
		src/simulation.cpp
		src/render.cpp
		src/physics.cpp
		src/ray.cpp
		src/scene_renderer.cpp
		src/starfield.cpp
)

set(HEADERS
		src/constants.h
		src/simulation.h
		src/physics.h
		src/render.h
		src/ray.h
		src/scene_renderer.h
		src/starfield.h
)

add_executable(blackhole_simulator
		${SOURCES}
		${HEADERS}
)

target_link_libraries(blackhole_simulator PRIVATE ${DEPS})

# Accuracy and performance comparison of the ray integrators (no OpenGL needed).
# Build it in Release for meaningful timings.
add_executable(integrator_benchmark benchmarks/integrator_benchmark.cpp)

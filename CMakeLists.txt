cmake_minimum_required(VERSION 4.4)
project(blackhole_simulator)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm CONFIG REQUIRED)

set(DEPS glfw GLEW::GLEW glm::glm OpenGL::GL)

add_executable(blackhole_simulator main.cpp)

target_link_libraries(blackhole_simulator PRIVATE ${DEPS})

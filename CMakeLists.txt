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

FetchContent_Declare(
		imgui
		GIT_REPOSITORY https://github.com/ocornut/imgui
		GIT_TAG v1.91.5
)
FetchContent_MakeAvailable(imgui)

# Dear ImGui has no CMake of its own: build the core and the GLFW + OpenGL2 backends.
add_library(imgui STATIC
		${imgui_SOURCE_DIR}/imgui.cpp
		${imgui_SOURCE_DIR}/imgui_draw.cpp
		${imgui_SOURCE_DIR}/imgui_tables.cpp
		${imgui_SOURCE_DIR}/imgui_widgets.cpp
		${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
		${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

# src/ layout (includes are written relative to src/, e.g. "core/ray.h"):
#   core/    physics, rays, simulation, emitters; scene description and settings data — glm only
#   toml/    generic TOML layer: IoError, helpers, field schemas
#   config/  reading and writing config/settings.toml, where the data files live
#   scene/   reading and writing scenario and style files (scenarios/*.toml, config/styles.toml)
#   render/  OpenGL primitives
#   view/    scene drawing: rays, black hole, starfield; its GL-free math (trail colors, view fit)
#   ui/      application controller (GL-free, blackhole_app); Dear ImGui: context and backends, overlay,
#            label texts, control panel, hotkeys
#   app/     entry point

# Pure simulation core: depends only on glm, never on OpenGL/GLFW/GLEW or TOML.
add_library(blackhole_core STATIC
		src/core/constants.h
		src/core/emitter.cpp
		src/core/emitter.h
		src/core/fixed_step_clock.cpp
		src/core/fixed_step_clock.h
		src/core/physics.cpp
		src/core/physics.h
		src/core/ray.cpp
		src/core/ray.h
		src/core/scene_description.cpp
		src/core/scene_description.h
		src/core/settings.h
		src/core/simulation.cpp
		src/core/simulation.h
)
target_include_directories(blackhole_core PUBLIC src)
target_link_libraries(blackhole_core PUBLIC glm::glm)

# Data files: the TOML layer, reading and writing the settings and scenes of core. No OpenGL.
add_library(blackhole_io STATIC
		src/toml/io_error.cpp
		src/toml/io_error.h
		src/toml/toml_schema.cpp
		src/toml/toml_schema.h
		src/toml/toml_util.cpp
		src/toml/toml_util.h
		src/config/data_paths.cpp
		src/config/data_paths.h
		src/config/settings_io.cpp
		src/config/settings_io.h
		src/scene/scene_io.cpp
		src/scene/scene_io.h
)
target_include_directories(blackhole_io PUBLIC src)
target_link_libraries(blackhole_io PUBLIC blackhole_core tomlplusplus::tomlplusplus)
# toml++ turns off std::to_chars for floats on GCC/Clang (written for 2020 standard libraries) and
# falls back to printing max_digits10 digits: 0.0125 becomes 0.012500000000000001. Our toolchains
# have floating-point <charconv> (toTomlFloat already relies on it), so get the shortest form.
target_compile_definitions(blackhole_io PUBLIC TOML_FLOAT_CHARCONV=1)
# Root directory that holds config/ and scenarios/.
target_compile_definitions(blackhole_io PUBLIC BLACKHOLE_DATA_DIR="${CMAKE_SOURCE_DIR}")

# GL-free part of the view layer (colors, projection math), so it can be tested without a window.
add_library(blackhole_view STATIC
		src/view/trail_color.cpp
		src/view/trail_color.h
		src/view/view_fit.cpp
		src/view/view_fit.h
)
target_include_directories(blackhole_view PUBLIC src)
target_link_libraries(blackhole_view PUBLIC blackhole_core)

# Application logic (scenarios, time control) without OpenGL or ImGui, so it can be tested.
add_library(blackhole_app STATIC
		src/ui/app_controller.cpp
		src/ui/app_controller.h
)
target_include_directories(blackhole_app PUBLIC src)
target_link_libraries(blackhole_app PUBLIC blackhole_core blackhole_io)

add_executable(blackhole_simulator
		src/app/main.cpp
		src/render/render.cpp
		src/render/render.h
		src/view/scene_renderer.cpp
		src/view/scene_renderer.h
		src/view/starfield.cpp
		src/view/starfield.h
		src/ui/imgui_layer.cpp
		src/ui/imgui_layer.h
		src/ui/overlay.cpp
		src/ui/overlay.h
		src/ui/control_panel.cpp
		src/ui/control_panel.h
		src/ui/hotkeys.cpp
		src/ui/hotkeys.h
)
target_link_libraries(blackhole_simulator PRIVATE blackhole_core blackhole_io blackhole_view blackhole_app imgui ${DEPS})
# System font from the fonts-dejavu-core package (see prepare.sh), it is not copied into the repository.
# DejaVu Sans has the Cyrillic and the arrow glyphs that the imgui fonts lack.
set(BLACKHOLE_FONT_PATH "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf" CACHE FILEPATH "UI font (TTF)")
target_compile_definitions(blackhole_simulator PRIVATE BLACKHOLE_FONT_PATH="${BLACKHOLE_FONT_PATH}")

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
		tests/core/test_emitter.cpp
		tests/core/test_fixed_step_clock.cpp
		tests/core/test_physics.cpp
		tests/core/test_ray.cpp
		tests/core/test_simulation.cpp
		tests/toml/test_toml_schema.cpp
		tests/config/test_data_paths.cpp
		tests/config/test_settings.cpp
		tests/scene/test_scene_io.cpp
		tests/scene/test_data_files.cpp
		tests/view/test_trail_color.cpp
		tests/view/test_view_fit.cpp
		tests/ui/test_app_controller.cpp
)
target_link_libraries(tests PRIVATE blackhole_core blackhole_io blackhole_view blackhole_app Catch2::Catch2WithMain)
add_test(NAME tests COMMAND tests)

//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <exception>
#include <iostream>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

#include "config/data_paths.h"
#include "config/settings_io.h"
#include "core/fixed_step_clock.h"
#include "core/simulation.h"
#include "render/render.h"
#include "scene/scene_io.h"
#include "view/scene_renderer.h"
#include "view/view_fit.h"

namespace {
	// Problems with the data files are reported and the defaults used, so the window still opens.

	SceneModule::Settings loadSettingsOrDefault(const SceneModule::DataPaths& paths) {
		auto settings = SceneModule::loadSettings(paths.settings());
		if (!settings) {
			std::cerr << SceneModule::formatError(settings.error()) << '\n';
			return {};
		}
		return std::move(*settings);
	}

	SceneModule::StyleMap loadStylesOrEmpty(const SceneModule::DataPaths& paths) {
		auto styles = SceneModule::loadStyles(paths.styles());
		if (!styles) {
			std::cerr << SceneModule::formatError(styles.error()) << '\n';
			return {};
		}
		return std::move(*styles);
	}

	SceneModule::SceneDescription loadSceneOrEmpty(const SceneModule::DataPaths& paths,
	                                               const SceneModule::Settings& settings) {
		auto loaded = SceneModule::loadScene(paths.scenario(settings.view.defaultScenario), settings.world);
		if (!loaded) {
			std::cerr << SceneModule::formatError(loaded.error()) << '\n';
			return {};
		}
		return std::move(loaded->scene);
	}
} // namespace

int main(int argc, char** argv) {
	using namespace RenderModule;
	using namespace SimulationModule;
	using namespace ViewModule;

	try {
		const SceneModule::DataPaths paths = SceneModule::dataPathsFromArgs(argc, argv);
		const SceneModule::Settings settings = loadSettingsOrDefault(paths);
		Simulation simulation(loadSceneOrEmpty(paths, settings), loadStylesOrEmpty(paths), settings);
		FixedStepClock clock(settings.simulation.stepsPerSecond);

		RenderEngine engine(settings.window.width, settings.window.height, "2D Black Hole Simulator");
		SceneRenderer sceneRenderer(settings.view.stars, settings.world);

		float previousTime = RenderEngine::time();
		while (!engine.shouldClose()) {
			const float now = RenderEngine::time();
			const double frameSeconds = now - previousTime;
			previousTime = now;
			for (int i = clock.advance(frameSeconds); i > 0; --i) {
				simulation.step();
			}

			const glm::ivec2 fb = engine.framebufferSize();
			const glm::vec2 half =
				fitView(fb.x, fb.y, {settings.world.halfWidth, settings.world.halfHeight}).halfExtents;
			engine.beginFrame(glm::ortho(-half.x, half.x, -half.y, half.y, -1.f, 1.f));
			sceneRenderer.draw(engine, simulation, now);
			engine.endFrame();
		}
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << '\n';
		return 1;
	}

	return 0;
}

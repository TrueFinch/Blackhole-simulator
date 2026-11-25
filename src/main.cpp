//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <exception>
#include <iostream>

#include "constants.h"
#include "render.h"
#include "scene_renderer.h"
#include "simulation.h"

int main() {
	using namespace RenderModule;
	using namespace SimulationModule;
	using namespace ViewModule;
	using SimulationModule::kWorldHalfWidth;
	using SimulationModule::kWorldHalfHeight;

	try {
		RenderEngine engine(kDefaultWidth, kDefaultHeight,
										"2D Black Hole Simulator");
		Simulation simulation;
		SceneRenderer sceneRenderer(kNumStars, kWorldHalfWidth, kWorldHalfHeight);

		while (!engine.shouldClose()) {
			simulation.update();

			engine.beginFrame(kWorldHalfWidth, kWorldHalfHeight);
			sceneRenderer.draw(engine, simulation, RenderEngine::time());
			engine.endFrame();
		}
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << '\n';
		return 1;
	}

	return 0;
}

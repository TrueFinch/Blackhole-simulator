//
// Created by Vladimir Glushkov on 23.11.2026.
//

#include <exception>
#include <iostream>
#include <string>

#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

#include "config/data_paths.h"
#include "core/simulation.h"
#include "render/render.h"
#include "ui/app_controller.h"
#include "ui/imgui_layer.h"
#include "ui/overlay.h"
#include "view/scene_renderer.h"
#include "view/view_fit.h"

int main(int argc, char** argv) {
	using namespace RenderModule;
	using SimulationModule::Simulation;
	using namespace ViewModule;

	try {
		const SceneModule::DataPaths paths = SceneModule::dataPathsFromArgs(argc, argv);
		// Problems with the data files are reported and the defaults used, so the window still opens.
		UiModule::AppController controller(paths);
		const SceneModule::Settings& settings = controller.settings();
		for (const std::string& error: controller.configErrors()) {
			std::cerr << error << '\n';
		}
		if (controller.lastError()) {
			std::cerr << *controller.lastError() << '\n';
		}
		for (const std::string& warning: controller.warnings()) {
			std::cerr << warning << '\n';
		}

		RenderEngine engine(settings.window.width, settings.window.height, "2D Black Hole Simulator");
		UiModule::ImGuiLayer imgui(engine.nativeWindow(), BLACKHOLE_FONT_PATH);
		SceneRenderer sceneRenderer(settings.view.stars, settings.world);

		float previousTime = RenderEngine::time();
		while (!engine.shouldClose()) {
			const float now = RenderEngine::time();
			const double frameSeconds = now - previousTime;
			previousTime = now;
			controller.update(frameSeconds);
			const Simulation& simulation = controller.simulation();

			const glm::ivec2 fb = engine.framebufferSize();
			const ViewFit fit = fitView(fb.x, fb.y, {settings.world.halfWidth, settings.world.halfHeight});
			const glm::vec2 half = fit.halfExtents;
			imgui.beginFrame();
			engine.beginFrame(glm::ortho(-half.x, half.x, -half.y, half.y, -1.f, 1.f));
			sceneRenderer.draw(engine, simulation, now);

			const float fps = ImGui::GetIO().Framerate;
			UiModule::drawOverlay({
				fps, fps > 0.f ? 1000.f / fps : 0.f, simulation.stepIndex(), simulation.time(),
				static_cast<int>(simulation.rays().size()), simulation.rayLimitHit()
			});
			UiModule::drawLabels(simulation.scene(), fit, fb);
			imgui.endFrame(); // before the swap
			engine.endFrame();
		}
	} catch (const std::exception& e) {
		std::cerr << "Fatal error: " << e.what() << '\n';
		return 1;
	}

	return 0;
}

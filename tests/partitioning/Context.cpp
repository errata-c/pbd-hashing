#include "Context.hpp"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/Window/Event.hpp>

#include <GL/glew.h>

#include "imgui.h"
#include "imgui-SFML.h"

#include <thread>

#include <fmt/core.h>

#include <glm/vec3.hpp>
#include <glm/gtx/color_space.hpp>
#include <glm/gtx/color_encoding.hpp>

Context::Context(int w, int h)
	: mviewport(w, h)
	, mdoc{
		glm::dvec2(0.0, 0.0), // Pan
		1.0 // Zoom factor
	}
{
	// But we can initialize the window
	mwindow.create(sf::VideoMode(w, h), "iplot", 7u, sf::ContextSettings(8, 8, 0, 4, 5));
	mwindow.setVerticalSyncEnabled(true);

	auto size = mwindow.getSize();
	mviewport = { size.x, size.y };

	mrenderer.setViewport(mviewport);
}
Context::~Context() {}

int Context::run() {
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		/* Problem: glewInit failed, something is seriously wrong. */
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
	}

	ImGui::SFML::Init(mwindow);

	sf::Clock deltaClock;
	while (mwindow.isOpen()) {
		sf::Event event;
		while (mwindow.pollEvent(event)) {
			ImGui::SFML::ProcessEvent(mwindow, event);

			if (event.type == sf::Event::Closed) {
				mwindow.close();
			}
			else if (event.type == sf::Event::Resized) {
				auto size = mwindow.getSize();
				mviewport = { size.x, size.y };
				mrenderer.setViewport(mviewport);

				// This took a long time to track down...
				// SFML windows keep the viewport setup they originally started with, they don't update when there is a resize!
				// This means we have to manually reset the viewport if we want that.
				sf::View view = mwindow.getView();
				view.reset(sf::FloatRect(0.f, 0.f, mviewport.x, mviewport.y));
				mwindow.setView(view);
			}
		}

		ImGui::SFML::Update(mwindow, deltaClock.restart());

		ImGui::ShowDemoWindow();

		ImGui::SetNextWindowPos({ -1,0 });
		ImGui::SetNextWindowSize(ImVec2(256, mviewport.y + 1), ImGuiCond_Always);
		if (ImGui::Begin("Context", 0,
			ImGuiWindowFlags_::ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_::ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_::ImGuiWindowFlags_NoResize
		)) {
			ImGui::Button("Look at this pretty button");
		}
		ImGui::End();

		// Could also call the mwindow.clear(); method. Does the same thing, I think.
		// This is just easier to understand for me, because nothing is hidden away behind an abstraction...
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		// Not necessary?
		//glViewport(0, 0, mviewport.x, mviewport.y);

		// Draw things here:
		if (mviewport.x != 0 && mviewport.y != 0) {
			BLContext & ctx = mrenderer.begin_frame(mwindow, mdoc);
			
			// Clear the image.
			ctx.setCompOp(BL_COMP_OP_SRC_COPY);
			ctx.setFillStyle(BLRgba32(0xFFFAFAFA));
			ctx.fillAll();

			// Fill some path.
			BLPath path;
			path.moveTo(26, 31);
			path.cubicTo(642, 132, 587, -136, 25, 464);
			path.cubicTo(882, 404, 144, 267, 27, 31);

			ctx.setCompOp(BL_COMP_OP_SRC_OVER);
			ctx.setFillStyle(BLRgba32(0xFF000000));
			//ctx.setFillStyle(BLRgba32(tmp.r * 255, tmp.g * 255, tmp.b * 255));
			ctx.fillPath(path);
			ctx.fillBox(0,0, 200, 200);

			mrenderer.end_frame(mwindow);
		}

		// Draw the imgui stuff here:
		ImGui::SFML::Render(mwindow);

		// Display the window.
		mwindow.display();

		// This is just to make sure we don't use up all the CPU time doing nothing.
		std::this_thread::sleep_for(std::chrono::milliseconds(8));
	}

	ImGui::SFML::Shutdown();

	return 0;
}
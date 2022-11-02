#pragma once
#include <glm/vec2.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include "Renderer.hpp"
#include "Document.hpp"

class Context {
public:
	Context(int w, int h);
	~Context();

	int run();

private:
	glm::ivec2 mviewport;
	sf::RenderWindow mwindow;

	Document mdoc;

	Renderer mrenderer;
};
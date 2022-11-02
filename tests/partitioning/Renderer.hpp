#pragma once
#include <memory>

#include <glm/vec2.hpp>

#include "Document.hpp"

#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/RenderWindow.hpp>

#include <blend2d.h>

class Renderer {
public:
	Renderer();
	~Renderer();

	glm::ivec2 getViewport();
	void setViewport(const glm::ivec2& _view);
	
	BLContext& begin_frame(sf::RenderWindow& window, const Document & doc);

	void end_frame(sf::RenderWindow& window);
private:
	// The current viewport, and the reserved texture memory.
	// The texture reserve is to make it possible to resize the window quickly without tons of unneeded allocs.
	glm::ivec2 mviewport, mreserve;

	sf::Texture mtexture;

	// Mapping from world space to viewport
	// divide by 2 * zoom, add 0.5, flip the y axis (1 - y), multiply by the viewport size.
	glm::dvec2 halfView, invZoom, offset;
	void prepareTransform(const Document& doc);
	glm::dvec2 transform(const glm::dvec2& point);

	BLContext ctx;
	BLImage img;
};
#include "Renderer.hpp"

#include <blend2d.h>

#include <fmt/core.h>

Renderer::Renderer()
	: mviewport(0, 0)
	, mreserve(0, 0)
{}
Renderer::~Renderer()
{}

void Renderer::prepareTransform(const Document& doc) {
	halfView = glm::dvec2(mviewport.x, mviewport.y) / 2.0;
	invZoom = 1.0 / glm::dvec2(doc.zoom);
	offset = -doc.pan;
}
glm::dvec2 Renderer::transform(const glm::dvec2& point) {
	glm::dvec2 tmp = point + offset;
	tmp *= invZoom;

	tmp.x = 1.0 + tmp.x;
	tmp.y = 1.0 - tmp.y;
	return tmp * halfView;
}

glm::ivec2 Renderer::getViewport() {
	return mviewport;
}
void Renderer::setViewport(const glm::ivec2& _view) {
	// Make sure we don't do anything stupid.
	if (_view.x == 0 || _view.y == 0) {
		return;
	}

	
	BLResult result = img.reset();
	assert(result == BL_SUCCESS);
	result = img.create(_view.x, _view.y, BL_FORMAT_PRGB32);
	assert(result == BL_SUCCESS);

	mviewport = _view;
	mreserve = mviewport;
	return;

	if (mreserve.x == 0 || mreserve.y == 0) {
		// Initialize the texture memory.
		if (!mtexture.create(_view.x, _view.y)) {
			assert(false);
		}
		//pixels.reset(new char[_view.x * _view.y * 4]);

		mviewport = _view;
		mreserve = mviewport;
	}
	else if (_view.x > mreserve.x || _view.y > mreserve.y) {
		// Update the reserve storage.
		// Multiply the mreserve size by 1.5, if the new size is too small just pick the desired size directly.
		glm::ivec2 cap = mreserve;
		if (_view.x > mreserve.x) {
			cap.x = std::max(double(_view.x), cap.x * 1.5);
		}
		if (_view.y > mreserve.y) {
			cap.y = std::max(double(_view.y), cap.y * 1.5);
		}

		if (!mtexture.create(cap.x, cap.y)) {
			assert(false);
		}
		//pixels.reset(new char[cap.x * cap.y * 4]);

		mviewport = _view;
		mreserve = cap;
	}
	else {
		// No reallocations needed.
		mviewport = _view;
	}
}

BLContext& Renderer::begin_frame(sf::RenderWindow& window, const Document& doc) {
	ctx.reset();
	BLResult result = ctx.begin(img);
	assert(result == BL_SUCCESS);

	// Set the context's transform such that the view is centered on the 

	return ctx;
}
void Renderer::end_frame(sf::RenderWindow& window) {
	BLResult result = ctx.end();
	assert(result == BL_SUCCESS);

	BLImageData idata;
	result = img.getData(&idata);
	assert(result == BL_SUCCESS);

	if (!mtexture.create(mviewport.x, mviewport.y)) {
		assert(false);
	}
	mtexture.update((const sf::Uint8*)idata.pixelData);
	
	sf::Sprite msprite;
	msprite.setTexture(mtexture, true);

	window.draw(msprite);
}

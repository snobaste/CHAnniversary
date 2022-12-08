#pragma once

#include "Rendering/LoadingIndicator.hpp"

#include "Book.hpp"

using namespace SnobasteCPP;

class Renderer;
class Loading {
public:
	Loading(Renderer *renderer);

	void Init(float width, float height);
	void Resize(float width, float height);

	void Draw(float delta, float x, float y);
	void Cleanup();

	void Reset();

	bool Initialized() const { return initialized; }
	float GetMaxRadius() const { return indicator.GetMaxRadius(); }
private:
	Renderer *renderer;

	LoadingIndicator indicator;

	bool initialized = false;

	Book::Page::Image image;
};
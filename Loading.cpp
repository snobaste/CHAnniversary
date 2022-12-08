#include "Loading.hpp"

#include "Filesystem/FileRepository.hpp"
#include "third_party/lodepng/lodepng.h"

#include "Renderer.hpp"

Loading::Loading(Renderer *renderer) :
	renderer(renderer) {
	/*
	image.relativePath = "Images/utuutsu.png";
	lodepng::decode(
		image.data,
		image.width,
		image.height,
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / image.relativePath).string()
	);
	*/
}

void Loading::Init(float width, float height) {
	indicator.InitializeOpenGL(width, height);
	indicator.SetObjectScale(1.0f);
	indicator.GetFont().SetColor(Color::Black);
	//renderer->LoadTexture(image);
	//image.Scale(width, height);
	initialized = true;
}

void Loading::Resize(float width, float height) {
	indicator.SetWindowSize(width, height);
	//image.Scale(width, height);
}

void Loading::Draw(float delta, float x, float y) {
	//renderer->RenderTexture(image);
	indicator.Draw(delta, x + renderer->GetWidth() / 2.0f, y + renderer->GetHeight() / 2.0f);
}
void Loading::Cleanup() {
	indicator.Cleanup();
}

void Loading::Reset() {
	indicator.Reset();
}
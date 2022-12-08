#pragma once

#include <memory>

#include "Book.hpp"

class Renderer;
class Curl {
public:
	enum class CurlDir {
		Left,
		Right
	};

	Curl(Renderer *renderer);
	~Curl() = default;

	void Init();
	void Resize(int width, int height);

	void Start(CurlDir curlDir, std::optional<std::function<void()>> callback = std::nullopt);
	void Stop(bool withCallback = false);
	void Update();
	void Render(GLuint texture, float x, float y, float *vertBuffer, const std::function<void(GLuint, float *, float *)> &f, float deltaTime, std::optional<float> fulcrum = std::nullopt, bool cover = false, bool foreward = false, std::optional<std::reference_wrapper<std::unique_ptr<Book::Page::Image>>> back = std::nullopt);

	void Cleanup();

	bool IsAnimating() const { return animationState != AnimationState::None; }

private:
	Renderer *renderer;

	float vertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	float backVertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	float frameBufferVertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	float textureBuffer[8] = { 0, 1, 0, 0, 1, 0, 1, 1 };

	float shadowVertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	float shadowTextureBuffer[8] = { 0, 0, 0, 1, 1, 1, 1, 0 };

	float textureBufferReverse[8] = { 1.0f, 0, 1.0f, 1, 0.0f, 1, 0.0f, 0 };

	Book::Page::Image rightPageShadow;

	float angle = 0.0f;
	float shadowAngle = 0.0f;

	enum class AnimationState {
		None,
		FirstHalf,
		SecondHalf
	};
	AnimationState animationState = AnimationState::None;

	Book::Page::Image rightPage;
	Book::Page::Image leftPage;
	Book::Page::Image leftPageMiddle;
	Book::Page::Image leftPageOccupied;

	std::optional<std::function<void()>> callback = std::nullopt;

	CurlDir curlDir = CurlDir::Right;

	bool renderedFinalFrame = false;
};
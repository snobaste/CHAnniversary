#include "Curl.hpp"

#include "Renderer.hpp"

constexpr float DEG2RAD = 3.14159f / 180.0f;

Curl::Curl(Renderer *renderer) :
	renderer(renderer) {
	rightPage.relativePath = "Images/rightpage.png";
	leftPage.relativePath = "Images/leftpage.png";
	leftPageMiddle.relativePath = "Images/leftpagemiddle.png";
	leftPageOccupied.relativePath = "Images/leftpageoccupied.png";
	rightPageShadow.relativePath = "Images/rightpageshadow.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / rightPageShadow.relativePath).string().c_str(),
		rightPageShadow.data,
		rightPageShadow.width,
		rightPageShadow.height,
		rightPageShadow.channels,
		4
	);
}

void Curl::Init() {
	//renderer->LoadTexture(rightPage);
	renderer->LoadTexture(rightPageShadow);
}

void Curl::Resize(int width, int height) {
	vertexBuffer[3] = shadowVertexBuffer[3] = height;
	vertexBuffer[4] = shadowVertexBuffer[4] = width;
	vertexBuffer[5] = shadowVertexBuffer[5] = height;
	vertexBuffer[6] = shadowVertexBuffer[6] = width;
}

void Curl::Start(CurlDir curlDir, std::optional<std::function<void()>> callback) {
	this->curlDir = curlDir;

	angle = 0.0f;
	shadowAngle = 0.0f;
	animationState = AnimationState::FirstHalf;
	this->callback = callback;
}

void Curl::Stop(bool withCallback) {
	animationState = AnimationState::None;

	if (withCallback && callback)
		callback.value()();

	callback = std::nullopt;
}

void Curl::Update() {

}

void Curl::Render(GLuint texture, float x, float y, float *vertBuffer, const std::function<void(GLuint, float *, float *)> &f, float deltaTime, std::optional<float> fulcrum, bool cover, bool foreward, std::optional<std::reference_wrapper<std::unique_ptr<Book::Page::Image>>> back) {
	memcpy(frameBufferVertexBuffer, vertBuffer, sizeof(float) * 8);

	glTranslatef(fulcrum ? *fulcrum : x, y, 0.0f);

	// Cast shadow before rotation
		//glRotatef(shadowAngle, 0.0f, 1.0f, 0.0f);
	shadowAngle = angle - 5.0f;
	

	//if (shadowAngle > 0.0f) {
	shadowTextureBuffer[0] = shadowTextureBuffer[2] = (shadowAngle / 90.0f);
	shadowTextureBuffer[4] = shadowTextureBuffer[6] = 1.0f - (shadowAngle / 90.0f);
	shadowVertexBuffer[4] = shadowVertexBuffer[6] = frameBufferVertexBuffer[4] * (1.0f - shadowAngle / 90.0f);
	
	if (animationState == AnimationState::FirstHalf) {
		glRotatef(curlDir == CurlDir::Right ? shadowAngle : 180.0f - shadowAngle, 0.0f, 1.0f, 0.0f);

		glColor4f(1.0f, 1.0f, 1.0f, std::clamp(sin(DEG2RAD * angle) * 2.0f, 0.0f, 1.0f));

		renderer->RenderTexture(rightPageShadow, shadowVertexBuffer, shadowTextureBuffer, true);

		glTranslatef(fulcrum ? *fulcrum : x, y, 0.0f);
		//}

		glRotatef(angle, 0.0f, 1.0f, 0.0f);

		if (fulcrum)
			glTranslatef(x, 0.0f, 0.0f);

		if (curlDir == CurlDir::Right)
			frameBufferVertexBuffer[4] = frameBufferVertexBuffer[6] = (vertBuffer[4]) * (1.0f - sin(DEG2RAD * angle));
		else
			frameBufferVertexBuffer[0] = frameBufferVertexBuffer[2] = (vertBuffer[4]) * (sin(DEG2RAD * angle));
		//textureBuffer[0] = textureBuffer[2] = (1.0f - ((angle - 1.0f) / 90.0f));

		f(texture, frameBufferVertexBuffer, textureBuffer);
	} else {
		glRotatef(curlDir == CurlDir::Right ? (90.0f + (90.0f - shadowAngle)) : shadowAngle, 0.0f, 1.0f, 0.0f);

		glColor4f(1.0f, 1.0f, 1.0f, std::clamp(sin(DEG2RAD * angle) * 2.0f, 0.0f, 1.0f));
		renderer->RenderTexture(rightPageShadow, shadowVertexBuffer, shadowTextureBuffer, true);

		glTranslatef(fulcrum ? *fulcrum : x + 1 /* Avoid subpixel weirdness */, y, 0.0f);

		// After 90 degrees, abandon FBO and render blank page
		glRotatef(curlDir == CurlDir::Right ? (90.0f + (90.0f - angle)) : angle, 0.0f, 1.0f, 0.0f);

		if (back) {
			backVertexBuffer[3] = backVertexBuffer[5] = ((*back).get())->scaledHeight;
			backVertexBuffer[4] = backVertexBuffer[6] = ((*back).get())->scaledWidth;
		}

		renderer->RenderTexture(
			(back ? *((*back).get()) : cover ? (foreward ? leftPage : leftPageMiddle) : (curlDir == CurlDir::Right ? leftPageOccupied : rightPage)),
			cover ? vertBuffer : back ? backVertexBuffer : vertexBuffer,
			back ? textureBufferReverse : Renderer::GetTextureBuffer()
		);
	}

	glLoadIdentity();

	if (angle == 0.0f)
		angle = 1.0f;

	if (animationState == AnimationState::FirstHalf) {
		angle *= 1.0f + ((deltaTime / 0.0015f) * 0.01f);
	} else {
		if (angle > 1.0f)
			angle /= 1.0f + ((deltaTime / 0.0015f) * 0.01f);

		// Switch to subtracting after passing 1.0 to
		// avoid Zeno problem.
		else
			angle -= ((deltaTime / 0.0015f) * 0.01f);

		if (angle < 0.0f)
			angle = 0.0f;
	}

	if (angle >= 90.0f) {
		// For now, stop the animation
		animationState = AnimationState::SecondHalf;
	} else if (angle <= 0.0f && animationState == AnimationState::SecondHalf) {
		if (!renderedFinalFrame) {
			renderedFinalFrame = true;
		} else {
			animationState = AnimationState::None;

			if (callback)
				callback.value()();

			callback = std::nullopt;
		}
	}
}

void Curl::Cleanup() {

}
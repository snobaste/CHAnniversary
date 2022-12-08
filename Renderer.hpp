#pragma once

#include <memory>

#include "Filesystem/FileRepository.hpp"
#include "Rendering/OpenGLFont.hpp"

#include "Book.hpp"
#include "Curl.hpp"
#include "Ease.hpp"
#include "GhostWriter.hpp"
#include "Loading.hpp"

using namespace SnobasteCPP;

class Engine;
class Renderer : public LoggableClass {
public:
	explicit Renderer(Engine *engine, std::shared_ptr<Book> book = nullptr);
	virtual ~Renderer();

	void Init();

	void Resize(int width, int height);

	bool Render();

	void Cleanup();

	void SetBook(std::shared_ptr<Book> book);
	void SetPage(std::size_t currentPage, bool threaded = false);
	void AdvancePage(bool force = false, bool reverse = false);

	void SetDeltaTime(float deltaTime) { this->deltaTime = deltaTime; }

	void LoadTexture(Book::Page::Image &image);
	void RenderTexture(const Book::Page::Image &image, float *vertexBuffer = nullptr, float *textureBuffer = Renderer::textureBuffer, bool color = false);

	int GetWidth() const { return width; }
	int GetHeight() const { return height; }

	void Back();

	Curl &GetCurl() { return curl; }

	void OnMouseClicked(double x, double y, int button, int mods);

	float *GetVertexBuffer() { return imageVertexBuffer; }
	const unsigned short *GetIndexBuffer() const { return indexBuffer; }
	static float *GetTextureBuffer() { return textureBuffer; }

	const auto GetMargin() const { return margin; }

	GLuint GetImage(const std::string &path) const { return images.at(path); }

	const Book::Page::Image &GetBackground() const { return background; }
	const Book::Page::Image &GetForewardBackground() const { return forewardBackground; }

	const std::function<void(GLuint, float *, float *)> GetCurlRenderCallback(float *textureBuffer = nullptr);

private:
	enum class WritingState {
		Header,
		Paragraph,
		Image,
		Done,
		Close,
		Move,
		Back
	};

	void UpdateBook();
	void UpdatePages();

	void AdvanceParagraph();

	void Reset(bool threaded = false);

	float AdvanceAudio();

	inline void UnbindFramebuffer() const;

	void StartHeaderAnimation();
	void StartImageAnimation();

	void FinishHeaderAnimation(int i, std::reference_wrapper<Book::Page> page);
	void FinishPage();

	Engine *engine = nullptr;

	int width = 0, height = 0;

	std::unique_ptr<OpenGLFont> headerFont;
	std::unique_ptr<OpenGLFont> footerFont;
	std::map<std::string, std::unique_ptr<OpenGLFont>> fonts;
	std::atomic<bool> bookUpdated = false;
	std::shared_ptr<Book> book = nullptr;
	std::size_t currentPage = 0;
	std::size_t margin = 64;

	std::map<std::string, GLuint> images;

	float backgroundVertexBuffer[8];
	float imageVertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static float textureBuffer[8];
	static float flipHorizontalTextureBuffer[8];
	unsigned short indexBuffer[6] = { 0, 1, 2, 0, 2, 3 };

	Book::Page::Image background;
	Book::Page::Image forewardBackground;

	std::map<uint32_t, OpenGLFont::SpanItem> spans;

	float deltaTime = 0.0f;

	std::optional<std::pair<std::size_t, std::size_t>> currentPos = std::nullopt;
	GhostWriter writer;

	WritingState writingState = WritingState::Header;

	// Render vars
	std::vector<std::reference_wrapper<Book::Page>> pages;

	std::unique_ptr<Ease<float>> ease;

	Loading loading;
	Curl curl;

	Book::Page::Image rightPage;
	Book::Page::Image leftPage;
	Book::Page::Image leftPageMiddle;
	Book::Page::Image leftPageOccupied;

	float framebufferVertexBuffer[8];
	float framebufferTextureBuffer[8] = { 0, 1, 0, 0, 1, 0, 1, 1 };

	uint8_t *pixels = nullptr;

	unsigned int framebuffers[2] = { 0, 0 };
	unsigned int textures[2] = { 0, 0 };

	bool paused = false;
	bool reset = false;

	Curl::CurlDir curlDir = Curl::CurlDir::Right;

	float headerAlpha = 0.0f;
	OpenGLFont::FontGlyph headerBounds;

	OpenGLFont debugFont;
	double totalFrametime = 0.0;
	std::size_t frametimes = 0;
	std::string fps;
	void DrawFPS();

	float backgroundAlpha = 1.0f;
	std::unique_ptr<Ease<float>> backgroundEase;

	std::optional<std::chrono::milliseconds> nextPageStart = std::nullopt;

	std::array<float, 361 * 2> circleVertexBuffer;
	std::array<uint16_t, 360 * 3> circleIndexBuffer;
	std::array<float, 361 * 2> circleTexCoordBuffer;

	static float halfTextureBufferReverse[8];

	std::unique_ptr<Ease<float>> audioFadeEase;
	std::atomic<bool> skipFirstPage = false;
};
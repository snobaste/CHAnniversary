#include "Renderer.hpp"

#include <sstream>

#include "Utils/Enumerate.hpp"
#include "Utils/GlobalViewerUtils.hpp"

#include "Defines.hpp"
#include "Engine.hpp"
#include "Markdown.hpp"

float Renderer::textureBuffer[8] = { 0, 0, 0, 1, 1, 1, 1, 0 };
float Renderer::flipHorizontalTextureBuffer[8] = { 1, 0, 1, 1, 0, 1, 0, 0 };

float Renderer::halfTextureBufferReverse[8] = {
	0.5f,
	0.0f,
	0.5f,
	1.0f,
	1.0f,
	1.0f,
	1.0f,
	0.0f
};

Renderer::Renderer(Engine *engine, std::shared_ptr<Book> book) :
	engine(engine),
	book(book),
	loading(this),
	curl(this) {

}

Renderer::~Renderer() {

}

void Renderer::Init() {
	for (unsigned short i = 2; i < 360; i++) {
		circleIndexBuffer[((i - 2) * 3)] = 0;
		circleIndexBuffer[((i - 2) * 3) + 1] = i - 1;
		circleIndexBuffer[((i - 2) * 3) + 2] = i;
	}

	circleIndexBuffer[((360 - 2) * 3)] = 0;
	circleIndexBuffer[((360 - 2) * 3) + 1] = 359;
	circleIndexBuffer[((360 - 2) * 3) + 2] = 1;
	circleIndexBuffer[((361 - 2) * 3)] = 0;
	circleIndexBuffer[((361 - 2) * 3) + 1] = 1;
	circleIndexBuffer[((361 - 2) * 3) + 2] = 2;

	circleVertexBuffer[0] = 0;
	circleVertexBuffer[1] = 0;

	circleTexCoordBuffer[0] = 0.5;
	circleTexCoordBuffer[1] = 0.5;

	float degInRad;
	for (int i = 0; i < 360; i++) {
		degInRad = i * DEG2RAD;
		circleVertexBuffer[(i * 2)] = static_cast<float>(sin(degInRad));
		circleVertexBuffer[(i * 2) + 1] = static_cast<float>(cos(degInRad));
	}

	footerFont = std::make_unique<OpenGLFont>("Fonts/EBGaramond");
	footerFont->InitFont();
	footerFont->SetScale(0.60f);
	footerFont->SetColor(Color::Black);
	background.relativePath = "Images/book.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / background.relativePath).string().c_str(),
		background.data,
		background.width,
		background.height,
		background.channels,
		4
	);
	background.UpdateRatio();
	LoadTexture(background);

	forewardBackground.relativePath = "Images/book_foreward.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / forewardBackground.relativePath).string().c_str(),
		forewardBackground.data,
		forewardBackground.width,
		forewardBackground.height,
		forewardBackground.channels,
		4
	);
	forewardBackground.UpdateRatio();
	LoadTexture(forewardBackground);

	rightPage.relativePath = "Images/rightpage.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / rightPage.relativePath).string().c_str(),
		rightPage.data,
		rightPage.width,
		rightPage.height,
		rightPage.channels,
		4
	);
	LoadTexture(rightPage);

	leftPage.relativePath = "Images/leftpage.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / leftPage.relativePath).string().c_str(),
		leftPage.data,
		leftPage.width,
		leftPage.height,
		leftPage.channels,
		4
	);
	LoadTexture(leftPage);

	leftPageMiddle.relativePath = "Images/leftpagemiddle.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / leftPageMiddle.relativePath).string().c_str(),
		leftPageMiddle.data,
		leftPageMiddle.width,
		leftPageMiddle.height,
		leftPageMiddle.channels,
		4
	);
	LoadTexture(leftPageMiddle);

	leftPageOccupied.relativePath = "Images/leftpageoccupied.png";
	auto loaded = fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / leftPageOccupied.relativePath).string().c_str(),
		leftPageOccupied.data,
		leftPageOccupied.width,
		leftPageOccupied.height,
		leftPageOccupied.channels,
		4
	);
	LoadTexture(leftPageOccupied);

	debugFont.InitFont();
	debugFont.SetScale(0.60f);

	engine->GetMenu()->Init();
	curl.Init();
}

void Renderer::Resize(int width, int height) {
	this->width = width;
	this->height = height;

	// Load viewport and matrix
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, width, height, 0, -100.0f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glViewport(0, 0, width * 1.0f, height * 1.0f);

	// Scale the background accordingly
	background.Scale(width, height);
	curl.Resize(background.scaledWidth / 2.0f, background.scaledHeight);
	rightPage.scaledWidth = background.scaledWidth / 2.0f;
	rightPage.scaledHeight = background.scaledHeight;

	leftPage.scaledWidth = background.scaledWidth / 2.0f;
	leftPage.scaledHeight = background.scaledHeight;

	leftPageMiddle.scaledWidth = background.scaledWidth / 2.0f;
	leftPageMiddle.scaledHeight = background.scaledHeight;

	leftPageOccupied.scaledWidth = background.scaledWidth / 2.0f;
	leftPageOccupied.scaledHeight = background.scaledHeight;

	forewardBackground.Scale(width, height);

	// Reset font scale
	if (headerFont)
		headerFont->SetScale(1.0f);
	for (auto &[path, font] : fonts)
		font->SetScale(1.0f);

	if (!loading.Initialized())
		loading.Init(width, height);
	else
		loading.Resize(width, height);

	if (book && book->GetBack())
		book->GetBack()->Scale(width / 2.0f, height);

	engine->GetMenu()->Resize();

#if defined(SNOBASTE_GL)
	glDeleteFramebuffersEXT(2, framebuffers);
#elif defined(SNOBASTE_GLES)
	glDeleteFramebuffersOES(2, framebuffers);
#endif

	/*
	delete[] pixels;
	pixels = new uint8_t[width / 2.0f * height * 4];
	*/

	framebufferVertexBuffer[0] = 0;
	framebufferVertexBuffer[1] = 0;
	framebufferVertexBuffer[2] = 0;
	framebufferVertexBuffer[3] = height;
	framebufferVertexBuffer[4] = width / 2.0f;
	framebufferVertexBuffer[5] = height;
	framebufferVertexBuffer[6] = width / 2.0f;
	framebufferVertexBuffer[7] = 0;

	const auto generateFramebufferTexture = [&](GLuint texture) {
		glBindTexture(GL_TEXTURE_2D, texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width / 2.0f, height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, nullptr);
	};

	glEnable(GL_TEXTURE_2D);
	glDeleteTextures(2, textures);
	glGenTextures(2, textures);
	generateFramebufferTexture(textures[0]);
	generateFramebufferTexture(textures[1]);
	glBindTexture(GL_TEXTURE_2D, 0);

	// Create FBO for render-to-texture.
#if defined(SNOBASTE_GL)
	glGenFramebuffersEXT(2, framebuffers);
#elif defined(SNOBASTE_GLES)
	glGenFramebuffersOES(2, framebuffers);
#endif

	const auto bindFramebufferToTexture = [&](GLuint texture, GLuint framebuffer) {
#if defined(SNOBASTE_GL)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffer);
#elif defined(SNOBASTE_GLES)
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffer);
#endif

#if defined(SNOBASTE_GL)
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT
#elif defined(SNOBASTE_GLES)
		glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES
#endif
			, GL_TEXTURE_2D, texture, 0);

#if defined(SNOBASTE_GL)
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#elif defined(__APPLE__)
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, 1);
#elif defined(__ANDROID__)
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
#endif
	};

	bindFramebufferToTexture(textures[0], framebuffers[0]);
	bindFramebufferToTexture(textures[1], framebuffers[1]);

	glDisable(GL_TEXTURE_2D);
}

void Renderer::LoadTexture(Book::Page::Image &image) {
	// Already cached or empty
	if (images.find(image.relativePath) != images.end() || image.data.empty()) {
		// See https://cplusplus.com/reference/vector/vector/clear/
		std::vector<uint8_t>().swap(image.data);
		return;
	}

	glEnable(GL_TEXTURE_2D);

	GLuint textureId;
	glGenTextures(1, &textureId);

	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, &image.data[0]);
	glBindTexture(GL_TEXTURE_2D, 0);

	glDisable(GL_TEXTURE_2D);

	images[image.relativePath] = textureId;

	// See https://cplusplus.com/reference/vector/vector/clear/
	std::vector<uint8_t>().swap(image.data);
}

void Renderer::SetBook(std::shared_ptr<Book> book) { 
	this->book = book; currentPage = 0;
	currentPos = std::nullopt;

	bookUpdated = true;
}

void Renderer::SetPage(std::size_t currentPage, bool threaded) {
	// Round to last odd (for now)
	if (currentPage != 0) {
		if (currentPage % 2) {
			this->currentPage = currentPage;
			skipFirstPage = false;
		} else {
			this->currentPage = currentPage - 1;

			// If we're odd, does the page before us
			// only have an image? If so, retain the
			// image animation.
			if (const auto &page = book.get()->GetPages()[currentPage - 1]; !page.image || !page.paragraphs.empty()) {
				skipFirstPage = true;
			}
		}
	} else {
		this->currentPage = currentPage;
	}

	Reset(threaded);
}

void Renderer::UpdateBook() {
	bookUpdated = false;

	if (!book) return;

	std::vector<OpenGLFont::SpanItem> headerSpanItems{
		{ OpenGLFont::Style::Regular },
		{ OpenGLFont::Style::BoldItalic }
	};

	std::vector<OpenGLFont::SpanItem> spanItems{
		{ OpenGLFont::Style::Regular }
	};
	std::set<std::string> fontPaths;

	// Load fonts
	// Load images into textures
	for (auto &page : book->GetPages()) {
		if (!page.second.font.empty()) {
			fontPaths.emplace(page.second.font);
		}

		if (!page.second.titleStyle.empty()) {
			page.second.style = magic_enum::enum_cast<OpenGLFont::Style>(page.second.titleStyle).value();
			headerSpanItems.emplace_back(OpenGLFont::SpanItem{
				page.second.style
				});
		}

		if (auto &image = page.second.image) {
			LoadTexture(*image);
		}
	}

	// Load back if it exists
	if (book->GetBack())
		LoadTexture(*book->GetBack());

	headerFont = std::make_unique<OpenGLFont>(
		FileRepository::registry->GetResourceDirectory() / "Fonts" / "Roboto",
		headerSpanItems
		);
	headerFont->InitFont();
	headerFont->SetColor(Color::Black);

	// Gather styles
	for (const auto &[hash, style] : book->GetStyles()) {
		auto parsed = Markdown::GetSpanForMarkdown(style);
		if (parsed) {
			spanItems.emplace_back(*parsed);
			spans.emplace(
				std::make_pair(
					hash, *parsed
				)
			);
		}
	}

	// Clear any existing fonts
	for (auto &font : fonts) {
		font.second->KillFont();
	}
	fonts.clear();

	for (const auto &font : fontPaths) {
		auto iter = fonts.emplace(
			std::make_pair(
				font,
				std::make_unique<OpenGLFont>(
					FileRepository::registry->GetResourceDirectory() / font,
					spanItems
					)
			)
		).first;
		iter->second->InitFont();
		iter->second->SetColor(Color::Black);
	}

	UpdatePages();
}

void Renderer::StartHeaderAnimation() {
	writingState = WritingState::Header;
	headerAlpha = 0.0f;
	ease = std::make_unique<Ease<float>>(0.0f, 1.0f, 1.0f);
}

void Renderer::StartImageAnimation() {
	writingState = WritingState::Image;
	ease = std::make_unique<Ease<float>>(0.0f, 1.0f, 1.5f);
}

void Renderer::UpdatePages() {
	// Render current two pages
	pages.clear();
	if (auto page = book->GetPages().find(currentPage); page != book->GetPages().end()) {
		pages.emplace_back(page->second);

		if (currentPage != 0 && ++page != book->GetPages().end())
			pages.emplace_back(page->second);
	}

	StartHeaderAnimation();
}

void Renderer::OnMouseClicked(double x, double y, int button, int mods) {
	if (writingState >= WritingState::Close) {
		if (writingState == WritingState::Back) Back();
		return;
	}

	// Did the user click on the book? If so, advance in the current page
	if (x > width / 2.0f - background.scaledWidth / 2.0f &&
		x < width / 2.0f + background.scaledWidth / 2.0f &&
		y > height / 2.0f - background.scaledHeight / 2.0f &&
		y < height / 2.0f + background.scaledHeight / 2.0f) {
		AdvancePage();
	} else {
		// If the user clicked beyond the book, go to previous / next page
		if (x < width / 2.0f - background.scaledWidth / 2.0f)
			AdvancePage(true, true);
		else if (x > width / 2.0f + background.scaledWidth / 2.0f)
			AdvancePage(true);
	}
}

void Renderer::AdvancePage(bool force, bool reverse) { 
	nextPageStart = std::nullopt;
	skipFirstPage = false;

	if (!book || writingState >= WritingState::Close) return;

	if (writingState == WritingState::Done || force) {
		auto offset = reverse ? -2 : 2;
		if (currentPage == 0 || (reverse && currentPage == 1 && book->GetPages().find(0) != book->GetPages().end()))
			offset /= 2;

		// Do we have any more pages?
		if (!reverse && currentPage + offset >= book->GetPages().size()) {
			if (book->GetBack()) {
				writingState = WritingState::Close;
				curlDir = Curl::CurlDir::Right;
				book->GetBack()->Scale(width / 2.0f, height);
				curl.Start(curlDir, [&] {
					writingState = WritingState::Move;
					ease = std::make_unique<Ease<float>>(0.0f, 1.0f, 1.0f);
				});
			}
			return;
		} else if (reverse && (currentPage == 0 || (currentPage == 1 && book->GetPages().find(0) == book->GetPages().end()))) {
			return;
		}

		const auto nextPage = [&, offset] {
			if (currentPage == 0) {
				backgroundAlpha = 0.0f;
				backgroundEase = std::make_unique<Ease<float>>(0.0f, 1.0f, 1.0f);
			}

			currentPage += offset;
			Reset();
		};

		// TODO: Do we really need to pause?
		//paused = true;
		if (curl.IsAnimating()) {
			curl.Stop(true);
			audioFadeEase.reset();
		} else {
			curlDir = reverse ? Curl::CurlDir::Left : Curl::CurlDir::Right;
			curl.Start(curlDir, nextPage);
			audioFadeEase = std::make_unique<Ease<float>>(0.0f, 1.0f, 1.5f);
		}
	} else if (writingState == WritingState::Header) {
		FinishHeaderAnimation(currentPos->first, pages[currentPos->first]);
	} else {
		AdvanceParagraph();
	}
}

void Renderer::Reset(bool threaded) {
	audioFadeEase.reset();
	engine->GetAudio()->Reset();
	curl.Stop();
	paused = false;
	currentPos = std::nullopt;

	if (!threaded) {
		// Reset font scale
		if (headerFont)
			headerFont->SetScale(1.0f);
		for (auto &[path, font] : fonts)
			font->SetScale(1.0f);
	} else {
		reset = true;
	}

	UpdatePages();
}

void Renderer::RenderTexture(const Book::Page::Image &image, float *vertexBuffer, float *textureBuffer, bool color) {
	if (!color)
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, images[image.relativePath]);

	if (!vertexBuffer) {
#ifdef DEBUG
		if (image.scaledWidth > 20000 || image.scaledHeight > 20000)
			throw std::runtime_error("Please scale your images first");
#endif
		imageVertexBuffer[3] = image.scaledHeight;
		imageVertexBuffer[4] = image.scaledWidth;
		imageVertexBuffer[5] = image.scaledHeight;
		imageVertexBuffer[6] = image.scaledWidth;

		glVertexPointer(2, GL_FLOAT, 0, imageVertexBuffer);
	} else {
		glVertexPointer(2, GL_FLOAT, 0, vertexBuffer);
	}
	glTexCoordPointer(2, GL_FLOAT, 0, textureBuffer);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexBuffer);
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
}

float Renderer::AdvanceAudio() {
	auto &page = pages[currentPos->first].get();

	// Does this page / paragraph have sound?
	// If so, load it.
	float duration = -1.0f;
	audioFadeEase.reset();
	if (!page.sounds.empty() && currentPos->second < page.sounds.size() && engine->GetAudio()->Load(page.sounds[currentPos->second])) {
		duration = engine->GetAudio()->GetDuration();
		logger.WriteDebug("Duration: ", duration, "s");
	} else {
		// Make sure to stop the currently-playing
		// sound, if there is one
		engine->GetAudio()->Reset();
	}

	return duration;
}

void Renderer::AdvanceParagraph() {
	// Are we in the header?
	if (writingState == WritingState::Header) {
		writingState = WritingState::Paragraph;
		return;
	}

	if (++currentPos->second >= pages[currentPos->first].get().paragraphs.size() && (!pages[currentPos->first].get().image || writingState == WritingState::Image)) {
		++currentPos->first;
		currentPos->second = 0;

		if (currentPos->first < pages.size() && pages[currentPos->first].get().title.empty()) {
			writingState = WritingState::Paragraph;
			ease.reset();
		} else {
			StartHeaderAnimation();
		}
	}

	// Do we have another paragraph?
	if (currentPos->first >= pages.size() || currentPos->second >= pages[currentPos->first].get().paragraphs.size()) {
		engine->GetAudio()->Stop();

		// Do we have an image?
		if (currentPos->first < pages.size() && pages[currentPos->first].get().image) {
			StartImageAnimation();
		} else {
			FinishPage();
		}

		logger.WriteDebug("Setting current writingState to ", magic_enum::enum_name(writingState));
	} else {
		writer.SetText(pages[currentPos->first].get().paragraphs[currentPos->second], AdvanceAudio());
	}
}

void Renderer::FinishHeaderAnimation(int i, std::reference_wrapper<Book::Page> page) {
	if (i == currentPos->first && page.get().paragraphs.empty()) {
		StartImageAnimation();
	} else {
		writingState = WritingState::Paragraph;
		ease.reset();
	}
	headerAlpha = 0.0f;
}

void Renderer::FinishPage() {
	writingState = WritingState::Done;

	if (engine->GetMenu()->GetSetting("Autoplay").value)
		nextPageStart = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

const std::function<void(GLuint, float *, float *)> Renderer::GetCurlRenderCallback(float *textureBuffer) {
	return [&, textureBuffer](GLuint texture, float *vertBuffer, float *texBuffer) {
		// Draw framebuffer as texture                      
		glEnable(GL_TEXTURE_2D);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texture);

		glVertexPointer(2, GL_FLOAT, 0, vertBuffer);
		glTexCoordPointer(2, GL_FLOAT, 0, textureBuffer ? textureBuffer : texBuffer);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indexBuffer);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glDisable(GL_TEXTURE_2D);
		glLoadIdentity();
	};
}

bool Renderer::Render() {
	const auto &state = engine->GetState();

	if (state == Engine::State::Menu || state == Engine::State::Loading) {
		engine->GetMenu()->Render();

		if (state == Engine::State::Loading)
			loading.Draw(deltaTime, engine->GetMenu()->GetSelectedPage() == 0 ? background.scaledWidth / 4.0f : 0.0f, 0.0f);

		// Render FPS
		if (engine->GetMenu()->GetSetting("FPSCounter").value)
			DrawFPS();

		return false;
	}

	bool ret = false;

	if (bookUpdated) {
		UpdateBook();
		ret = true;
	}

	if (reset) {
		// Reset font scale
		if (headerFont)
			headerFont->SetScale(1.0f);
		for (auto &[path, font] : fonts)
			font->SetScale(1.0f);

		ret = true;
		reset = false;
	}

	if (audioFadeEase) {
		auto value = audioFadeEase->Advance(deltaTime);
		if (value >= 1.0f) {
			engine->GetAudio()->Reset();
			audioFadeEase.reset();
		} else {
			engine->GetAudio()->SetVolume(1.0f - value);
		}
	}

	if (book && state == Engine::State::Book) {
		if (writingState >= WritingState::Move) {
			glTranslatef(
				width / 2.0f - background.scaledWidth / 2.0f + (ease ? ease->Advance(engine->GetManager()->GetDeltaTime()) * book.get()->GetBack()->scaledWidth / 2.0f : book.get()->GetBack()->scaledWidth / 2.0f),
				0.0f,
				0.0f
			);
			RenderTexture(*book->GetBack());

			// Render FPS
			if (engine->GetMenu()->GetSetting("FPSCounter").value)
				DrawFPS();

			if (ease && ease->Value() >= 1.0f) {
				writingState = WritingState::Back;
				ease.reset();
			}

			return false;
		}

		// Render book texture
		glTranslatef(width / 2 - background.scaledWidth / 2, height / 2 - background.scaledHeight / 2, 0.0f);
		if (currentPage == 0 || (currentPage == 1 && curl.IsAnimating() && curlDir == Curl::CurlDir::Left)) {
			RenderTexture(forewardBackground);
		} else {
			if (backgroundEase) {
				RenderTexture(forewardBackground);
				glTranslatef(width / 2 - background.scaledWidth / 2, height / 2 - background.scaledHeight / 2, 0.0f);
				glColor4f(1.0f, 1.0f, 1.0f, backgroundEase->Advance(engine->GetManager()->GetDeltaTime()));
				if (backgroundEase->Value() >= 1.0f)
					backgroundEase.reset();
			}

			RenderTexture(
				background,
				writingState == WritingState::Close ? engine->GetMenu()->GetHalfVertexBuffer() : nullptr,
				writingState == WritingState::Close ? Menu::GetHalfTextureBufferReverse() : textureBuffer,
				true
			);
		}

		// Interate current position, if needed
		if (!currentPos && !pages.empty()) {
			currentPos = { 0, 0 };
			if (skipFirstPage) {
				++currentPos->first;
			}
			// If we have text on this page, start animating it
			if (auto &paragraphs = pages[currentPos->first].get().paragraphs; !paragraphs.empty())
				writer.SetText(paragraphs.at(currentPos->second), AdvanceAudio());
		}

		for(auto &[i, page] : Enumerate(pages)) {
			bool spicy = engine->GetMenu()->GetSetting("StreamingMode").value && page.get().spicy;

			glLoadIdentity();
			glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
			if (i == 0) {
#if defined(SNOBASTE_GL)
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffers[0]);
#elif defined(SNOBASTE_GLES)
				glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffers[0]);
#endif
				glClear(GL_COLOR_BUFFER_BIT);
				glTranslatef(
					(width / 2.0f - std::floor(background.scaledWidth / 2.0f)), // Avoid subpixel weirdness
					height / 2.0f - background.scaledHeight / 2.0f,
					0
				);
				RenderTexture(leftPageOccupied, nullptr, flipHorizontalTextureBuffer);
			}
			if (i == 1 || currentPage == 0) {
#if defined(SNOBASTE_GL)
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, framebuffers[1]);
#elif defined(SNOBASTE_GLES)
				glBindFramebufferOES(GL_FRAMEBUFFER_OES, framebuffers[1]);
#endif
				glClear(GL_COLOR_BUFFER_BIT);
				glTranslatef(0.0f, height / 2.0f - background.scaledHeight / 2.0f, 0);

				if (writingState == WritingState::Close) {
					RenderTexture(
						background,
						engine->GetMenu()->GetHalfVertexBuffer(),
						halfTextureBufferReverse
					);
				} else {
					RenderTexture(rightPage);
				}
			}

			// Render page number
			auto footerBounds = footerFont->GetBoundsForString(std::to_string(currentPage + i + 1));
			footerFont->Draw(
				std::to_string(currentPage + i + 1),
				(i == 0 && currentPage != 0 ? width / 2.0f - margin / 2.0f : background.scaledWidth / 2.0f - margin) - footerBounds.w,
				height - margin,
				(writingState == WritingState::Header && (currentPos->first == 0 || skipFirstPage)) ? headerAlpha : 1.0f,
				OpenGLFont::FontMargin::FONT_MARGIN_NONE,
				OpenGLFont::FontMargin::FONT_MARGIN_NONE
			);

			if (writingState == WritingState::Header) {
				if (i == 0 && (headerAlpha = ease->Advance(deltaTime)) >= 1.0f) {
					skipFirstPage = false;
					FinishHeaderAnimation(i, page);
				}
			}

			// Does this page start an entry?
			// If so, render a header
			if (!page.get().title.empty()) {
				std::stringstream header;
				if (page.get().entryNumber)
					header << "#" << *page.get().entryNumber << u8" \u2014 ";
				header << page.get().title;

				const auto size = header.str().size();
				headerFont->SetSpan({
					{ 0, { size - 2 - page.get().title.size(), OpenGLFont::Style::BoldItalic }},
					{ size - 2 - page.get().title.size(), { size - 3, page.get().titleStyle.empty() ? OpenGLFont::Style::BoldItalic : page.get().style}}
				});

				header << " (" << page.get().date << ")";
	
				bool widthValid = false;
				while (!widthValid) {
					headerBounds = headerFont->GetBoundsForString(header.str());

					if (headerBounds.w > background.scaledWidth / 2.0f - margin * 2) {
						headerFont->SetScale(headerFont->GetScale() - FontScaleDelta);
					} else {
						widthValid = true;
					}
				}

				if (i <= currentPos->first) {
					headerFont->Draw(
						header.str(),
						(i == 1 ? background.scaledWidth / 2.0f : width / 2) - (headerBounds.w + margin),
						height / 2 - background.scaledHeight / 2.0f + margin,
						(writingState == WritingState::Header && (i == currentPos->first || skipFirstPage)) ? headerAlpha : 1.0f,
						OpenGLFont::FontMargin::FONT_MARGIN_NONE,
						OpenGLFont::FontMargin::FONT_MARGIN_NONE
					);
				}
			}

			if (writingState == WritingState::Header && i == currentPos->first) {
				// Unbind framebuffer
				UnbindFramebuffer();
				continue;
			}

			float offset = 0.0f;
			if (auto font = fonts.find(page.get().font); font != fonts.end()) {
				// Load each paragraph's span
				std::map<std::size_t, OpenGLFont::Span> paragraphSpans;
				for (const auto &[i, paragraph] : Enumerate(page.get().paragraphs)) {
					if (auto iter = page.get().GetSpans().find(i); iter != page.get().GetSpans().end()) {
						OpenGLFont::Span span;

						for (const auto &s : iter->second) {
							span.emplace(
								std::make_pair(
									std::get<1>(s),
									std::make_pair(
										std::get<2>(s),
										spans.at(std::get<0>(s))
									)
								)
							);
						}

						paragraphSpans.emplace(
							std::make_pair(
								i,
								span
							)
						);
					}
				}

				// Measure combined paragraph size
				OpenGLFont::FontGlyph bounds;
				bool widthValid = false;
				while (!widthValid) {
					// Do we need to wrap?
					if (page.get().type == Book::Page::Type::Story ||
						page.get().type == Book::Page::Type::Foreward) {
						for (auto &paragraph : page.get().paragraphs) {
							auto copy = StringUtils::ReplaceAll(paragraph, "\n", " ");
							paragraph.clear();
							Rectangle<int> rect = {
								0,
								0,
								static_cast<int>(background.scaledWidth / 2.0f - margin * 2),
								static_cast<int>(background.scaledHeight - margin * 4 - headerBounds.h * 2)
							};

							font->second->Wrap(
								copy,
								rect,
								rect,
								[&](std::string_view line, int y, bool hyphenate) {
									auto string = std::string(line.data(), line.size());
									if (hyphenate) string.push_back('-');
									else string.push_back('\n');

									paragraph.append(string);
									return font->second->GetBounds(line).h;
								}
							);
						}
					}

					bounds = OpenGLFont::FontGlyph();
					for (const auto &[i, paragraph] : Enumerate(page.get().paragraphs)) {
						if (auto iter = paragraphSpans.find(i); iter != paragraphSpans.end()) {
							font->second->SetSpan(iter->second);
						}
						
						auto paragraphBounds = font->second->GetBoundsForString(paragraph);
						bounds.w = std::max(bounds.w, paragraphBounds.w);
						bounds.h += paragraphBounds.h;
						font->second->ClearSpan();
					}

					if ((page.get().type == Book::Page::Type::Poem && bounds.w > background.scaledWidth / 2.0f - margin * 2) ||
						bounds.h > background.scaledHeight - margin * 4 - headerBounds.h * 2) {
						font->second->SetScale(font->second->GetScale() - FontScaleDelta);
						ret = true;
					} else {
						widthValid = true;
					}
				}

				if (auto &image = page.get().image; image) {
					image->Scale(background.scaledWidth / 2.0f - margin * 1.5f, background.scaledHeight - margin * 1.5f - bounds.h);
					offset -= page.get().image->scaledHeight / 2.0f;
				}

				// Alternate justifications
				auto justification = page.get().type == Book::Page::Type::Foreward ? OpenGLFont::Justification::Center : OpenGLFont::Justification::Left;
				for (const auto &[p, paragraph] : Enumerate(page.get().paragraphs)) {
					if (i > currentPos->first || (i == currentPos->first && p > currentPos->second)) break;

					if (auto iter = paragraphSpans.find(p); iter != paragraphSpans.end()) {
						font->second->SetSpan(iter->second);
					}

					std::optional<std::pair<bool, std::string>> write = std::nullopt;
					if (i == currentPos->first && p == currentPos->second) {
						auto pos = writer.GetPos();
						write = writer.GetText(paused ? 0 : deltaTime);

						// Start playing audio on our first
						// rendered character
						if (pos == 0 && !write->second.empty() && !spicy) {
							engine->GetAudio()->Play();
						}
					}

					auto paragraphBounds = font->second->GetBoundsForString(write && justification == OpenGLFont::Justification::Left ? write->second : paragraph);

					float xOrigin = ( i == 0 && currentPage != 0 ? (
							justification == OpenGLFont::Justification::Left ?
							width / 2.0f - background.scaledWidth / 2.0f + margin :
							width / 2.0f - margin / 2.0f - paragraphBounds.w
						) : (
							justification == OpenGLFont::Justification::Left ?
							margin / 2.0f :
							background.scaledWidth / 2.0f - margin - paragraphBounds.w
						)
					);

					// To properly center forewards
					if (currentPage == 0)
						xOrigin -= margin * 0.5f;

					font->second->Draw(
						write ? write->second : paragraph,
						xOrigin,
						height / 2 - bounds.h / 2 + offset,
						skipFirstPage ? headerAlpha : 1.0f,
						OpenGLFont::FontMargin::FONT_MARGIN_NONE,
						OpenGLFont::FontMargin::FONT_MARGIN_NONE,
						justification,
						!spicy,
						justification == OpenGLFont::Justification::Left ? std::optional<std::string_view>(std::nullopt) : paragraph
					);

					font->second->ClearSpan();

					if (write && write->first) {
						AdvanceParagraph();
					}

					offset += font->second->GetBoundsForString(paragraph).h;

					if (page.get().type == Book::Page::Type::Poem)
						justification = (justification == OpenGLFont::Justification::Left ? OpenGLFont::Justification::Right : OpenGLFont::Justification::Left);
				}
			}

			// Does this page have an image?
			// If so, render it after the paragraphs
			auto &image = page.get().image;
			if (image && (static_cast<int>(writingState) > static_cast<int>(WritingState::Paragraph) || ((i == 0 && writingState > WritingState::Header) || (currentPos->first == 1 && writingState == WritingState::Header)))) {
				if (page.get().paragraphs.empty())
					image->Scale(background.scaledWidth / 2.0f - margin * 1.5f, background.scaledHeight - margin * 1.5f);

				auto center = ((background.scaledWidth / 2.0f - margin * 1.5f) - image->scaledWidth) / 2.0f;
				glTranslatef(
					(
						i == 0 ?
							margin * 0.25f + width / 2.0f - background.scaledWidth / 4.0f - image->scaledWidth / 2.0f :
							margin * 0.25f + ((background.scaledWidth / 2.0f - margin * 1.5f) - image->scaledWidth) / 2.0f
					),
					(page.get().paragraphs.empty() ? height / 2 - image->scaledHeight / 2 : image->scaledHeight / 2 + margin) + offset,
					0
				);

				// Are we animating the image?
				if (writingState == WritingState::Image) {
					glTranslatef(image->scaledWidth / 2.0f, image->scaledHeight / 2.0f, 0.0f);

					auto value = paused ? ease->Value() : ease->Advance(deltaTime);
					if (value >= 1.0f) {
						ease.reset();

						if (i != 0) {
							FinishPage();
						} else {
							AdvanceParagraph();
						}
					}

					float degInRad;
					for (int i = 0; i < 360; i++) {
						degInRad = i * DEG2RAD;

						circleTexCoordBuffer[i * 2] = ((std::sin(degInRad) * value)) * (static_cast<float>(image->scaledHeight) / image->scaledWidth) + 0.5;
						circleTexCoordBuffer[i * 2 + 1] = (std::cos(degInRad) * value) + 0.5;
					}

					if (spicy)
						glColor4f(0.0f, 0.0f, 0.0f, 1.0f);
					else
						glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

					glScalef(image->scaledHeight * value, image->scaledHeight * value, 1.0f);
					glEnable(GL_TEXTURE_2D);
					glBindTexture(GL_TEXTURE_2D, images[image->relativePath]);
					glVertexPointer(2, GL_FLOAT, 0, circleVertexBuffer.data());
					glTexCoordPointer(2, GL_FLOAT, 0, circleTexCoordBuffer.data());
					glEnableClientState(GL_VERTEX_ARRAY);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);
					glDrawElements(GL_TRIANGLE_FAN, circleIndexBuffer.size(), GL_UNSIGNED_SHORT, circleIndexBuffer.data());
					glDisableClientState(GL_TEXTURE_COORD_ARRAY);
					glDisableClientState(GL_VERTEX_ARRAY);
					glLoadIdentity();
					glDisable(GL_TEXTURE_2D);
				} else {
					if (spicy)
						glColor4f(0.0f, 0.0f, 0.0f, 1.0f);

					RenderTexture(*image, nullptr, textureBuffer, spicy);
				}
			}

			// Unbind framebuffer
			UnbindFramebuffer();

			/*
			// Get texture from framebuffer
			glEnable(GL_TEXTURE_2D);
			glBindTexture(GL_TEXTURE_2D, textures[0]);
			glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
			int target;
			for (int p = 0; p < width / 2.0f * height; ++p) {
				target = rand() % (width / 2 * height);
				std::swap(pixels[p * 4], pixels[target * 4]);
				std::swap(pixels[p * 4 + 1], pixels[target * 4 + 1]);
				std::swap(pixels[p * 4 + 2], pixels[target * 4 + 2]);
			}
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width / 2.0f, height, 0, GL_RGBA,
				GL_UNSIGNED_BYTE, pixels);
			glDisable(GL_TEXTURE_2D);
			*/
		}
		
		if (curl.IsAnimating()) {
			if (curlDir == Curl::CurlDir::Right) {
				if (currentPage != 0)
					GetCurlRenderCallback()(textures[0], framebufferVertexBuffer, framebufferTextureBuffer);

				curl.Render(
					textures[1],
					width / 2.0f, 0.0f,
					framebufferVertexBuffer,
					GetCurlRenderCallback(),
					deltaTime,
					std::nullopt,
					false,
					false,
					writingState == WritingState::Close ? book->GetBack() : static_cast<std::optional<std::reference_wrapper<std::unique_ptr<Book::Page::Image>>>>(std::nullopt)
				);
			} else {
				glTranslatef(width / 2.0f, 0.0f, 0);
				GetCurlRenderCallback()(textures[1], framebufferVertexBuffer, framebufferTextureBuffer);
				curl.Render(textures[0], -width / 2.0f, 0.0f, framebufferVertexBuffer, GetCurlRenderCallback(), deltaTime, width / 2.0f);
			}
		} else {
			glTranslatef(width / 2.0f, 0.0f, 0);
			GetCurlRenderCallback()(textures[1], framebufferVertexBuffer, framebufferTextureBuffer);
			if (currentPage != 0)
				GetCurlRenderCallback()(textures[0], framebufferVertexBuffer, framebufferTextureBuffer);
		}
	}

	// Render FPS
	if (engine->GetMenu()->GetSetting("FPSCounter").value)
		DrawFPS();

	// Has enough time passed in autoplay mode that we
	// should turn the page?
	if (nextPageStart && std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()) - *nextPageStart > 2s)
		AdvancePage();

	return ret;
}

void Renderer::Back() {
	engine->GetAudio()->Reset();
	engine->GetMenu()->Show();
	engine->GetMenu()->Resize();
	engine->SetState(Engine::State::Menu);
}

void Renderer::DrawFPS() {
	if (deltaTime > 0) {
		totalFrametime += deltaTime;
		++frametimes;

		if (totalFrametime >= 0.50f) {
			std::stringstream fpsStream;
			fpsStream << static_cast<int>(1.0f / (totalFrametime / frametimes)) << " FPS";
			fps = fpsStream.str();

			totalFrametime = 0.0;
			frametimes = 0;
		}
	}

	if (debugFont.IsLoaded()) {
		debugFont.Draw(
			fps,
			0,
			0,
			OpenGLFont::FONT_MARGIN_FULL_CHAR,
			OpenGLFont::FONT_MARGIN_FULL_CHAR
		);
	}
}

inline void Renderer::UnbindFramebuffer() const {
#if defined(SNOBASTE_GL)
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
#elif defined(SNOBASTE_GLES)
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, 0);
#endif
}

void Renderer::Cleanup() {
#if defined(SNOBASTE_GL)
	glDeleteFramebuffersEXT(2, framebuffers);
#elif defined(SNOBASTE_GLES)
	glDeleteFramebuffersOES(2, framebuffers);
#endif
	glDeleteTextures(2, textures);

	for (auto &[path, image] : images)
		glDeleteTextures(1, &image);

	for (auto &[path, font] : fonts)
		font->KillFont();

	if (headerFont)
		headerFont->KillFont();

	if (footerFont)
		footerFont->KillFont();

	debugFont.KillFont();

	engine->GetMenu()->Cleanup();
}
#include "Menu.hpp"

#include <thread>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Filesystem/FileRepository.hpp"

#include "Defines.hpp"
#include "Engine.hpp"

Menu::AnimationState &operator++(Menu::AnimationState &c) {
	using IntType = typename std::underlying_type<Menu::AnimationState>::type;
	c = static_cast<Menu::AnimationState>(static_cast<IntType>(c) + 1);
	return c;
}

float Menu::halfTextureBuffer[8] = {
	0.5f,
	0.0f,
	0.5f,
	1.0f,
	1.0f,
	1.0f,
	1.0f,
	0.0f
};

float Menu::halfTextureBufferReverse[8] = {
	0.0f,
	0.0f,
	0.0f,
	1.0f,
	0.5f,
	1.0f,
	0.5f,
	0.0f
};

Menu::Menu(Engine *engine) :
	engine(engine),
	back("Back"),
	curl(engine->GetRenderer().get()) {
	headerFont = std::make_unique<OpenGLFont>(FileRepository::registry->GetResourceDirectory() / "Fonts" / "ka1");
	menuFont = std::make_unique<OpenGLFont>(
		FileRepository::registry->GetResourceDirectory() / "Fonts" / "Roboto",
		std::vector<OpenGLFont::SpanItem>{
			OpenGLFont::Style::Regular,
			{ OpenGLFont::Style::Bold, 1.0f },
			{ OpenGLFont::Style::Bold, 1.5f }
		}
	);

	mainMenuItems = std::vector<MenuItem>{
		{ "Books", [&](MenuItem &item, bool init) {
				SetCurrentMenuItems(GetBookMenuItems());
			} 
		},
		{ "Settings", [&](MenuItem &item, bool init) {
				SetCurrentMenuItems(&settingsMenuItems);
			}
		},
		{ "Quit", [this](MenuItem &item, bool init) {
				if (auto window = this->engine->GetWindow()) {
					glfwSetWindowShouldClose(window, true);
				}
			} 
		}
	};

	settingsMenuItems = std::vector<MenuItem>{
		{ "Resolution", "Sets the default resolution", "Resolution", -1, [&](MenuItem &item, bool init) {
				if (!init && ++item.value >= item.settingValues.size())
					item.value = 0;

				auto resolution = StringUtils::Split(item.settingValues[item.value], "x");
				glfwSetWindowSize(this->engine->GetWindow(), std::stoi(resolution[0]), std::stoi(resolution[1]));
				FileRepository::registry->SetSetting(item.settingKey, item.value);
			}
		},
		{ "FPS Counter", "Toggles an FPS counter in the upper left corner", "FPSCounter", 0, [&](MenuItem &item, bool init) {
				item.value = !item.value;
				FileRepository::registry->SetSetting(item.settingKey, item.value);
			}
		},
		{ "Audio", "Toggles voiceover readings of poems and stories", "Audio", 1, [&](MenuItem &item, bool init) {
				item.value = !item.value;
				this->engine->GetAudio()->Reset();
				FileRepository::registry->SetSetting(item.settingKey, item.value);
			}
		},
		{ "Streaming Mode", "Hides spicier stories and images", "StreamingMode", 1, [&](MenuItem &item, bool init) {
				item.value = !item.value;
				FileRepository::registry->SetSetting(item.settingKey, item.value);
			}
		},
		{ "Autoplay", "Turns the pages of the books for you", "Autoplay", 0, [&](MenuItem &item, bool init) {
				item.value = !item.value;
				FileRepository::registry->SetSetting(item.settingKey, item.value);
			}
		},
		{ back, [&](MenuItem &item, bool init) {
				SetCurrentMenuItems(&mainMenuItems);
			}
		}
	};

	// Initialize settings
	for (auto &setting : settingsMenuItems.items) {
		auto value = FileRepository::registry->GetSettingInteger(setting.settingKey);
		if (!value) {
			setting.value = setting.defaultValue;
			FileRepository::registry->SetSetting(setting.settingKey, setting.defaultValue);
		} else {
			setting.value = *value;
		}
	}

	checkbox.SetColor(Color::ChipTan);
	checkbox.SetObjectScale(1.75f);

	currentMenuItems = &mainMenuItems;
}

Menu::~Menu() {
	SetCurrentMenuItems(nullptr);
}

Menu::MenuItems *Menu::GetBookMenuItems() {
	class BookMenuItems : public MenuItems {
	friend class Menu;
	public:
		BookMenuItems(Menu *menu) :
			menu(menu) {

		}
		virtual ~BookMenuItems() {
			menu->scrollbarXPos = 0.0f;
			menu->hoveredOverScrollbar = false;
			menu->draggingScrollbar = false;
			menu->scrollBarActive = false;
		}

	private:
		Menu *menu;
	};

	// Assemble menu items for books
	BookMenuItems *bookMenuItems = new BookMenuItems(this);

	for (const auto &book : books) {
		bookMenuItems->items.emplace_back(
			MenuItem(
				book.GetTitle(),
				book.GetFront(),
				[&](MenuItem &item, bool init) {
					selectedBook = book;
					SetCurrentMenuItems(GetMenuItemsForBook(book));
				}
			)
		);
	}

	bookMenuItems->items.emplace_back(
		MenuItem(
			back,
			[&](MenuItem &item, bool init) {
				SetCurrentMenuItems(&mainMenuItems);
			}
		)
	);

	// Reset scrollbar
	scrollbarXPos = 0.0f;
	hoveredOverScrollbar = false;
	draggingScrollbar = false;
	scrollBarActive = true;

	return bookMenuItems;
}

Menu::MenuItems *Menu::GetMenuItemsForBook(const Book &book) {
	MenuItems *menuItems = new MenuItems();

	for (const auto &page : book.GetPages()) {
		auto path = book.GetPath();

		const auto animate = [&](MenuItem &item, bool init) {
			selectedPage = page.first;
			animationState = AnimationState::Fade;
			ease = std::make_unique<Ease<float>>(0.0f, 1.0f, 0.5f);

			if (auto currentBook = engine->GetBook(); !currentBook || currentBook->GetTitle() != selectedBook->get().GetTitle()) {
				loaded = false;

				loadingMutex.lock();

				auto path = selectedBook->get().GetPath();
				std::thread([&, path] {
					auto book = std::make_shared<Book>(path);
					loaded = true;
					std::unique_lock<std::mutex> lock(loadingMutex);

					engine->SetBook(book);
					engine->GetRenderer()->SetPage(selectedPage, true);
					engine->SetState(Engine::State::Book);

					ease.reset();
				}).detach();
			}
		};

		if (!page.second.title.empty()) {
			std::stringstream stream;
			if (page.second.entryNumber)
				stream << "#" << *page.second.entryNumber << u8" \u2014 ";
			stream << page.second.title;
			
			menuItems->items.emplace_back(
				MenuItem(
					stream.str(),
					animate
				)
			);
		} else if (page.second.type == Book::Page::Type::Foreward) {
			menuItems->items.emplace_back(
				MenuItem(
					"Foreward",
					animate
				)
			);
		}
	}

	menuItems->items.emplace_back(
		MenuItem(
			back,
			[&](MenuItem &item, bool init) {
				SetCurrentMenuItems(GetBookMenuItems());
			}
		)
	);

	return menuItems;
}

void Menu::SetCurrentMenuItems(MenuItems *menuItems) {
	// Prevent us from deleting the current
	// menu items if we set twice
	if (menuItems != currentMenuItems) {

		if (currentMenuItems && currentMenuItems != &mainMenuItems && currentMenuItems != &settingsMenuItems) {
			delete currentMenuItems;
			currentMenuItems = nullptr;
		}

		currentMenuItems = menuItems;
	}

	// Scale menu items if needed
	ScaleMenuItems();
}

void Menu::ScaleMenuItems() {
	menuFont->SetScale(1.0f);
	checkbox.SetSize(baseCheckBoxSize);

	bool widthValid = false;
	while (!widthValid) {
		widthValid = true;
		DoWithMenuItems([&](const OpenGLFont::FontGlyph &bounds, float accum, float xOffset, std::size_t i, const MenuItem &item) {
			if (headerBounds.advance * 2 + bounds.advance * 4 + xOffset + bounds.w + (item.settingKey.empty() ? 0 : checkbox.GetSize() * 2) > engine->GetRenderer()->GetWidth()) {
				menuFont->SetScale(menuFont->GetScale() - FontScaleDelta);
				widthValid = false;
				return false;
			}

			return true;
		});
	}
}

void Menu::Init() {
	headerFont->InitFont();
	headerFont->SetScale(2.5f);
	headerFont->SetColor(Color::ChipTan);

	headerBounds = headerFont->GetBoundsForString("CHAnniversary");

	menuFont->InitFont();
	menuFont->SetScale(1.0f);
	menuFont->SetColor(Color::ChipTan);

	// Load background Chip
	backgroundChip.relativePath = "Images/menuchip.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / backgroundChip.relativePath).string().c_str(),
		backgroundChip.data,
		backgroundChip.width,
		backgroundChip.height,
		backgroundChip.channels,
		4
	);
	engine->GetRenderer()->LoadTexture(backgroundChip);

	// Find all books
	auto bookPaths = FileRepository::fileRepository->GetFilesWithExtension(
		FileRepository::registry->GetResourceDirectory() / "Books",
		".json"
	);

	// Try to soft load the books
	for (const auto &path : bookPaths) {
		auto book = Book(path, Book::LoadMode::Soft);

		if (book.IsValid()
#ifndef DEBUG
			&& !book.IsTest()
#endif
			) {
			books.emplace_back(
				std::move(book)
			);
		}
	}

	// Preload book covers
	for (const auto &book : books) {
		if (const auto &front = book.GetFront(); !front.empty()) {
			Book::Page::Image image;

			image.relativePath = front;
			fpng::fpng_decode_file(
				std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / image.relativePath).string().c_str(),
				image.data,
				image.width,
				image.height,
				image.channels,
				4
			);
			engine->GetRenderer()->LoadTexture(image);

			covers.emplace(
				std::make_pair(
					front,
					std::move(image)
				)
			);
		}
	}

	curl.Init();

	// Load resolutions into resolution setting
	auto monitor = glfwGetPrimaryMonitor();
	int count;
	auto modes = glfwGetVideoModes(monitor, &count);

	auto &resolutionSetting = settingsMenuItems.items[settingsMenuItems.keyedItems.at("Resolution")];

	int indexFor1080p = -1;
	int indexFor720p = -1;
	std::set<std::string> uniqueResolutions;
	for (auto i = 0; i < count; ++i) {
		if (modes[i].width < 1280 || modes[i].height < 720) continue;

		auto resolution = std::to_string(modes[i].width) + "x" + std::to_string(modes[i].height);

		if (uniqueResolutions.find(resolution) != uniqueResolutions.end()) continue;

		if (modes[i].width == 1920 && modes[i].height == 1080)
			indexFor1080p = resolutionSetting.settingValues.size();
		else if (modes[i].width == 1280 && modes[i].height == 720)
			indexFor720p = resolutionSetting.settingValues.size();

		uniqueResolutions.emplace(resolution);
		resolutionSetting.settingValues.emplace_back(
			std::move(resolution)
		);
	}

	if (resolutionSetting.value < 0 || resolutionSetting.value >= count) {
		if (indexFor1080p != -1)
			resolutionSetting.value = indexFor1080p;
		else if (indexFor720p != -1)
			resolutionSetting.value = indexFor720p;
		else
			resolutionSetting.value = 0;

	}

	resolutionSetting.onClicked(resolutionSetting, true);

	return;
}

void Menu::Resize() {
	SetCurrentMenuItems(currentMenuItems);

	// Update aspect ratios for covers
	for (auto &cover : covers) {
		cover.second.UpdateRatio();
		cover.second.scaledHeight = engine->GetRenderer()->GetHeight() - headerBounds.h * 5.0f;
		cover.second.scaledWidth = cover.second.scaledHeight * cover.second.ratio;
	}

	curl.Resize(
		engine->GetRenderer()->GetBackground().scaledWidth / 2.0f,
		engine->GetRenderer()->GetBackground().scaledHeight
	);

	halfVertexBuffer[3] = engine->GetRenderer()->GetForewardBackground().scaledHeight;
	halfVertexBuffer[4] = engine->GetRenderer()->GetForewardBackground().scaledWidth / 2.0f;
	halfVertexBuffer[5] = engine->GetRenderer()->GetForewardBackground().scaledHeight;
	halfVertexBuffer[6] = engine->GetRenderer()->GetForewardBackground().scaledWidth / 2.0f;

	backgroundRectVertexBuffer[3] = engine->GetRenderer()->GetHeight();
	backgroundRectVertexBuffer[4] = engine->GetRenderer()->GetWidth();
	backgroundRectVertexBuffer[5] = engine->GetRenderer()->GetHeight();
	backgroundRectVertexBuffer[6] = engine->GetRenderer()->GetWidth();

	backgroundChip.Scale(engine->GetRenderer()->GetWidth() / 2.0f, engine->GetRenderer()->GetHeight() - headerBounds.h);
}

void Menu::DoWithMenuItems(std::function<bool(const OpenGLFont::FontGlyph &, float, float, std::size_t, const MenuItem &)> f) {
	float accum = headerBounds.h * (currentMenuItems == &mainMenuItems ? 3.0f : 2.5f);
	float xOffset = 0.0f;
	int maxWidth = std::numeric_limits<int>::min();

	if (currentMenuItems) {
		for (const auto &[i, item] : Enumerate(currentMenuItems->items)) {
			if (item.image.empty()) {
				auto bounds = menuFont->GetBoundsForString(item.label);
				auto hintBounds = 
					item.hint.empty() ? std::optional<OpenGLFont::FontGlyph>(std::nullopt) : bounds = menuFont->GetBoundsForString(item.hint);

				maxWidth = std::max(bounds.w, maxWidth);

				if (!f(hintBounds && hintBounds->w > bounds.w ? *hintBounds : bounds, accum, xOffset, i, item))
					break;

				if (!item.hint.empty()) {
					menuFont->Draw(
						item.hint,
						headerBounds.advance + hintBounds->advance * 2.0f + xOffset,
						accum + bounds.h,
						OpenGLFont::FontMargin::FONT_MARGIN_FULL_CHAR,
						OpenGLFont::FontMargin::FONT_MARGIN_NONE
					);
					maxWidth = std::max(bounds.w + bounds.advance, maxWidth);
				}

				accum += bounds.h * 2.5f;

				if (currentMenuItems != &settingsMenuItems && accum >= engine->GetRenderer()->GetHeight() - headerBounds.h * 2.5f) {
					accum = headerBounds.h * 2.5f;
					xOffset += maxWidth + bounds.advance * 2.0f;
					maxWidth = std::numeric_limits<int>::min();
				}
			} else {
				const auto &cover = covers[item.image];
				const auto viewport = (scrollbarXPos / engine->GetRenderer()->GetWidth() * totalWidth);

				glTranslatef(
					headerBounds.advance + xOffset - viewport,
					accum,
					0.0f
				);

				// Is our mouse hovered over this item?
				if (auto pos = engine->GetManager()->GetMousePos();
					pos.first >= headerBounds.advance + xOffset - viewport &&
					pos.first <= headerBounds.advance + xOffset - viewport + cover.scaledWidth &&
					pos.second >= accum &&
					pos.second <= accum + cover.scaledHeight &&
					!draggingScrollbar) {
					glColor4f(Color::ChipPink.r, Color::ChipPink.g, Color::ChipPink.b, 1.0f);
					hoveredIndex = i;
				} else {
					glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
				}

				engine->GetRenderer()->RenderTexture(cover, nullptr, Renderer::GetTextureBuffer(), true);

				xOffset += cover.scaledWidth * 1.25f;
				if (i == currentMenuItems->items.size() - 2) {
					// Generate the scrollbar vertex buffer
					totalWidth = xOffset;

					// Hide scrollbar if it's not needed
					if (totalWidth <= engine->GetRenderer()->GetWidth())
						scrollBarActive = false;

					scrollBarVertexBuffer[4] = scrollBarVertexBuffer[6] = (engine->GetRenderer()->GetWidth() / totalWidth) * engine->GetRenderer()->GetWidth();

					xOffset = 0.0f;
					accum += cover.scaledHeight + menuFont->GetBoundsForString(back).h;
					scrollBarYPos = accum;
					accum += 30.0f;
				}
			}
		}

		// Draw checkboxes if we're in settings
		if (currentMenuItems == &settingsMenuItems) {
			accum = headerBounds.h * 2.5f;
			for (const auto &[i, item] : Enumerate(currentMenuItems->items)) {
				if (!item.settingKey.empty()) {
					auto bounds = menuFont->GetBoundsForString(item.label);

					if (item.settingValues.empty()) {
						checkbox.Draw(
							maxWidth + headerBounds.advance + bounds.advance * 2.0f + xOffset + checkbox.GetSize(),
							accum + bounds.h / 2.0f, item.value > 0
						);
					} else {
						menuFont->SetColor(Color::ChipTan);
						menuFont->Draw(
							item.settingValues[item.value],
							maxWidth + headerBounds.advance + bounds.advance * 2.0f + xOffset + checkbox.GetSize(),
							accum + bounds.h / 2.0f,
							OpenGLFont::FontMargin::FONT_MARGIN_NONE,
							OpenGLFont::FontMargin::FONT_MARGIN_NONE
						);
					}

					accum += bounds.h * 2.5f;

					if (accum >= engine->GetRenderer()->GetHeight() - headerBounds.h * 2.5f) {
						accum = headerBounds.h * 2.5f;
						xOffset += maxWidth + bounds.advance * 2.0f;
						maxWidth = std::numeric_limits<int>::min();
					}
				}
			}
		}
	}
}

void Menu::Render() {
	auto fontAlpha = (
		animationState == AnimationState::None ?
			1.0f :
			(
				animationState == AnimationState::Fade ?
				1.0f - ease->Advance(engine->GetManager()->GetDeltaTime()) :
				0.0f
			)
	);

	// Render background
	if (animationState < AnimationState::In) {
		glColor4f(Color::ChipTan.r, Color::ChipTan.g, Color::ChipTan.b, fontAlpha);
		glVertexPointer(2, GL_FLOAT, 0, backgroundRectVertexBuffer);
		glEnableClientState(GL_VERTEX_ARRAY);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, engine->GetRenderer()->GetIndexBuffer());
		glColor4f(Color::ChipRed.r, Color::ChipRed.g, Color::ChipRed.b, fontAlpha);
		float scaleX = 1.0f - (static_cast<float>(headerBounds.advance) / engine->GetRenderer()->GetWidth());
		float scaleY = 1.0f - (static_cast<float>(headerBounds.h) / engine->GetRenderer()->GetHeight());
		glTranslatef(headerBounds.advance / 2.0f, headerBounds.h / 2.0f, 0.0f);
		glScalef(scaleX, scaleY, 1.0f);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, engine->GetRenderer()->GetIndexBuffer());
		glDisableClientState(GL_VERTEX_ARRAY);
		glLoadIdentity();

		// Fill bottom right corner of screen with background Chip
		glTranslatef(
			engine->GetRenderer()->GetWidth() - backgroundChip.scaledWidth - headerBounds.advance,
			engine->GetRenderer()->GetHeight() - backgroundChip.scaledHeight - headerBounds.h / 2.0f,
			0.0f
		);
		glColor4f(1.0f, 1.0f, 1.0f, fontAlpha);
		engine->GetRenderer()->RenderTexture(backgroundChip, nullptr, engine->GetRenderer()->GetTextureBuffer(), true);

		// If we're not in the main menu, dim background
		if (currentMenuItems != &mainMenuItems) {
			glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
			glVertexPointer(2, GL_FLOAT, 0, backgroundRectVertexBuffer);
			glEnableClientState(GL_VERTEX_ARRAY);
			glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, engine->GetRenderer()->GetIndexBuffer());
		}

		headerFont->Draw(
			"CHAnniversary",
			0,
			0,
			fontAlpha,
			OpenGLFont::FontMargin::FONT_MARGIN_FULL_CHAR,
			OpenGLFont::FontMargin::FONT_MARGIN_FULL_CHAR
		);
	}

	auto pos = engine->GetManager()->GetMousePos();

	hoveredIndex = std::nullopt;
	DoWithMenuItems([&] (const OpenGLFont::FontGlyph &bounds, float accum, float xOffset, std::size_t i, const MenuItem &item) {
		// Is our mouse hovered over this item?
		float yOffset = 0.0f;
		OpenGLFont::FontGlyph newBounds;
		if (pos.first >= headerBounds.advance + bounds.advance * 2.0f + xOffset &&
			pos.first <= headerBounds.advance + bounds.advance * 2.0f + xOffset + bounds.w &&
			pos.second >= accum - (currentMenuItems == &mainMenuItems ? bounds.h * 0.25f : 0) &&
			pos.second <= accum + bounds.h * (item.hint.empty() ? 1.0f : 2.5f) + (currentMenuItems == &mainMenuItems ? bounds.h * 0.25f : 0) &&
			animationState == AnimationState::None) {
			if (currentMenuItems != &mainMenuItems) {
				menuFont->SetColor(Color::ChipRed);
				menuFont->SetStyle({
					OpenGLFont::SpanItem{ OpenGLFont::Style::Bold, 1.0f }
				});
			} else {
				menuFont->SetStyle({
					OpenGLFont::SpanItem{ OpenGLFont::Style::Bold, 1.5f }
				});
			}
			newBounds = menuFont->GetBoundsForString(item.label);
			yOffset = (bounds.h - newBounds.h) / 2.0f;
			hoveredIndex = i;
		} else {
			menuFont->SetColor(Color::ChipTan);
		}

		menuFont->Draw(
			item.label,
			headerBounds.advance + bounds.advance * 2 + xOffset,
			accum + yOffset - (currentMenuItems == &mainMenuItems ? newBounds.h / 4.0f : 0),
			fontAlpha,
			OpenGLFont::FontMargin::FONT_MARGIN_NONE,
			OpenGLFont::FontMargin::FONT_MARGIN_NONE
		);

		menuFont->ClearSpan();

		return true;
	});

	if (animationState >= AnimationState::In && animationState <= AnimationState::Move && selectedBook && !selectedBook->get().GetFront().empty()) {
		auto &cover = covers[selectedBook->get().GetFront()];

		cover.Scale(
			engine->GetRenderer()->GetWidth(),
			engine->GetRenderer()->GetHeight()
		);

		if (animationState == AnimationState::In) {
			cover.scaledWidth *= ease->Advance(engine->GetManager()->GetDeltaTime());
			cover.scaledHeight *= ease->Value();
		}
		
		glTranslatef(
			engine->GetRenderer()->GetWidth() / 2.0f - cover.scaledWidth / 2.0f + (
				animationState == AnimationState::Move ?
				ease->Advance(engine->GetManager()->GetDeltaTime()) * cover.scaledWidth / 2.0f :
				(
					animationState > AnimationState::Move ?
					cover.scaledWidth / 2.0f :
					0.0f
				)
			),
			engine->GetRenderer()->GetHeight() / 2.0f - cover.scaledHeight / 2.0f,
			0.0f
		);

		glColor4f(1.0f, 1.0f, 1.0f, animationState == AnimationState::In ? ease->Value() : 1.0f);
		engine->GetRenderer()->RenderTexture(cover, nullptr, Renderer::GetTextureBuffer(), true);
	}

	if (animationState == AnimationState::Open) {
		// Render the right half of the first page's texture under the curl
		glTranslatef(engine->GetRenderer()->GetWidth() / 2.0f, 0.0f, 0.0f);
		engine->GetRenderer()->RenderTexture(
			engine->GetRenderer()->GetForewardBackground(),
			halfVertexBuffer,
			halfTextureBuffer
		);

		if (curl.IsAnimating()) {
			curl.Render(
				engine->GetRenderer()->GetImage(selectedBook->get().GetFront()),
				engine->GetRenderer()->GetWidth() / 2.0f,
				0.0f,
				halfVertexBuffer,
				engine->GetRenderer()->GetCurlRenderCallback(engine->GetRenderer()->GetTextureBuffer()),
				engine->GetManager()->GetDeltaTime(),
				std::nullopt,
				true,
				selectedPage == 0
			);
		} else {
			glTranslatef(engine->GetRenderer()->GetWidth() / 2.0f - engine->GetRenderer()->GetForewardBackground().scaledWidth / 2.0f, 0.0f, 0.0f);
			engine->GetRenderer()->RenderTexture(
				selectedPage == 0 ? engine->GetRenderer()->GetForewardBackground() : engine->GetRenderer()->GetBackground(),
				halfVertexBuffer,
				halfTextureBufferReverse
			);
		}
	}

	if (animationState != AnimationState::None && ease && ease->Value() >= ease->End()) {
		++animationState;

		ease = std::make_unique<Ease<float>>(0.0f, 1.0f, 1.0f);
		if (animationState == AnimationState::Open) {
			curl.Start(Curl::CurlDir::Right, [&] {
				// TODO: Load book in separate thread while animation plays
				if (auto currentBook = engine->GetBook(); currentBook && currentBook->GetTitle() == selectedBook->get().GetTitle()) {
					ease.reset();
					animationState = AnimationState::None;

					// Resize to reset the scale of the book covers
					Resize();

					engine->GetRenderer()->SetPage(selectedPage);
					engine->SetState(Engine::State::Book);
				} else {
					if (!loaded)
						engine->SetState(Engine::State::Loading);

					loadingMutex.unlock();
				}
			});
		}
	}

	// Render scrollbar

	if (!scrollBarActive) return;

	if (draggingScrollbar || 
		(pos.first >= scrollbarXPos &&
		pos.first <= scrollbarXPos + scrollBarVertexBuffer[6] &&
		pos.second >= scrollBarYPos &&
		pos.second <= scrollBarYPos + scrollBarVertexBuffer[3])) {
		hoveredOverScrollbar = true;
		glColor4f(Color::ChipPink.r, Color::ChipPink.g, Color::ChipPink.b, Color::ChipPink.a);
		
		if (draggingScrollbar) {
			scrollbarXPos = std::clamp(
				scrollbarXPos + static_cast<float>(engine->GetManager()->GetMouseDelta().first),
				0.0f,
				engine->GetRenderer()->GetWidth() - scrollBarVertexBuffer[6]
			);
		}
	} else {
		hoveredOverScrollbar = false;
		glColor4f(Color::ChipRed.r, Color::ChipRed.g, Color::ChipRed.b, Color::ChipRed.a);
	}

	glTranslatef(scrollbarXPos, scrollBarYPos, 0.0f);
	glVertexPointer(2, GL_FLOAT, 0, scrollBarVertexBuffer);
	glEnableClientState(GL_VERTEX_ARRAY);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, engine->GetRenderer()->GetIndexBuffer());
	glDisableClientState(GL_VERTEX_ARRAY);

	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
}

void Menu::OnClick(int action) {
	if (animationState != AnimationState::None) return;

	if (action == GLFW_PRESS) {
		if (currentMenuItems && hoveredIndex) {
			auto &item = currentMenuItems->items.at(*hoveredIndex);
			item.onClicked(item, false);
		}

		if (hoveredOverScrollbar)
			draggingScrollbar = true;
	} else {
		draggingScrollbar = false;
	}
}

void Menu::Back() {
	if (animationState != AnimationState::None) return;

	if (currentMenuItems == &mainMenuItems) {
		if (auto window = this->engine->GetWindow()) {
			glfwSetWindowShouldClose(window, true);
		}
	} else if (currentMenuItems) {
		// Find "Back" menu item
		for (auto &item : currentMenuItems->items) {
			if (item.label == back)
				item.onClicked(item, false);
		}
	}
}

void Menu::Show() {
	animationState = AnimationState::None;
}

void Menu::Cleanup() {
	headerFont->KillFont();
	menuFont->KillFont();
}
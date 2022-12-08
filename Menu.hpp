#pragma once

#include <mutex>

#include "Rendering/Checkbox.hpp"
#include "Rendering/OpenGLFont.hpp"

#include "Book.hpp"
#include "Curl.hpp"
#include "Ease.hpp"

using namespace SnobasteCPP;

class Engine;
class Menu : public LoggableClass {
public:
	class MenuItem {
	public:
		MenuItem(const std::string &label, std::function<void(MenuItem &, bool)> onClicked) :
			label(label),
			onClicked(onClicked) {

		}

		MenuItem(const std::string &label, const std::string &front, std::function<void(MenuItem &, bool)> onClicked) :
			label(label),
			image(front),
			onClicked(onClicked) {

		}

		MenuItem(const std::string &label, const std::string &hint, const std::string &settingKey, int defaultValue, std::function<void(MenuItem &, bool)> onClicked) :
			label(label),
			hint(hint),
			settingKey(settingKey),
			defaultValue(defaultValue),
			onClicked(onClicked) {

		}

		MenuItem(const MenuItem &right) = default;

		std::string label;
		std::string image;
		std::string hint;
		std::string settingKey;
		std::vector<std::string> settingValues;
		int defaultValue;
		int value;
		std::function<void(MenuItem &, bool)> onClicked;
	};

	explicit Menu(Engine *engine);
	~Menu();

	void Init();
	void Resize();
	void Render();
	void Cleanup();

	void OnClick(int action);

	void Back();

	void Show();

	std::size_t GetSelectedPage() const { return selectedPage; }

	const MenuItem &GetSetting(const std::string &key) const { return settingsMenuItems.items[settingsMenuItems.keyedItems.at(key)]; }

	float *GetHalfVertexBuffer() { return halfVertexBuffer; }
	static float *GetHalfTextureBuffer() { return halfTextureBuffer; }
	static float *GetHalfTextureBufferReverse() { return halfTextureBufferReverse; }

private:
	enum class State {
		Main,
		Book,
		Settings
	};

	enum class AnimationState {
		None,
		Fade,
		In,
		Move,
		Open,
		Done
	};
	friend AnimationState &operator++(AnimationState &c);

	class MenuItems {
	public:
		MenuItems() = default;
		MenuItems(std::vector<MenuItem> &&items) :
			items(std::move(items)) {
			KeyItems();
		}
		MenuItems(const std::vector<MenuItem> &items) :
			items(items) {
			KeyItems();
		}
		virtual ~MenuItems() {}

		std::vector<MenuItem> items;
		std::map<std::string, std::size_t> keyedItems;

	private:
		void KeyItems() {
			for (const auto &[i, item] : Enumerate(items)) {
				if (!item.settingKey.empty()) {
					keyedItems[item.settingKey] = i;
				}
			}
		}
	};

	void SetCurrentMenuItems(MenuItems *menuItems);
	void DoWithMenuItems(std::function<bool(const OpenGLFont::FontGlyph &, float, float, std::size_t, const MenuItem &)> f);
	void ScaleMenuItems();

	MenuItems *GetBookMenuItems();
	MenuItems *GetMenuItemsForBook(const Book &book);

	Engine *engine = nullptr;

	std::unique_ptr<OpenGLFont> headerFont;
	std::unique_ptr<OpenGLFont> menuFont;

	State state = State::Main;

	MenuItems mainMenuItems;
	MenuItems settingsMenuItems;
	MenuItems *currentMenuItems = nullptr;

	std::optional<std::size_t> hoveredIndex = std::nullopt;

	std::vector<Book> books;

	OpenGLFont::FontGlyph headerBounds;

	const std::string back;

	std::map<std::string, Book::Page::Image> covers;

	AnimationState animationState = AnimationState::None;
	std::unique_ptr<Ease<float>> ease;
	std::optional<std::reference_wrapper<const Book>> selectedBook;
	std::size_t selectedPage;

	std::mutex loadingMutex;

	Curl curl;
	float halfVertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	static float halfTextureBuffer[8];
	static float halfTextureBufferReverse[8];

	bool scrollBarActive = false;
	float scrollBarVertexBuffer[8] = { 0, 0, 0, 15.0f, 0, 15.0f, 0, 0.0f };
	float scrollbarXPos = 0.0f;
	float scrollBarYPos = 0.0f;
	float totalWidth = 0.0f;
	bool hoveredOverScrollbar = false;
	bool draggingScrollbar = false;

	Checkbox checkbox;

	float backgroundRectVertexBuffer[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	Book::Page::Image backgroundChip;

	std::atomic<bool> loaded = false;
};
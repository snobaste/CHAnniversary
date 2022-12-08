#pragma once

#include "Audio.hpp"
#include "Book.hpp"
#include "InputManager.hpp"
#include "Menu.hpp"
#include "Renderer.hpp"

class Engine {
public:
	enum class State {
		Menu,
		Loading,
		Book
	};

	Engine(GLFWwindow *window) :
		window(window),
		audio(std::make_unique<Audio>(this)),
		renderer(std::make_unique<Renderer>(this)),
		manager(std::make_unique<InputManager>(this)),
		menu(std::make_unique<Menu>(this)) {

	}

	std::unique_ptr<Audio> &GetAudio() { return audio; }
	std::unique_ptr<Renderer> &GetRenderer() { return renderer; }
	std::unique_ptr<InputManager> &GetManager() { return manager; }
	std::unique_ptr<Menu> &GetMenu() { return menu; }

	void SetBook(std::shared_ptr<Book> book) { this->book = book; renderer->SetBook(book); }
	const std::shared_ptr<Book> &GetBook() const { return book; }

	const State &GetState() const { return state; }
	void SetState(State state) { this->state = state; }

	GLFWwindow *GetWindow() { return window; }
private:
	GLFWwindow *window = nullptr;

	std::unique_ptr<Renderer> renderer;
	std::unique_ptr<InputManager> manager;
	std::unique_ptr<Audio> audio;
	std::unique_ptr<Menu> menu;

	std::shared_ptr<Book> book = nullptr;

	State state = State::Menu;
};
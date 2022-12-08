#pragma once

#include <map>
#include <memory>

struct GLFWwindow;
class Engine;
class InputManager {
public:
	InputManager(Engine *engine);
	void ProcessInput(GLFWwindow *window);

	void SetDeltaTime(float deltaTime);
	const float GetDeltaTime() const { return deltaTime; }

	void SetFocued(bool focused) { this->focused = focused; }
	bool GetFocued() const { return focused; }

	auto GetMouseX() const { return mouseX; }
	auto GetMouseY() const { return mouseY; }

	std::pair<double, double> GetMousePos() const { return { GetMouseX(), GetMouseY() }; }
	std::pair<double, double> GetMouseDelta() const { return { GetMouseX() - lastMouseX, GetMouseY() - lastMouseY }; }

	void OnMouseClicked(int button, int action, int mods);

	int GetMouseButtonState(int button) const;

private:
	Engine *engine;

	float deltaTime = 0.0f;
	bool focused = true;

	double mouseX = 0.0, mouseY = 0.0;
	double lastMouseX = 0.0, lastMouseY = 0.0;

	std::map<int, int> mouseButtonStates;
};
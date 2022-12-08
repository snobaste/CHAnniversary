#include "InputManager.hpp"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Engine.hpp"

InputManager::InputManager(Engine *engine) :
	engine(engine) {
}

void InputManager::SetDeltaTime(float deltaTime) { 
	this->deltaTime = deltaTime;
	engine->GetRenderer()->SetDeltaTime(deltaTime);
}

void InputManager::ProcessInput(GLFWwindow *window) {
	if (!focused) return;

	lastMouseX = mouseX;
	lastMouseY = mouseY;
	glfwGetCursorPos(window, &mouseX, &mouseY);
}

void InputManager::OnMouseClicked(int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		// If we're not focused, we now will be
		if (!focused) focused = true;
		ProcessInput(engine->GetWindow());

		const auto &state = engine->GetState();

		if (state == Engine::State::Menu) {
			engine->GetMenu()->OnClick(action);
		} else if (state == Engine::State::Book && action == GLFW_PRESS) {
			engine->GetRenderer()->OnMouseClicked(mouseX, mouseY, button, mods);
		}
	}

	mouseButtonStates[button] = action;
}

int InputManager::GetMouseButtonState(int button) const {
	if (auto iter = mouseButtonStates.find(button);
		iter != mouseButtonStates.end() && iter->second == GLFW_PRESS)
		return true;

	return false;
}
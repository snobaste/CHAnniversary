#include <iostream>
#include <thread>

#include "third_party/fpng/fpng.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

#include "Filesystem/FileRepository.hpp"
#include "Filesystem/Registry/WindowsRegistry.hpp"
#include "Language/LanguageUtils.hpp"
#include "Rendering/OpenGLFont.hpp"

#include "Engine.hpp"
#include "Book.hpp"

using namespace SnobasteCPP;

constexpr auto WindowWidth = 1920;
constexpr auto WindowHeight = 1080;

void scroll(GLFWwindow *window, double x, double y) {
	//z -= y;
}

void window_focus_callback(GLFWwindow *window, int focused) {
	auto engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

	engine->GetManager()->SetFocued(focused == GLFW_TRUE);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
	// Ignore minimizing
	if (width == 0 || height == 0) return;

	auto engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

	engine->GetRenderer()->Resize(width, height);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	auto engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));
	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_SPACE:
			engine->GetRenderer()->AdvancePage();
			break;
		case GLFW_KEY_ESCAPE:
			if (engine->GetState() == Engine::State::Menu)
				engine->GetMenu()->Back();
			else if (engine->GetState() == Engine::State::Book)
				engine->GetRenderer()->Back();

			break;
		case GLFW_KEY_RIGHT:
		case GLFW_KEY_LEFT:
			if (engine->GetState() == Engine::State::Book)
				engine->GetRenderer()->AdvancePage(true, key == GLFW_KEY_LEFT);
			break;
		}
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	auto engine = static_cast<Engine *>(glfwGetWindowUserPointer(window));

	engine->GetManager()->OnMouseClicked(button, action, mods);
}

int main(int argc, char *argv[]) {
	FileRepository::registry = new WindowsRegistry("CHAnniversary");
	LanguageUtils::SetCurrentLanguage("en-us");

	fpng::fpng_init();

	glfwInit();

	glfwWindowHint(GLFW_SAMPLES, 1);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);

	GLFWwindow *window = glfwCreateWindow(WindowWidth, WindowHeight, "CHAnniversary", nullptr, nullptr);
	GLFWimage image;

	auto icon = new Book::Page::Image();
	icon->relativePath = "Images/chibi.png";
	fpng::fpng_decode_file(
		std::filesystem::absolute(FileRepository::registry->GetResourceDirectory() / icon->relativePath).string().c_str(),
		icon->data,
		icon->width,
		icon->height,
		icon->channels,
		4
	);
	image.width = icon->width;
	image.height = icon->height;
	image.pixels = icon->data.data();

	glfwSetWindowIcon(window, 1, &image);
	delete icon;
	glfwSetWindowSizeLimits(window, 1280, 720, GLFW_DONT_CARE, GLFW_DONT_CARE);
	if (window == nullptr) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	auto engine = std::make_unique<Engine>(window);
	glfwSetWindowUserPointer(window, engine.get());

	//SnobasteCPP::ParticleSystem system(*engine.renderer);

	glfwSetScrollCallback(window, scroll);
	glfwSetWindowFocusCallback(window, window_focus_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	//system.OnInit(0.0f, 0.0f);

	//engine.renderer->Init();

	// Enable depth test
	//glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	//glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	//glEnable(GL_CULL_FACE);

	//GLuint image = SnobasteCPP::Utils::LoadBMP("../../uvtemplate.bmp");
	//glfwSetCursorPos(window, engine.renderer->GetWidth() / 2.0, engine.renderer->GetHeight() / 2.0);

	//engine.GetRenderer()->Resize(WindowWidth, WindowHeight);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Hardcode first book for now
	/*
	std::thread([&] {
		engine->SetState(Engine::State::Loading);
		auto book = std::make_shared<Book>(FileRepository::registry->GetResourceDirectory() / "Books" / "OneInATerabyte.json");
		engine->SetBook(book);
		engine->SetState(Engine::State::Book);
	}).detach();
	*/

	engine->GetRenderer()->Init();

	const auto &resolutionSetting = engine->GetMenu()->GetSetting("Resolution");
	auto resolution = StringUtils::Split(resolutionSetting.settingValues[resolutionSetting.value], "x");
	framebuffer_size_callback(window, std::stoi(resolution[0]), std::stoi(resolution[1]));

#ifdef _WIN32
	// Turn on vertical screen sync under Windows.
	// (I.e. it uses the WGL_EXT_swap_control extension)
	typedef BOOL(WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int interval);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = NULL;
	wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	if (wglSwapIntervalEXT)
		wglSwapIntervalEXT(1);
#endif

	double lastTime = glfwGetTime();
	while (!glfwWindowShouldClose(window)) {
		double currentTime = glfwGetTime();
		engine->GetManager()->SetDeltaTime(currentTime - lastTime);
		engine->GetManager()->ProcessInput(window);

		// Clear background
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		//glBindTexture(GL_TEXTURE_2D, image);
		auto load = engine->GetRenderer()->Render();
		//system.OnLoop();
		//system.OnRender(0);

		glfwSwapBuffers(window);

		// If we had to load something during our
		// render, toss out this frametime
		lastTime = load ? glfwGetTime() : currentTime;

		glfwPollEvents();
	}

	// Kill engine before terminating GL
	engine->GetRenderer()->Cleanup();
	engine.reset();

	glfwTerminate();
	return 0;
}

#ifdef WIN32
int WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nShowCmd
) {
	return main(1, nullptr);
}
#endif
#include "Engine.hpp"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "Camera.hpp"
#include "GLFW.hpp"
#include "HelloWorld.hpp"
#include "Image.hpp"
#include "Input.hpp"
#include "Utils.hpp"

vk::Engine::Engine()
{
	m_isRunning = false;
	m_lastFrameTime = 0.0;
}

bool vk::Engine::Initialize()
{
	// TODO: Could probably store this somewhere else
	int windowWidth = 1280;
	int windowHeight = 720;

	Platform::get().StartUp(windowWidth, windowHeight);

	if (m_context.MakeContext(Platform::get().window, 1280, 720))
	{
		m_isRunning = true;
	}

	std::printf("Engine initialized\n");
	m_Renderer = std::make_unique<Renderer>(m_context);

	return m_isRunning;
}


void vk::Engine::Shutdown()
{
	m_Renderer->Destroy();
	m_Renderer.reset();
	m_context.Destroy(); // Free vulkan device, allocator, window 
	Platform::get().ShutDown();
}

void vk::Engine::Run()
{
	// Physics hello world!
	HelloWorld();

	while (m_isRunning && !glfwWindowShouldClose(m_context.mWindow))
	{
		double currentFrameTime = glfwGetTime();
		deltaTime = currentFrameTime - m_lastFrameTime;
		m_lastFrameTime = currentFrameTime;

		PollInputEvents();

		Update(deltaTime);

		Render();
	}

	Shutdown();
}

void vk::Engine::UpdateLogic() {
	if (IsKeyDown(KEY::_ESCAPE)) {
		glfwSetWindowShouldClose(Platform::get().window, GLFW_TRUE);
	}

	auto camera =
		static_cast<Camera *>(glfwGetWindowUserPointer(Platform::get().window));
	assert(camera);

	auto &inputMap = camera->inputMap;
	camera->SetInput(EInputState::FORWARD, IsKeyDown(KEY::_W));
	camera->SetInput(EInputState::BACKWARD, IsKeyDown(KEY::_S));
	camera->SetInput(EInputState::LEFT, IsKeyDown(KEY::_A));
	camera->SetInput(EInputState::RIGHT, IsKeyDown(KEY::_D));

	camera->SetInput(EInputState::DOWN, IsKeyDown(KEY::_Q));
	camera->SetInput(EInputState::UP, IsKeyDown(KEY::_E));

	camera->SetInput(EInputState::FAST, IsKeyDown(KEY::_LEFT_SHIFT));
	camera->SetInput(EInputState::SLOW, IsKeyDown(KEY::_LEFT_CONTROL));

	if (IsKeyPressed(KEY::_5)) {
		postProcessSettings.Enable =
			postProcessSettings.Enable == true ? false : true;

		const std::string result =
			postProcessSettings.Enable == true ? "Enabled"
												: "Disabled";

		SPDLOG_INFO("Post process: {}", result);
	}
}

void vk::Engine::Update(double deltaTime)
{
	UpdateLogic();
	m_Renderer->Update(deltaTime);
}

void vk::Engine::Render()
{
	m_Renderer->Render();
}

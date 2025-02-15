#include "Engine.hpp"
#include "Image.hpp"
#include <glm/glm.hpp>
#include "Utils.hpp"

#include "HelloWorld.hpp"

#include "GLFW.hpp"

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

		PollEvents();
		Update(deltaTime);
		Render();
	}

	Shutdown();
}

void vk::Engine::Update(double deltaTime)
{
	m_Renderer->Update(deltaTime);
}

void vk::Engine::Render()
{
	m_Renderer->Render();
}

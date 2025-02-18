#include "Engine.hpp"

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

#include "Camera.hpp"
#include "GLFW.hpp"
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

	if (m_context.MakeContext(Platform::get().window))
	{
		m_isRunning = true;
	}

	std::printf("Engine initialized\n");

	m_Renderer = std::make_unique<Renderer>(m_context);

    // ---PHYSICS TEST INITIALISATION---
    // make settings to create the sphere
    BodyCreationSettings sphere_settings(new SphereShape(0.5f), RVec3(0.0_r, 2.0_r, 0.0_r), Quat::sIdentity(), EMotionType::Dynamic, Layers::MOVING);
    
    // add the sphere to the physics system
    RigidBody ball = RigidBody(sphere_settings, &m_Physics);

    // add linear velocity to the ball
    m_Physics.body_interface.SetLinearVelocity(ball.ID, Vec3(0.0f, 5.0f, 0.0f));
    
    // add the floor using the default constructor
    [[maybe_unused]]RigidBody floor = RigidBody(RigidBody::Floor, &m_Physics);

    // ---END OF PHYSICS TEST INITIALISATION---

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
	while (m_isRunning && !glfwWindowShouldClose(m_context.mWindow))
	{
		double currentFrameTime = glfwGetTime();
		deltaTime = currentFrameTime - m_lastFrameTime;
		m_lastFrameTime = currentFrameTime;

		PollInputEvents();

		Update(deltaTime);
        
        m_Physics.UpdatePhysics(deltaTime);

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

	if (IsMouseButtonPressed(MOUSE_BUTTON::_RIGHT)) {
		auto& flag = camera->inputMap[std::size_t(EInputState::MOUSING)];
		flag = !flag;

		if (flag) {
			glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		} else {
			glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
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

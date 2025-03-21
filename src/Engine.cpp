#include "Engine.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <spdlog/spdlog.h>

#include <tracy/Tracy.hpp>

#include "Camera.hpp"
#include "Entity.hpp"
#include "GLFW.hpp"
#include "Image.hpp"
#include "Input.hpp"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Body/Body.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/SphereShape.h"
#include "PhysicsHelpers.hpp"
#include "PhysicsManager.hpp"
#include "RigidBody.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "glm/ext/quaternion_trigonometric.hpp"
#include "glm/fwd.hpp"
#include "glm/trigonometric.hpp"


#include <Jolt/Physics/Collision/Shape/ConvexHullShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/PhysicsMaterialSimple.h>
#include <Jolt/Geometry/Triangle.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Core/HashCombine.h>
#include <Jolt/Geometry/IndexedTriangle.h>

#define TEMP_DISABLE_PHYSICS 0

Engine::Engine() {
    m_isRunning = false;
    m_lastFrameTime = 0.0;
}

bool Engine::Initialize() {
    // TODO: Could probably store this somewhere else
    int windowWidth = 1280;
    int windowHeight = 720;

    Platform::get().StartUp(windowWidth, windowHeight);

    if (m_context.MakeContext(Platform::get().window)) {
        m_isRunning = true;
    }

    mScene = std::make_shared<Scene>(m_context);

    mRenderer = std::make_unique<Renderer>(m_context, mScene);
    
    PhysicsManager::get().StartUp();

    mScene->Initialise();

    mRenderer->CreateRenderPasses();
    // call the scene awake function
    mScene->Awake();

    // ---END OF PHYSICS TEST INITIALISATION---

    SPDLOG_DEBUG("Engine initialised.");



    return m_isRunning;
}

void Engine::Shutdown() {
    mRenderer->Destroy();
    mRenderer.reset();
    mScene->Destroy();
    m_context.Destroy(); // Free vulkan device, allocator, window
    Platform::get().ShutDown();
    PhysicsManager::get().ShutDown();
}

void Engine::Run() {
    auto camera = static_cast<Camera *>(glfwGetWindowUserPointer(Platform::get().window));
    camera->SetPhysics(&PhysicsManager::get());
    camera->SetScene(mScene.get());

    m_lastFrameTime = glfwGetTime();

    while (m_isRunning && !glfwWindowShouldClose(m_context.mWindow)) {
        double currentFrameTime = glfwGetTime();
        GlobalUtil::deltaTime = currentFrameTime - m_lastFrameTime;
        m_lastFrameTime = currentFrameTime;

        // See imgui.cpp
        // "(So you want to try calling NewFrame() as early as you can in your main loop to be able to use Dear ImGui everywhere)"
        ImGuiRenderer::NewFrame();

        PollInputEvents();

        Update(GlobalUtil::deltaTime);

        ImGuiRenderer::EndFrame();

        Render();

        FrameMark;
    }

    Shutdown();
}

void Engine::UpdateLogic() {
    if (IsKeyDown(KEY::eESCAPE)) {
        glfwSetWindowShouldClose(Platform::get().window, GLFW_TRUE);
    }

    auto camera = static_cast<Camera *>(glfwGetWindowUserPointer(Platform::get().window));
    assert(camera);

    camera->SetInput(EInputState::FORWARD, IsKeyDown(KEY::eW));
    camera->SetInput(EInputState::BACKWARD, IsKeyDown(KEY::eS));
    camera->SetInput(EInputState::LEFT, IsKeyDown(KEY::eA));
    camera->SetInput(EInputState::RIGHT, IsKeyDown(KEY::eD));

    camera->SetInput(EInputState::DOWN, IsKeyDown(KEY::eQ));
    camera->SetInput(EInputState::UP, IsKeyDown(KEY::eE));

    camera->SetInput(EInputState::FAST, IsKeyDown(KEY::eLEFT_SHIFT));
    camera->SetInput(EInputState::SLOW, IsKeyDown(KEY::eLEFT_CONTROL));

    camera->SetInput(EInputState::SWITCHVIEW, IsKeyPressed(KEY::eV));

    camera->SetInput(EInputState::TELEPORT, IsKeyPressed(KEY::eT));

    camera->SetInput(EInputState::ZOOM_IN, IsKeyPressed(KEY::eY));
    camera->SetInput(EInputState::ZOOM_OUT, IsKeyPressed(KEY::eU));

    if (IsKeyPressed(KEY::e5)) {
        vkutil::postProcessSettings.Enable = vkutil::postProcessSettings.Enable == true ? false : true;

        const std::string result = vkutil::postProcessSettings.Enable == true ? "Enabled" : "Disabled";

        SPDLOG_INFO("Post process: {}", result);
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON::eRIGHT)) {
        auto &flag = camera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = !flag;

        if (flag) {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    if(IsKeyPressed(KEY::eP))
    {
        SPDLOG_INFO("Camera Location: {}", glm::to_string(camera->GetPosition()));
    }
}

void Engine::Update(double deltaTime) {
    ZoneScopedN("Engine::Update");

    UpdateLogic();
    mScene->Update(deltaTime);
    mScene->UpdateUi(deltaTime);
    PhysicsManager::get().UpdatePhysics(deltaTime);
    mRenderer->Update(deltaTime);

}

void Engine::Render() {
    ZoneScopedN("Engine::Render");

    mRenderer->Render();
}

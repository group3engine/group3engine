#include "Engine.hpp"

#include <glm/ext.hpp>
#include <glm/glm.hpp>

#include <spdlog/spdlog.h>

#include "Camera.hpp"
#include "GLFW.hpp"
#include "Image.hpp"
#include "Input.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"
#include "Utils.hpp"

Engine::Engine() {
    m_isRunning = false;
    m_lastFrameTime = 0.0;
}

void Engine::InitScene() {
    // Current path is the current working directory, i.e., where the root CMakeLists.txt is
	std::filesystem::path basePath = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets";
	std::filesystem::path gltfPath = basePath / Sample::Sponza;

    // Define Light sources
    Light directionalLight;
    directionalLight.Type = LightType::Directional;
    directionalLight.position = glm::vec4(-8.161, 24.2f, 4.0f, 1.0f); // -0.2972
    directionalLight.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    std::vector<glm::vec4> spotLightPositions;

    // Random spot light positions put side by side each other
    for (size_t i = 0; i < 25; i++) {
        spotLightPositions.push_back(glm::vec4(-9.0 + i * 0.8, 0.7f, 0.5f, 1.0f));
    }

    // Create the scene which will store models and lights
    // Add GLTF to the scene
    // Add a directional light source defined earlier
    mScene->Load(gltfPath);
    mScene->AddLightSource(directionalLight);

    // Loop through the positions and instantiate a light
    // and pass to the scene to add the lights to the scene
    for (const auto &position : spotLightPositions) {
        Light spotLight = {};
        spotLight.Type = LightType::Spot;
        spotLight.position = position;
        spotLight.colour = glm::vec4(glm::linearRand(0.0f, 1.0f), glm::linearRand(0.0f, 1.f),
                                     glm::linearRand(0.0f, 1.0f), 1.0f);
        mScene->AddLightSource(spotLight);
    }

    SPDLOG_DEBUG("Number of Lights: {}", mScene->GetLights().size());
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

    // NOTE: This has to come after the renderer has been initialised currently.
    InitScene();

    // NOTE: This has to happen after the scene has been initialised currently
    // TODO: Not ideal having this as a separate public method just to be able
    // to initialise the render passes correctly after the scene is intialised.
    mRenderer->CreateRenderPasses();

    PhysicsManager::get().StartUp();

    // ---PHYSICS TEST INITIALISATION---

    RigidBody floor(RigidBody::Floor);
    // Add rigid body to the physics system
    // NOTE: Doing this outside of the constructor gives us a bit more flexibility
    floor.Init(PhysicsManager::get());

    auto &frontEntity = mScene->m_Entities.front();
    frontEntity.AddRigidBody(std::make_unique<RigidBody>(RigidBody::Ball));
    frontEntity.mRigidBody->Init(PhysicsManager::get());

    // Add linear velocity to the front entity (ball)
    PhysicsManager::get().mPhysicsSystem.GetBodyInterface().SetLinearVelocity(
        frontEntity.mRigidBody->mBodyId, Vec3(0.0f, 5.0f, 0.0f));

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
    while (m_isRunning && !glfwWindowShouldClose(m_context.mWindow)) {
        double currentFrameTime = glfwGetTime();
        GlobalUtil::deltaTime = currentFrameTime - m_lastFrameTime;
        m_lastFrameTime = currentFrameTime;

        PollInputEvents();

        Update(GlobalUtil::deltaTime);

        PhysicsManager::get().UpdatePhysics(1.f / 60.f);

        Render();
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
}

void Engine::Update(double deltaTime) {
    UpdateLogic();
    mScene->Update();
    mRenderer->Update(deltaTime);
}

void Engine::Render() {
    mRenderer->Render();
}

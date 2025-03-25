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

#include "Config.hpp"

#define TEMP_DISABLE_PHYSICS 0

namespace {
    // TODO: Improve this temporary scene switching mechanism

    enum class SceneValue {
        OBBY,
        OBBY_TEST_SCENE
    };

    SceneValue sceneValue{SceneValue::OBBY_TEST_SCENE};

    const std::filesystem::path &SwitchScene() {
        if (sceneValue == SceneValue::OBBY) {
            sceneValue = SceneValue::OBBY_TEST_SCENE;
            return Sample::SampleObbyTestScene;
        } else if (sceneValue == SceneValue::OBBY_TEST_SCENE) {
            sceneValue = SceneValue::OBBY;
            return Sample::SampleObby;
        } else {
            SPDLOG_ERROR("Unaccounted for case.");
            exit(EXIT_FAILURE);
        }
    }
}

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

    mMaterialManager = std::make_unique<MaterialManager>(m_context);
    mMaterialManager->Initialise();

    // Don't need to reinitialise mMeshManager, data can just be added again
    mMeshManager = std::make_unique<MeshManager>(m_context);

    mTextureManager = std::make_unique<TextureManager>(m_context);
    mTextureManager->Initialise();

    mScene = std::make_shared<Scene>(m_context,
                                     mMaterialManager.get(),
                                     mMeshManager.get(),
                                     mTextureManager.get());
    mScene->StartUp();

    mRenderer = std::make_unique<Renderer>(m_context, mScene);
    
    PhysicsManager::get().StartUp();

    mScene->Initialise(Sample::SampleObbyTestScene);

    mRenderer->CreateRenderPasses();
    // call the scene awake function
    mScene->Awake();

#ifdef JPH_DEBUG_RENDERER
    mDebugRenderer = std::make_unique<DebugRendererImp>(mRenderer.get());
#endif // JPH_DEBUG_RENDERER

    SPDLOG_DEBUG("Engine initialised.");



    return m_isRunning;
}

void Engine::Shutdown() {
#ifdef JPH_DEBUG_RENDERER
    static_cast<DebugRendererImp*>(mDebugRenderer.get())->Destroy();
#endif // JPH_DEBUG_RENDERER

    mRenderer->Destroy();
    mRenderer.reset();
    mScene->Destroy();

    mMeshManager->Destroy();
    mMaterialManager->Destroy();
    mTextureManager->Destroy();

    m_context.Destroy(); // Free vulkan device, allocator, window
    Platform::get().ShutDown();
    PhysicsManager::get().ShutDown();
}

void Engine::ChangeScene(const std::filesystem::path &filePath)
{
    m_sceneNeedsChanging = true;
    m_scenePath = filePath;
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

        if (m_sceneNeedsChanging)
        {
            ChangeSceneFR();
            m_sceneNeedsChanging = false;
        }



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

void Engine::ChangeSceneFR()
{
    // vkDestroyBuffer():  can't be called on VkBuffer that is currently in use by VkCommandBuffer
    vkQueueWaitIdle(m_context.graphicsQueue);
    vkQueueWaitIdle(m_context.presentQueue);

    mScene->Destroy();
    mMaterialManager->Destroy();
    mMeshManager->Destroy();
    mTextureManager->Destroy();

    mMaterialManager->Initialise();

    // Don't need to reinitialise mMeshManager, data can just be added again

    mTextureManager->Initialise();

    // Remove all UI textures as they were linked with the texture manager
    ImGuiRenderer::RemoveTextures();

#ifndef NDEBUG
    // Check there are no physics bodies left after scene destruction
    BodyIDVector bodyIds;
    PhysicsManager::get().mPhysicsSystem.GetBodies(bodyIds);
    assert(bodyIds.empty());
#endif // #ifndef NDEBUG

    mScene->StartUp();
    mScene->Initialise(m_scenePath);

    // Add back UI textures
    ImGuiRenderer::AddTextures(mTextureManager.get());

    mRenderer->RebuildSceneDescriptors();

    mScene->Awake();
}

void Engine::Update(double deltaTime) {
    ZoneScopedN("Engine::Update");

    UpdateLogic();
    mScene->Update(deltaTime);
    mScene->UpdateUi(deltaTime);

// Draw physics before physics update
// TODO: Understand why Jolt does this
#ifdef JPH_DEBUG_RENDERER
    if (GlobalConfig::enablePhysicsDebugRenderer) {
        auto cameraPos = mRenderer->GetCamera()->GetPosition();
        mDebugRenderer.get()->SetCameraPos(RVec3{cameraPos.x, cameraPos.y, cameraPos.z});

        // Create render primitives: vertex buffers, index buffers and store them for later
        // Except for lines, we will create the primitives at draw time
        DrawPhysics();
    }
#endif // JPH_DEBUG_RENDERER

    PhysicsManager::get().UpdatePhysics(deltaTime);
    mRenderer->Update(deltaTime);
}

#ifdef JPH_DEBUG_RENDERER
void Engine::DrawPhysics() {
    ZoneScopedN("DrawPhysics");

    JPH::BodyManager::DrawSettings bodyDrawSettings;
    bodyDrawSettings.mDrawShape = true;
    PhysicsManager::get().mPhysicsSystem.DrawBodies(bodyDrawSettings, mDebugRenderer.get());
}
#endif // JPH_DEBUG_RENDERER

void Engine::Render() {
    ZoneScopedN("Engine::Render");

    mRenderer->BeginFrame(mRenderer->GetCommandBuffer());

    {
        mRenderer->GetShadowMap()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetDepthPrepass()->Execute(mRenderer->GetCommandBuffer());

        mRenderer->GetForwardPass()->BeginExecute(mRenderer->GetCommandBuffer());

#ifdef JPH_DEBUG_RENDERER
        if (GlobalConfig::enablePhysicsDebugRenderer) {
            static_cast<DebugRendererImp*>(mDebugRenderer.get())->Draw();
        }
#endif // JPH_DEBUG_RENDERER

        mRenderer->GetForwardPass()->EndExecute(mRenderer->GetCommandBuffer());

        mRenderer->GetGBuffer()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetSSAO()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetSSR()->Execute(mRenderer->GetCommandBuffer());

        mRenderer->GetBloomPass()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetCompositePass()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetPresentPass()->Execute(mRenderer->GetCommandBuffer(), mRenderer->GetImageIndex());

        mRenderer->EndFrame(mRenderer->GetCommandBuffer());
    }
}

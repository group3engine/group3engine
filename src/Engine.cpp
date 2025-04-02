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

#include "imgui.h"

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

    Scene::get().StartUp(&m_context,
                         mMaterialManager.get(),
                         mMeshManager.get(),
                         mTextureManager.get());

    mScene = Scene::get().GetActiveScene();

    mRenderer = std::make_unique<Renderer>(m_context, mScene);
    
    PhysicsManager::get().StartUp();

    mScene->Load(Sample::SampleObbyTestScene);

    mRenderer->CreateRenderPasses();
    // call the scene awake function
    mScene->Awake();

    mRenderer->AddCameras();

#ifdef JPH_DEBUG_RENDERER
    mDebugRenderer = std::make_unique<DebugRendererImp>(mRenderer.get(), mScene);
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
    mScene->Unload();
    mScene->ShutDown();

    mMeshManager->Destroy();
    mMaterialManager->Destroy();
    mTextureManager->Destroy();

    m_context.Destroy(); // Free vulkan device, allocator, window
    Platform::get().ShutDown();
    PhysicsManager::get().ShutDown();
}

void Engine::ChangeScene()
{
    m_sceneNeedsChanging = true;
    m_scenePath = SwitchScene();
}

void Engine::Run() {
    for (auto *camera : mScene->GetCameras()) {
        camera->SetPhysics(&PhysicsManager::get());
        camera->SetScene(mScene);
    }

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

    if (IsKeyPressed(KEY::e5)) {
        vkutil::postProcessSettings.Enable = vkutil::postProcessSettings.Enable == true ? false : true;

        const std::string result = vkutil::postProcessSettings.Enable == true ? "Enabled" : "Disabled";

        SPDLOG_INFO("Post process: {}", result);
    }
}

void Engine::ChangeSceneFR()
{
    // vkDestroyBuffer():  can't be called on VkBuffer that is currently in use by VkCommandBuffer
    vkQueueWaitIdle(m_context.graphicsQueue);
    vkQueueWaitIdle(m_context.presentQueue);

    // Remove all UI textures as they were linked with the texture manager
    ImGuiRenderer::RemoveTextures();

    mScene->Unload();
    mMaterialManager->Destroy();
    mMeshManager->Destroy();
    mTextureManager->Destroy();

    mMaterialManager->Initialise();

    // Don't need to reinitialise mMeshManager, data can just be added again

    mTextureManager->Initialise();

    // load in heart
    std::filesystem::path loadingPath = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets" / "loadingImage.png";
    ImGuiRenderer::AddTextures(mTextureManager.get(), loadingPath, "load");


    m_isLoading = true;
    m_progress = 0.f;
    // std::thread loadingScreen(&Engine::RenderLoadingScreen, this);



#ifndef NDEBUG
    // Check there are no physics bodies left after scene destruction
    BodyIDVector bodyIds;
    PhysicsManager::get().mPhysicsSystem.GetBodies(bodyIds);
    assert(bodyIds.empty());
#endif // #ifndef NDEBUG
    m_progress = 25.f;
    mScene->Load(m_scenePath);
    m_progress = 75.f;

    // Add back UI textures
    std::filesystem::path path = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets" / "heart.png";
    ImGuiRenderer::AddTextures(mTextureManager.get(), path, "heart");

    mScene->Awake();
    m_progress = 100.f;
    // sleep for 1 second
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    // end the loading screen
    m_isLoading = false;
    // wait for loading screen thread to finish
    // while (!loadingScreen.joinable()) {}
    // loadingScreen.join();
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
        auto cameraPos = mScene->GetCameras()[0]->GetPosition();
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

void Engine::RenderLoadingScreen()
{
    // load in a new image from assets/loadingImage.png

    try
    {
        while (m_isLoading)
        {
            // ImGuiRenderer::NewFrame();
            // ImGuiRenderer::Image("load", ImVec2{0,0}, ImVec2{1,1});
            // ImGuiRenderer::LoadingBar(m_progress, ImVec2(500, 500));
            // ImGuiRenderer::EndFrame();
            // // render some text with imgui
            // mRenderer->RenderUIOnly();
        }
    }catch (const std::exception& e) {
        // Handle the exception
        SPDLOG_ERROR(e.what());
    }
}

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
#include "Fonts.hpp"
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
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // #ifndef WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif
#define TEMP_DISABLE_PHYSICS 0

namespace {
    // TODO: Improve this temporary scene switching mechanism
    std::filesystem::path mainMenuPath{"MainMenu/main_menu.gltf"};

    const std::vector<std::filesystem::path *> scenePaths = {
        &Sample::SampleObby,
        &Sample::SampleObbyTestScene,
        &Sample::ArrowSample,
        &Sample::TileSample,
        &Sample::SpikePitSample,
        &Sample::LadderSample,
        &Sample::SinkingSample,

    };

    const std::filesystem::path *scenePathSelection = scenePaths[0];

    const std::vector<const char *> playerCounts = {
        "1",
        "2",
        "3",
        "4"
    };

    const char *playerCountSelection = playerCounts[0];
}

Engine::Engine() {
    m_isRunning = false;
    m_lastFrameTime = 0.0;
}

bool Engine::Initialize() {


#ifdef PLATINUM
    // get the file path to the executable
    {
        #ifdef _WIN32
        char path[MAX_PATH];
        if (GetModuleFileNameA(nullptr, path, MAX_PATH)) {
            assetsPath =
                std::filesystem::path(path).parent_path().parent_path() /
                "assets";
        } else {
            SPDLOG_ERROR("Error getting executable path.");
            exit(EXIT_FAILURE);
        }
        #else
                char path[PATH_MAX];
                ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
                if (len != -1) {
                    path[len] = '\0'; // Null-terminate the string
                    assetsPath =
                        std::filesystem::path(path).parent_path().parent_path() /
                        "assets";
                } else {
                    SPDLOG_ERROR("Error getting executable path.");
                    exit(EXIT_FAILURE);
                }

        #endif
    }
#else
    assetsPath = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets";
#endif

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

    constexpr size_t mainMenuPlayerCount = 1;
    mScene->Load(mainMenuPath, mainMenuPlayerCount);
    m_scenePath = mScene->GetSceneFilename();

    mRenderer->CreateRenderPasses();
    // call the scene awake function
    mScene->Awake();

    mRenderer->AddCameras();

#ifdef JPH_DEBUG_RENDERER
    mDebugRenderer = std::make_unique<DebugRendererImp>(mRenderer.get(), mScene);
#endif // JPH_DEBUG_RENDERER

    SPDLOG_DEBUG("Engine initialised.");

    ImGuiRenderer::themes.applyTheme("Catpuccin Mocha");
    Fonts::LoadFonts();




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

void Engine::ChangeScene(const std::filesystem::path &pendingScenePath, size_t pendingPlayerCount) {
    m_sceneNeedsChanging = true;
    mPendingScenePath = pendingScenePath;
    mPendingScenePlayerCount = pendingPlayerCount;
}

void Engine::Run() {
    for (auto *camera : mScene->GetCameras()) {
        camera->SetPhysics(&PhysicsManager::get());
        camera->SetScene(mScene);
    }

    m_lastFrameTime = glfwGetTime();

    while (m_isRunning && !glfwWindowShouldClose(m_context.mWindow)) {
        mIsMainMenu = mScene->GetSceneFilename() == "main_menu";

        double currentFrameTime = glfwGetTime();
        GlobalUtil::unscaledDeltaTime = currentFrameTime - m_lastFrameTime;
        GlobalUtil::deltaTime = GlobalUtil::unscaledDeltaTime * m_timeScale;
        m_lastFrameTime = currentFrameTime;

        // See imgui.cpp
        // "(So you want to try calling NewFrame() as early as you can in your main loop to be able to use Dear ImGui everywhere)"
        ImGuiRenderer::NewFrame();

        PollInputEvents();

        Update(GlobalUtil::deltaTime);

        if (mIsMainMenu || m_timeScale == 0.f) {
            ImGuiRenderer::BeginMainMenu(m_context);
            playerCountSelection = ImGuiRenderer::AddMainMenuPlayerCountSelection(m_context, playerCounts, playerCountSelection);
            scenePathSelection = ImGuiRenderer::AddMainMenuSceneSelection(m_context, scenePaths, scenePathSelection);
            ImGuiRenderer::AddLoadSceneButton(*scenePathSelection, std::stoi(playerCountSelection));
            ImGuiRenderer::AddQuitButton();
            ImGuiRenderer::EndMainMenu();
        }


        ImGuiRenderer::EndFrame();

        Render();

        if (m_sceneNeedsChanging)
        {
            ChangeSceneFR(mPendingScenePath, mPendingScenePlayerCount);
            m_sceneNeedsChanging = false;
        }

        FrameMark;
    }

    Shutdown();
}

void Engine::UpdateLogic() {
    if (m_shouldQuit) {
        glfwSetWindowShouldClose(Platform::get().window, GLFW_TRUE);
    }

    if (IsKeyPressed(KEY::e5)) {
        vkutil::postProcessSettings.Enable = vkutil::postProcessSettings.Enable == true ? false : true;

        const std::string result = vkutil::postProcessSettings.Enable == true ? "Enabled" : "Disabled";

        SPDLOG_INFO("Post process: {}", result);
    }
}

void Engine::ChangeSceneFR(const std::filesystem::path &scenePath, size_t playerCount)
{
    // vkDestroyBuffer():  can't be called on VkBuffer that is currently in use by VkCommandBuffer
    vkQueueWaitIdle(m_context.graphicsQueue);
    vkQueueWaitIdle(m_context.presentQueue);

    // Remove all UI textures as they were linked with the texture manager
    ImGuiRenderer::RemoveTextures();

    mScene->Unload();
    LightManager::getInstance().Unload();
    mMaterialManager->Destroy();
    mMeshManager->Destroy();
    mTextureManager->Destroy();

    mMaterialManager->Initialise();

    // Don't need to reinitialise mMeshManager, data can just be added again

    mTextureManager->Initialise();

    // load in heart
    std::filesystem::path loadingPath = assetsPath/ "loadingImage.png";
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

    mScene->Load(scenePath, playerCount);
    m_scenePath = mScene->GetSceneFilename();

    m_progress = 75.f;

    // Add back UI textures
    std::filesystem::path path = assetsPath/ "heart.png";
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

    if (!mIsMainMenu) {
        mScene->UpdateUi(deltaTime);

#ifndef PLATINUM
        playerCountSelection = ImGuiRenderer::NewPlayerCountSelection(playerCounts, playerCountSelection);
        scenePathSelection = ImGuiRenderer::NewSceneSelection(scenePaths, scenePathSelection);
        ImGuiRenderer::AddLoadSceneButton(*scenePathSelection, std::stoi(playerCountSelection));
        ImGuiRenderer::Update(mScene);
#endif
    }

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



        //mRenderer->GetGBuffer()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetSSAO()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetSSR()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetFogPass()->Execute(mRenderer->GetCommandBuffer());

        mRenderer->GetBloomPass()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetCompositePass()->Execute(mRenderer->GetCommandBuffer());
        mRenderer->GetPresentPass()->Execute(mRenderer->GetCommandBuffer(), mRenderer->GetImageIndex());

        mRenderer->EndFrame(mRenderer->GetCommandBuffer());
    }
}

void Engine::RenderLoadingScreen()
{
    // load in a new image from loadingImage.png

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

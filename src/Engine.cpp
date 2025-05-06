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

#include "AudioManager.hpp"

namespace {
    // TODO: Improve this temporary scene switching mechanism
    std::filesystem::path mainMenuPath{"MainMenu/main_menu.gltf"};

    const std::vector<std::filesystem::path *> scenePaths = {
        &Sample::Game,
        &Sample::JumpTest,
        &Sample::SampleObbyTestScene,
        &Sample::ArrowSample,
        &Sample::AxeSample,
        &Sample::TileSample,
        &Sample::SpikePitSample,
        &Sample::LadderSample,
        &Sample::SinkingSample,
        &Sample::LeverSample,
        &Sample::BoulderSample,
        &Sample::SpikeTrapSample,
        &Sample::DisappearingPlatformSample,


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

    mIsMainMenu = true;

    mRenderer->CreateRenderPasses();
    // call the scene awake function
    mScene->Awake();

    mRenderer->AddCameras();

#ifdef JPH_DEBUG_RENDERER
    mDebugRenderer = std::make_unique<DebugRendererImp>(mRenderer.get(), mScene);

    mScene->SetDebugRenderer(mDebugRenderer.get());
#endif // JPH_DEBUG_RENDERER

    SPDLOG_DEBUG("Engine initialised.");

    ImGuiRenderer::themes.applyTheme("Catpuccin Mocha");
    Fonts::LoadFonts();

    AudioManager::get().StartUp();

    AudioManager::get().SetBackgroundMusic("main_menu_music");

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

    AudioManager::get().ShutDown();
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
        double currentFrameTime = glfwGetTime();
        GlobalUtil::unscaledDeltaTime = currentFrameTime - m_lastFrameTime;
        GlobalUtil::deltaTime = GlobalUtil::unscaledDeltaTime * m_timeScale;
        m_lastFrameTime = currentFrameTime;

        // See imgui.cpp
        // "(So you want to try calling NewFrame() as early as you can in your main loop to be able to use Dear ImGui everywhere)"
        {
            std::lock_guard<std::mutex> lock(m_context.graphicsQueueMutex);
            ImGuiRenderer::NewFrame();
        }

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
            // reset the last frame time to avoid a large delta time
            m_lastFrameTime = glfwGetTime();
            m_timeScale = 1.f;
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
    mSceneLoadingThread = std::thread(&Engine::LoadRestOfStuff, this, scenePath, playerCount);

    RenderLoadingScreen();

    // wait for loading screen thread to finish
     while (!mSceneLoadingThread.joinable()) {}
    mSceneLoadingThread.join();
    vkQueueWaitIdle(m_context.presentQueue);

    mIsMainMenu = mScene->GetSceneFilename() == "main_menu";

    if (mIsMainMenu) {
        AudioManager::get().SetBackgroundMusic("main_menu_music");
    } else {
        AudioManager::get().StopBackgroundMusic();
    }
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
}

#ifdef JPH_DEBUG_RENDERER
void Engine::DrawPhysics() {
    ZoneScopedN("DrawPhysics");

    JPH::BodyManager::DrawSettings bodyDrawSettings = {
        .mDrawGetSupportFunction = false,                                        ///< Draw the GetSupport() function, used for convex collision detection
        .mDrawSupportDirection = false,                                          ///< When drawing the support function, also draw which direction mapped to a specific support point
        .mDrawGetSupportingFace = false,                                         ///< Draw the faces that were found colliding during collision detection
        .mDrawShape = true,                                                      ///< Draw the shapes of all bodies
        .mDrawShapeWireframe = false,                                            ///< When mDrawShape is true and this is true, the shapes will be drawn in wireframe instead of solid.
        .mDrawShapeColor = BodyManager::EShapeColor::MotionTypeColor,            ///< Coloring scheme to use for shapes
        .mDrawBoundingBox = false,                                               ///< Draw a bounding box per body
        .mDrawCenterOfMassTransform = true,                                      ///< Draw the center of mass for each body
        .mDrawWorldTransform = true,                                             ///< Draw the world transform (which can be different than the center of mass) for each body
        .mDrawVelocity = false,                                                  ///< Draw the velocity vector for each body
        .mDrawMassAndInertia = false,                                            ///< Draw the mass and inertia (as the box equivalent) for each body
        .mDrawSleepStats = false,                                                ///< Draw stats regarding the sleeping algorithm of each body
        .mDrawSoftBodyVertices = false,                                          ///< Draw the vertices of soft bodies
        .mDrawSoftBodyVertexVelocities = false,                                  ///< Draw the velocities of the vertices of soft bodies
        .mDrawSoftBodyEdgeConstraints = false,                                   ///< Draw the edge constraints of soft bodies
        .mDrawSoftBodyBendConstraints = false,                                   ///< Draw the bend constraints of soft bodies
        .mDrawSoftBodyVolumeConstraints = false,                                 ///< Draw the volume constraints of soft bodies
        .mDrawSoftBodySkinConstraints = false,                                   ///< Draw the skin constraints of soft bodies
        .mDrawSoftBodyLRAConstraints = false,                                    ///< Draw the LRA constraints of soft bodies
        .mDrawSoftBodyPredictedBounds = false,                                   ///< Draw the predicted bounds of soft bodies
        .mDrawSoftBodyConstraintColor = ESoftBodyConstraintColor::ConstraintType ///< Coloring scheme to use for soft body constraints
    };
    PhysicsManager::get().mPhysicsSystem.DrawBodies(bodyDrawSettings, mDebugRenderer.get());
    // TODO: Draw constraints
    //       - Which needs DrawLine to be implemented
    PhysicsManager::get().mPhysicsSystem.DrawConstraints(mDebugRenderer.get());
}
#endif // JPH_DEBUG_RENDERER

void Engine::Render() {
    ZoneScopedN("Engine::Render");

    mRenderer->BeginFrame(mRenderer->GetCommandBuffer());

    mRenderer->Update(GlobalUtil::deltaTime);

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
        mRenderer->GetFXAAPass()->Execute(mRenderer->GetCommandBuffer());
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

            
            float mTotalTime = glfwGetTime() - m_lastFrameTime;
            float mProgress = m_progress;
            mProgress += mTotalTime * 10.f;
            if (m_progress < 25) {
                mProgress = std::min(mProgress, 25.f);
            } else if (m_progress < 85) {
                mProgress = std::min(mProgress, 85.f);
            }
            mProgress = std::min(mProgress, 99.f);
            {
                std::lock_guard<std::mutex> lock2(m_context.presentQueueMutex);
                std::lock_guard<std::mutex> lock(m_context.graphicsQueueMutex);
                ImGuiRenderer::NewFrame();
            }
            ImGuiRenderer::Image("load", ImVec2{0, 0}, ImVec2{1, 1});
            ImGuiRenderer::LoadingBar(mProgress, ImVec2(500, 500));
            ImGuiRenderer::EndFrame();
            {
                // render some text with imgui
                std::lock_guard<std::mutex> lock3(m_context.presentQueueMutex);
                mRenderer->RenderUIOnly();
            }
             // sleep for 50ms
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

        }
    }catch (const std::exception& e) {
        // Handle the exception
        SPDLOG_ERROR(e.what());
    }
}

void Engine::LoadRestOfStuff(const filesystem::path &scenePath, size_t playerCount)
{


#ifndef NDEBUG
    // Check there are no physics bodies left after scene destruction
    BodyIDVector bodyIds;
    PhysicsManager::get().mPhysicsSystem.GetBodies(bodyIds);
    assert(bodyIds.empty());
#endif // #ifndef NDEBUG
    m_progress = 25.f;

    mScene->Load(scenePath, playerCount);
    m_scenePath = mScene->GetSceneFilename();

    m_progress = 85.f;

    // Add back UI textures
    std::filesystem::path path = assetsPath/ "heart.png";
    ImGuiRenderer::AddTextures(mTextureManager.get(), path, "heart");

    mScene->Awake();
    m_progress = 100.f;
    // end the loading screen
    m_isLoading = false;
}

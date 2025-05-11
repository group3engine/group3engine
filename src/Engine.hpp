#pragma once

#include <memory>
#include <thread>

#include "Camera.hpp"
#include "Context.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "PhysicsManager.hpp"
#include "Renderer.hpp"
#include "RigidBody.hpp"
#include "UIManager.hpp"

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#include "DebugRendererImp.h"
#endif // JPH_DEBUG_RENDERER

// Menus
class MainMenuScreen;
class NewGameMenu;
class ConfigGameMenu;
class PauseMenu;

class MaterialManager;
class MeshManager;
class TextureManager;

struct UITexture {
    std::filesystem::path path;
    std::string name;
};

class Engine {
  private:
    Engine();
    ~Engine() = default;

  public:
    Engine(const Engine &) = delete;
    Engine &operator=(const Engine &) = delete;

    static Engine &get() {
        static Engine instance;
        return instance;
    }

    void SetTimeScale(float timeScale) { m_timeScale = timeScale; }
    float GetTimeScale() const { return m_timeScale; }
    bool IsPaused() const { return m_timeScale == 0.0f; }
    void Quit() {m_shouldQuit = true; }

  public:
    bool Initialize();
    void Run();
    void Shutdown();
    void ChangeScene(const std::filesystem::path &pendingScenePath, size_t pendingPlayerCount);

    static std::vector<std::filesystem::path *> &GetScenePaths();

  private:
    Context m_context;
    bool m_isRunning;
    double m_lastFrameTime;
    bool m_isLoading = false;
    float m_progress = 0.0f;
    float m_timeScale = 1.0f;
    bool m_shouldQuit = false;

    bool m_sceneNeedsChanging = false;
    std::filesystem::path m_scenePath;

    // TODO: Change this when entities call change scene
    std::filesystem::path mPendingScenePath = Sample::SampleObbyTestScene;
    size_t mPendingScenePlayerCount = 1;

    bool mIsMainMenu = false;

    void UpdateLogic();

    void ChangeSceneFR(const std::filesystem::path &scenePath, size_t playerCount);

    void Update(double deltaTime);
    void Render();

    void RenderLoadingScreen();
    void LoadRestOfStuff(const std::filesystem::path &scenePath, size_t playerCount);

#ifdef JPH_DEBUG_RENDERER
    void DrawPhysics();
#endif // JPH_DEBUG_RENDERER

    void InitGuiTextures();

    Scene *mScene;
    std::unique_ptr<Renderer> mRenderer;

#ifdef JPH_DEBUG_RENDERER
    std::unique_ptr<DebugRendererSimple> mDebugRenderer;
#endif // JPH_DEBUG_RENDERER

    // Managers
    std::unique_ptr<MeshManager> mMeshManager;
    std::unique_ptr<MaterialManager> mMaterialManager;
    std::unique_ptr<TextureManager> mTextureManager;

    // scene loading thread
    std::thread mSceneLoadingThread;
    MainMenuScreen* m_MainMenuScreen;
    NewGameMenu *m_NewGameMenu;
    ConfigGameMenu *m_ConfigGameMenu;
    PauseMenu *mPauseMenu = nullptr;
    UIManager m_UIManager;
    std::vector<UITexture> mGuiTextures;
};

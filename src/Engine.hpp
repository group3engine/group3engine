#pragma once

#include <memory>

#include "Camera.hpp"
#include "Context.hpp"
#include "PhysicsManager.hpp"
#include "Renderer.hpp"
#include "RigidBody.hpp"

#ifdef JPH_DEBUG_RENDERER
#include <Jolt/Renderer/DebugRendererSimple.h>
#include "DebugRendererImp.h"
#endif // JPH_DEBUG_RENDERER

class MaterialManager;
class MeshManager;
class TextureManager;

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

  public:
    bool Initialize();
    void Run();
    void Shutdown();
    void ChangeScene();

  private:
    Context m_context;
    bool m_isRunning;
    double m_lastFrameTime;
    bool m_isLoading = false;
    float m_progress = 0.0f;

    bool m_sceneNeedsChanging = false;
    std::filesystem::path m_scenePath;

    void UpdateLogic();

    void ChangeSceneFR();

    void Update(double deltaTime);
    void Render();

    void RenderLoadingScreen();

#ifdef JPH_DEBUG_RENDERER
    void DrawPhysics();
#endif // JPH_DEBUG_RENDERER

    Scene *mScene;
    std::unique_ptr<Renderer> mRenderer;

#ifdef JPH_DEBUG_RENDERER
    std::unique_ptr<DebugRendererSimple> mDebugRenderer;
#endif // JPH_DEBUG_RENDERER

    // Managers
    std::unique_ptr<MeshManager> mMeshManager;
    std::unique_ptr<MaterialManager> mMaterialManager;
    std::unique_ptr<TextureManager> mTextureManager;
};

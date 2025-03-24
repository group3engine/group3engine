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
  public:
    Engine();
    bool Initialize();
    void Run();
    void Shutdown();

  private:
    Context m_context;
    bool m_isRunning;
    double m_lastFrameTime;

    void UpdateLogic();

    void Update(double deltaTime);
    void Render();

#ifdef JPH_DEBUG_RENDERER
    void DrawPhysics();
#endif // JPH_DEBUG_RENDERER

    std::shared_ptr<Scene> mScene;
    std::unique_ptr<Renderer> mRenderer;

#ifdef JPH_DEBUG_RENDERER
    std::unique_ptr<DebugRendererSimple> mDebugRenderer;
#endif // JPH_DEBUG_RENDERER

    // Managers
    std::unique_ptr<MeshManager> mMeshManager;
    std::unique_ptr<MaterialManager> mMaterialManager;
    std::unique_ptr<TextureManager> mTextureManager;
};

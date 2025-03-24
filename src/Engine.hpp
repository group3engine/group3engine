#pragma once

#include <memory>

#include "Camera.hpp"
#include "Context.hpp"
#include "PhysicsManager.hpp"
#include "Renderer.hpp"
#include "RigidBody.hpp"

class MaterialManager;
class MeshManager;
class TextureManager;

class Engine {
public:
    static Engine& get();
private:
    static Engine* instance;
  public:
    Engine();
    bool Initialize();
    void Run();
    void Shutdown();
    void ChangeScene(const std::filesystem::path &filePath);

  private:
    Context m_context;
    bool m_isRunning;
    double m_lastFrameTime;

    bool m_sceneNeedsChanging;
    std::filesystem::path m_scenePath;

    void UpdateLogic();

    void ChangeSceneFR();

    void Update(double deltaTime);
    void Render();

    std::shared_ptr<Scene> mScene;
    std::unique_ptr<Renderer> mRenderer;

    // Managers
    std::unique_ptr<MeshManager> mMeshManager;
    std::unique_ptr<MaterialManager> mMaterialManager;
    std::unique_ptr<TextureManager> mTextureManager;
};

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
    void ChangeScene(const std::filesystem::path &filePath);

  private:
    Context m_context;
    bool m_isRunning;
    double m_lastFrameTime;

    bool m_sceneNeedsChanging = false;
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

#pragma once

#include <memory>

#include "Camera.hpp"
#include "Context.hpp"
#include "PhysicsManager.hpp"
#include "Renderer.hpp"
#include "RigidBody.hpp"

#include "CharacterVirtualTest.h"

class Engine {
  public:
    Engine();
    void InitScene();
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

    std::shared_ptr<Scene> mScene;
    std::unique_ptr<Renderer> mRenderer;

    CharacterVirtualTest mCharacterVirtualTest;
};

#ifndef PAUSE_MENU_HPP
#define PAUSE_MENU_HPP

#include "BaseMenu.hpp"
#include "Engine.hpp"
#include "SampleGLTFFilePaths.hpp"

class UIManager;
class Context;
class Scene;

class PauseMenu : public BaseMenu
{
  public:
    explicit PauseMenu(Context &context, UIManager &uiManager, Scene *scene);
    void Render(ImVec2 screenSize) override;
    void Resize() override;
    size_t NumPlayers;

    std::filesystem::path m_SelectedLevelPath = *Engine::GetScenePaths()[0]; // Default path is test scene

    inline static bool enableSceneSelection = true;

  private:
    Scene *mScene;
};

#endif // PAUSE_MENU_HPP

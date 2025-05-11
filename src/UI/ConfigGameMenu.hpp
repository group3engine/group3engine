#ifndef CONFIG_MENU_HPP
#define CONFIG_MENU_HPP

#include "BaseMenu.hpp"
#include "Engine.hpp"
#include "SampleGLTFFilePaths.hpp"

class UIManager;
class Context;

class ConfigGameMenu : public BaseMenu
{
  public:
    explicit ConfigGameMenu(Context &context, UIManager &uiManager);
    void Render(ImVec2 screenSize) override;
    void Resize() override;
    size_t NumPlayers;

    std::filesystem::path m_SelectedLevelPath = *Engine::GetScenePaths()[0]; // Default path is test scene

    inline static bool enableSceneSelection = true;
};

#endif // CONFIG_MENU_HPP

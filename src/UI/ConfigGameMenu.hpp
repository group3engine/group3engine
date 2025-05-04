#include "BaseMenu.hpp"

class UIManager;
class Context;

class ConfigGameMenu : public BaseMenu
{
  public:
    explicit ConfigGameMenu(Context &context, UIManager &uiManager);
    void Render(ImVec2 screenSize) override;
    void Resize() override;

    size_t NumPlayers;


};
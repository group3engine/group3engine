#ifndef MAIN_MENU_HPP
#define MAIN_MENU_HPP

#include <iostream>

#include "BaseMenu.hpp"

class UIManager;
class Context;

class NewGameMenu;

class MainMenuScreen : public BaseMenu
{
  public:

    MainMenuScreen(Context &context, UIManager& uiManager);
    void Render(ImVec2 screenSize) override;
    void Resize() override;
};

#endif // MAIN_MENU_HPP

#pragma once
#include "BaseMenu.hpp"

class UIManager;
class Context;

class NewGameMenu : public BaseMenu {

public:
    explicit NewGameMenu(Context& context, UIManager& uiManager);
    void Render(ImVec2 screenSize) override;
    void Resize() override;
};

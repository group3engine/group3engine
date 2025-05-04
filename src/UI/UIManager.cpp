#include "UIManager.hpp"
#include "BaseMenu.hpp"

void UIManager::RegisterMenu(const std::string &menuID, BaseMenu* menu) {
    menus[menuID] = menu;
}

void UIManager::SwitchToMenu(const std::string &menuID) {
    currentMenu = menus[menuID];
}

void UIManager::RenderCurrentMenu(ImVec2 screenSize) {
    if (currentMenu) {
        currentMenu->Render(screenSize);
    }
}

void UIManager::Resize() const {
    for (auto &[key, menu] : menus) {
        menu->Resize();
    }
}
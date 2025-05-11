#ifndef UI_MANAGER_HPP
#define UI_MANAGER_HPP

#include <imgui.h>
#include <unordered_map>
#include <string>
#include <memory>

class BaseMenu;

class UIManager {
  public:
    void RegisterMenu(const std::string &menuID, BaseMenu* menu);
    void SwitchToMenu(const std::string &menuID);
    void RenderCurrentMenu(ImVec2 screenSize);
    void Resize() const;
  private:
    std::unordered_map<std::string, BaseMenu*> menus;
    BaseMenu *currentMenu = nullptr;
};

#endif // UI_MANAGER_HPP

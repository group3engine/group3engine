#ifndef BASE_MENU_HPP
#define BASE_MENU_HPP

#include "Image.hpp"
#include <Utils.hpp>
#include <functional>
#include <imgui.h>
#include <imgui_impl_vulkan.h>
#include <string>

#include "Context.hpp"
#include "Fonts.hpp"

/*

    Derive menu's from this base class and implement the Render() method and
   Resize() methods. This class stores buttons, background image, and logo
   image. This can be set per menu so that each menu has a unique background and
   a logo though you'll likely have one logo Resize for all menus is called
   inside the Renderer class by the UIManager.

    All menu objects are being instantiated and registered with the UIManager in
   Engine.cpp.

    For now it has just buttons which you can keep adding since this is usually
   what you would find on menus but you can add more if it's needed.

*/

class UIManager;

class BaseMenu {
  public:
    struct ButtonEntry {
        std::string label;
        std::function<void()> onClick;
    };

    BaseMenu() = delete;
    explicit BaseMenu(Context &context, UIManager &uiManager);
    virtual ~BaseMenu();
    virtual void Render(ImVec2 ScreenSize) = 0;
    virtual void Resize() = 0;

    void SetLogo(const std::filesystem::path &logoPath);
    void SetBackground(const std::filesystem::path &backgroundPath,
                       ImVec2 backgroundSize);

  protected:
    Context &context;
    UIManager &uiManager;

    // Logo
    ImTextureID m_LogoTextureID;
    ImVec2 m_LogoPosition{0.0f, 0.0f};
    ImVec2 m_LogoSize{200, 200};

    // Background
    ImTextureID m_BackgroudTextureID;
    ImVec2 m_BackgroundSize{0.0f, 0.0f};

    std::vector<ButtonEntry> buttons;

    void DrawBackground(ImVec2 screenSize);
    void DrawButtons(ImVec2 screenSize);
    bool DrawStyledButton(const char *label, ImVec2 size);

  private:
    Image m_LogoImage;
    Image m_BackgroundImage;
};

#endif // BASE_MENU_HPP
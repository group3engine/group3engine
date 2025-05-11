#include "NewGameMenu.hpp"
#include "UIManager.hpp"
#include "Context.hpp"

NewGameMenu::NewGameMenu(Context &context, UIManager &uiManager) : BaseMenu(context, uiManager) {

    buttons.push_back({
        "New Adventure", [&uiManager]() {
            uiManager.SwitchToMenu("ConfigGameMenu");
        }
    });

    buttons.push_back({
        "Load Save", []() {
            std::cout << "Load save selected.\n";
        }
    });

    buttons.push_back({
        "Main Menu", [this, &context, &uiManager]() {
            uiManager.SwitchToMenu("MainMenu");
        }
    });

    SetBackground(assetsPath / "MainMenu/bg2.jpg", ImGui::GetIO().DisplaySize);
    SetLogo(assetsPath / "MainMenu/LOGO.png");

}

void NewGameMenu::Render(ImVec2 screenSize) {

    DrawBackground(screenSize);
    ImGui::SetNextWindowSize(screenSize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("New Game Menu", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBackground);

    DrawButtons(screenSize);


    ImGui::End();
}

void NewGameMenu::Resize()
{
    ImGui::GetIO().DisplaySize = ImVec2(context.extent.width, context.extent.height);
    m_LogoPosition = {context.extent.width / 2.0f - m_LogoSize.x / 2.0f, 50.0f};
    SetLogo(assetsPath / "MainMenu/LOGO.png");
    SetBackground(assetsPath / "MainMenu/bg2.jpg", ImGui::GetIO().DisplaySize);
}

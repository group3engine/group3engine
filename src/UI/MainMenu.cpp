
#include "MainMenu.hpp"
#include "NewGameMenu.hpp"
#include "Context.hpp"
#include "UIManager.hpp"
#include "ImGuiRenderer.hpp"
#include "Engine.hpp"

MainMenuScreen::MainMenuScreen(Context &context, UIManager& uiManager) : BaseMenu(context, uiManager) {

    ImGui::GetIO().DisplaySize = ImVec2(context.extent.width, context.extent.height);
    buttons.push_back({"New Game", [this, &context, &uiManager]() {
        uiManager.SwitchToMenu("NewGameMenu");
    }});

    buttons.push_back({"Settings", []() {

        std::cout << "Settings\n";

    }});

    buttons.push_back({"Exit", []() {
        Engine::get().Quit();
    }});

    SetLogo(assetsPath / "MainMenu/LOGO.png");
    SetBackground(assetsPath / "MainMenu/bg.jpg", ImGui::GetIO().DisplaySize);
}

void MainMenuScreen::Render(ImVec2 screenSize) {

    DrawBackground(screenSize);

    ImGui::SetNextWindowSize(screenSize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));

    ImGui::Begin("SOME BUTTONS", nullptr,
                        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoResize |
                        ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoBackground);

    DrawButtons(screenSize);


    ImGui::End();
}

void MainMenuScreen::Resize()
{
    ImGui::GetIO().DisplaySize = ImVec2(context.extent.width, context.extent.height);
    m_LogoPosition = {context.extent.width / 2.0f - m_LogoSize.x / 2.0f, 50.0f};
    SetLogo(assetsPath / "MainMenu/LOGO.png");
    SetBackground(assetsPath / "MainMenu/bg.jpg", ImGui::GetIO().DisplaySize);
}

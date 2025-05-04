#include "ConfigGameMenu.hpp"
#include "UIManager.hpp"
#include "Context.hpp"
#include "Engine.hpp"
#include <algorithm>

ConfigGameMenu::ConfigGameMenu(Context &context, UIManager &uiManager) : BaseMenu(context, uiManager) {

    NumPlayers = 1;

    /* Max player count is 4 ? */
    buttons.push_back({
        "+", [this]() {
            if (NumPlayers < 4)
                NumPlayers++;
        }
    });

    buttons.push_back({
        "-", [this]() {
            if (NumPlayers > 1)
                NumPlayers--;
        }
    });

    buttons.push_back({
        "Play", [this]() {
            SPDLOG_INFO("Load Scene");
            size_t playerCount = this->NumPlayers;
            Engine::get().ChangeScene(Sample::SampleObbyTestScene, playerCount);
        }
    });


    buttons.push_back({
        "Back", [this, &context, &uiManager]() {
            uiManager.SwitchToMenu("NewGameMenu");
        }
    });

    SetBackground(assetsPath / "MainMenu/bg3.jpg", ImGui::GetIO().DisplaySize);
    SetLogo(assetsPath / "MainMenu/LOGO.png");

}

void ConfigGameMenu::Render(ImVec2 screenSize)
{
    DrawBackground(screenSize);
    ImGui::SetNextWindowSize(screenSize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::Begin("New Game Menu", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                     ImGuiWindowFlags_NoBackground);

    DrawButtons(screenSize);

    // Push the font and style for the player count controls
    ImGui::PushFont(Fonts::GameFont);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.38f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

    const std::string text = "Number of Players: " + std::to_string(NumPlayers);
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    float textPosX = (screenSize.x - textSize.x) * 0.5f;
    ImGui::SetCursorPosX(textPosX);
    ImGui::Text("%s", text.c_str());

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    ImGui::PopFont();

    ImGui::End();
}

void ConfigGameMenu::Resize()
{
    ImGui::GetIO().DisplaySize = ImVec2(context.extent.width, context.extent.height);
    m_LogoPosition = {context.extent.width / 2.0f - m_LogoSize.x / 2.0f, 50.0f};
    SetLogo(assetsPath / "MainMenu/LOGO.png");
    SetBackground(assetsPath / "MainMenu/bg3.jpg", ImGui::GetIO().DisplaySize);
}

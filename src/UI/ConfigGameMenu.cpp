#include "ConfigGameMenu.hpp"
#include "UIManager.hpp"
#include "Context.hpp"
#include <algorithm>

ConfigGameMenu::ConfigGameMenu(Context &context, UIManager &uiManager) : BaseMenu(context, uiManager) {

    NumPlayers = 1;

    // // Max number of players is 4.
    // buttons.push_back({
    //     "+", [this]() {
    //         if (NumPlayers < 4)
    //             NumPlayers++;
    //     }
    // });

    // // Min number of players is 1.
    // buttons.push_back({
    //     "-", [this]() {
    //         if (NumPlayers > 1)
    //             NumPlayers--;
    //     }
    // });

    buttons.push_back({
        "PLAY", [this]() {
            SPDLOG_INFO("Loading scene");
            size_t playerCount = this->NumPlayers;
            Engine::get().ChangeScene(m_SelectedLevelPath, playerCount);
        }
    });

    // buttons.push_back({
    //     "Back", [this, &context, &uiManager]() {
    //         uiManager.SwitchToMenu("NewGameMenu");
    //     }
    // });
    buttons.push_back({"EXIT", []() {
        Engine::get().Quit();
    }});

    SetBackground(assetsPath / "MainMenu/title_shot_cropped.png", ImGui::GetIO().DisplaySize);
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
    ImGui::PushFont(Fonts::InGameFont);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.1f, 0.1f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.35f, 0.38f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);

    // const std::string text = "Number of Players: " + std::to_string(NumPlayers);
    // ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    // float textPosX = (screenSize.x - textSize.x) * 0.5f;
    // ImGui::SetCursorPosX(textPosX);
    // ImGui::Text("%s", text.c_str());

    ImGui::PushFont(Fonts::TitleFont);
    const std::string title = "WIPEOUT";
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImVec2 textPos = {screenSize.x * 0.5f - textSize.x * 0.5f,
                      screenSize.y * 0.25f - textSize.y * 0.5f};
    ImGui::SetCursorPos(textPos);
    ImGui::Text("%s", title.c_str());
    ImGui::PopFont();

    float xPos = ImGui::GetIO().DisplaySize.x - 300 - 10.0f;
    float yPos = 20.0f;
    ImGui::SetCursorPos(ImVec2(xPos, yPos));
    for (const auto *path : Engine::GetScenePaths())
    {
        bool isSelected = (*path == m_SelectedLevelPath);

        if (isSelected)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(.1f, 0.3f, 0.7f, 1.0f));

        ImGui::SetCursorPosX(xPos);
        if (ImGui::Button(path->filename().string().c_str(), ImVec2(300, 50)))
        {
            m_SelectedLevelPath = *path;
        }

        if (isSelected)
            ImGui::PopStyleColor();
    }

    ImGui::PopStyleVar(1);
    ImGui::PopStyleColor(4);
    ImGui::PopFont();



    ImGui::End();
}

void ConfigGameMenu::Resize()
{
    ImGui::GetIO().DisplaySize = ImVec2(context.extent.width, context.extent.height);
    m_LogoPosition = {context.extent.width / 2.0f - m_LogoSize.x / 2.0f, 50.0f};
    SetLogo(assetsPath / "MainMenu/LOGO.png");
    SetBackground(assetsPath / "MainMenu/title_shot_cropped.png", ImGui::GetIO().DisplaySize);
}

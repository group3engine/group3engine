//
// Created by thomas on 09/04/25.
//

#include "Fonts.hpp"

namespace Fonts
{
    bool LoadFonts()
    {
        ImGuiIO &io = ImGui::GetIO();

        // Load heading font
        HeadingFont = io.Fonts->AddFontFromFileTTF("assets/fonts/junegull/junegull.ttf", 32.0f);
        if (!HeadingFont) {
            std::cerr << "Failed to load heading font" << std::endl;
            return false;
        }

// Load subheading font
        SubHeadingFont = io.Fonts->AddFontFromFileTTF("assets/fonts/ubuntu-title/Ubuntu-Title.ttf", 24.0f);
        if (!SubHeadingFont) {
            std::cerr << "Failed to load subheading font" << std::endl;
            return false;
        }

// Load text font
        TextFont = io.Fonts->AddFontFromFileTTF("assets/fonts/ubuntu-title/Ubuntu-Title.ttf", 18.0f);
        if (!TextFont) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the subtle font
        TextFontSubtle = io.Fonts->AddFontFromFileTTF("assets/fonts/ubuntu-title/Ubuntu-Title.ttf", 14.0f);

        if (!TextFontSubtle) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the small font
        TextFontSmall = io.Fonts->AddFontFromFileTTF("assets/fonts/ubuntu-title/Ubuntu-Title.ttf", 10.0f);
        if (!TextFontSmall) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        return true;
    }
}
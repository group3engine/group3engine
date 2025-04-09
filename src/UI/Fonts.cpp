//
// Created by thomas on 09/04/25.
//

#include "Fonts.hpp"
#include "Utils.hpp"
#define FONT_PATH assetsPath / "fonts/"
namespace Fonts
{
    bool LoadFonts()
    {
        ImGuiIO &io = ImGui::GetIO();
        std::filesystem::path fontPath1 = FONT_PATH / "ubuntu-title/Ubuntu-Title.ttf";
        std::filesystem::path fontPath2 = FONT_PATH / "junegull/junegull.ttf";
        
        // Load heading font
        HeadingFont = io.Fonts->AddFontFromFileTTF(fontPath2.c_str(), 32.0f);
        if (!HeadingFont) {
            std::cerr << "Failed to load heading font" << std::endl;
            return false;
        }

// Load subheading font
        SubHeadingFont = io.Fonts->AddFontFromFileTTF(fontPath1.c_str(), 24.0f);
        if (!SubHeadingFont) {
            std::cerr << "Failed to load subheading font" << std::endl;
            return false;
        }

// Load text font
        TextFont = io.Fonts->AddFontFromFileTTF(fontPath1.c_str(), 18.0f);
        if (!TextFont) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the subtle font
        TextFontSubtle = io.Fonts->AddFontFromFileTTF(fontPath1.c_str(), 14.0f);

        if (!TextFontSubtle) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the small font
        TextFontSmall = io.Fonts->AddFontFromFileTTF(fontPath1.c_str(), 10.0f);
        if (!TextFontSmall) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        return true;
    }
}
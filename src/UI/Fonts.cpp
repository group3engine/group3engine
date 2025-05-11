//
// Created by thomas on 09/04/25.
//

#include "Fonts.hpp"

#include <spdlog/spdlog.h>

#include "Utils.hpp"
#define FONT_PATH assetsPath / "fonts/"
namespace Fonts
{
    bool LoadFonts()
    {
        ImGuiIO &io = ImGui::GetIO();
        std::filesystem::path fontPath1 = FONT_PATH / "ubuntu-title/Ubuntu-Title.ttf";
        std::filesystem::path fontPath2 = FONT_PATH / "junegull/junegull.ttf";
        std::filesystem::path fontPath3 = FONT_PATH / "segment16c/Segment16C Bold.ttf";
        std::filesystem::path gameFont =  FONT_PATH / "GameFont/GFont.ttf";
        std::filesystem::path alteHaasBoldPath = FONT_PATH / "alte_haas_grotesk/AlteHaasGroteskBold.ttf";
        std::filesystem::path alteHaasRegularPath = FONT_PATH / "alte_haas_grotesk/AlteHaasGroteskRegular.ttf";
        std::filesystem::path roaringJunglePath = FONT_PATH / "roaring_jungle/Roaring Jungle.otf";

        TitleFont = io.Fonts->AddFontFromFileTTF(roaringJunglePath.string().c_str(), 128.0f);
        if (!TitleFont) {
            SPDLOG_ERROR("Failed to load title font");
            return false;
        }

        // Load heading font
        HeadingFont = io.Fonts->AddFontFromFileTTF(alteHaasBoldPath.string().c_str(), 32.0f);
        if (!HeadingFont) {
            std::cerr << "Failed to load heading font" << std::endl;
            return false;
        }

// Load subheading font
        SubHeadingFont = io.Fonts->AddFontFromFileTTF(alteHaasBoldPath.string().c_str(), 24.0f);
        if (!SubHeadingFont) {
            std::cerr << "Failed to load subheading font" << std::endl;
            return false;
        }

// Load text font
        TextFont = io.Fonts->AddFontFromFileTTF(alteHaasRegularPath.string().c_str(), 18.0f);
        if (!TextFont) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the subtle font
        TextFontSubtle = io.Fonts->AddFontFromFileTTF(alteHaasRegularPath.string().c_str(), 14.0f);

        if (!TextFontSubtle) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the small font
        TextFontSmall = io.Fonts->AddFontFromFileTTF(alteHaasRegularPath.string().c_str(), 10.0f);
        if (!TextFontSmall) {
            std::cerr << "Failed to load text font" << std::endl;
            return false;
        }
        // load the loading font
        LoadingFont = io.Fonts->AddFontFromFileTTF(alteHaasRegularPath.string().c_str(), 32.0f);
        if (!LoadingFont) {
            std::cerr << "Failed to load loading font" << std::endl;
            return false;
        }
        // load the loading font small
        LoadingFontSmall = io.Fonts->AddFontFromFileTTF(alteHaasRegularPath.string().c_str(), 16.0f);
        if (!LoadingFontSmall) {
            std::cerr << "Failed to load loading font" << std::endl;
            return false;
        }

        GameFont = io.Fonts->AddFontFromFileTTF(alteHaasBoldPath.string().c_str(), 32.0f);

        if (!GameFont)
        {
            std::cerr << "Failed to load loading font" + alteHaasBoldPath.string() << std::endl;
            return false;
        }

        {
            std::string filename = alteHaasBoldPath.string();
            InGameFont = io.Fonts->AddFontFromFileTTF(filename.c_str(), 32.0f);
            if (!InGameFont) {
                SPDLOG_ERROR("Failed to load {}", filename.c_str());
            }
        }

        return true;
    }
}
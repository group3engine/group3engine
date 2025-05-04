#include "BaseMenu.hpp"
#include "UIManager.hpp"
#include "Context.hpp"
#include "Image.hpp"
#include "Fonts.hpp"
#include <imgui_impl_vulkan.h>
#include <Utils.hpp>

BaseMenu::BaseMenu(Context& context, UIManager& uiManager)
    : context(context), uiManager(uiManager)
{
    m_LogoPosition = { context.extent.width / 2.0f - m_LogoSize.x / 2.0f, 50.0f };
}

BaseMenu::~BaseMenu() {

    if (m_LogoTextureID) {
        m_LogoImage->Destroy(context.device);
    }
    if (m_BackgroudTextureID) {
        m_BackgroundImage->Destroy(context.device);
    }

    m_LogoTextureID = nullptr;
    m_BackgroudTextureID = nullptr;
}

void BaseMenu::SetLogo(const std::filesystem::path& logoPath) {
    m_LogoImage = std::make_unique<Image>(LoadTextureFromDisk(logoPath, context, VK_FORMAT_R8G8B8A8_SRGB));
    m_LogoTextureID = ImGui_ImplVulkan_AddTexture(vkutil::clampToEdgeSamplerAniso, m_LogoImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void BaseMenu::SetBackground(const std::filesystem::path& backgroundPath, ImVec2 backgroundSize) {
    m_BackgroundImage = std::make_unique<Image>(LoadTextureFromDisk(backgroundPath, context, VK_FORMAT_R8G8B8A8_UNORM));
    m_BackgroundSize = backgroundSize;
    m_BackgroudTextureID = ImGui_ImplVulkan_AddTexture(vkutil::clampToEdgeSamplerAniso, m_BackgroundImage->imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
}

void BaseMenu::DrawBackground(ImVec2 screenSize) {

    if (m_BackgroudTextureID) {
        ImGui::GetBackgroundDrawList()->AddImage(
            m_BackgroudTextureID,
            ImVec2(0, 0),
            ImVec2(m_BackgroundSize.x, m_BackgroundSize.y)
        );
    }

    if (m_LogoTextureID) {
        ImGui::GetBackgroundDrawList()->AddImage(
            m_LogoTextureID,
            m_LogoPosition,
            ImVec2(m_LogoPosition.x + m_LogoSize.x, m_LogoPosition.y + m_LogoSize.y)
        );
    }
}

void BaseMenu::DrawButtons(ImVec2 screenSize) {

    ImVec2 buttonSize(300, 50);
    float spacing = 20.0f;
    float totalHeight = (buttonSize.y + spacing) * buttons.size() - spacing;
    float startY = screenSize.y / 2 - totalHeight / 2;

    ImGui::SetCursorPosY(startY);

    for (auto& btn : buttons) {
        ImGui::SetCursorPosX(100.0f); // left aligned with offset
        if (DrawStyledButton(btn.label.c_str(), buttonSize)) {
            if (btn.onClick) btn.onClick();
        }
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + spacing);
    }
}

bool BaseMenu::DrawStyledButton(const char* label, ImVec2 size) {
    ImGui::PushFont(Fonts::GameFont);

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.1f, 0.1f, 0.1f, 0.1f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.35f, 0.38f, 0.45f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text,          ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 10));

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    ImGui::PopFont();

    return clicked;
}

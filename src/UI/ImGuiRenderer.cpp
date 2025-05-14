#include "Context.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "RenderPass.hpp"
#include "RenderPassCommon.hpp"
#include "ImGuiRenderer.hpp"
#include "Utils.hpp"
#include "Themes.hpp"

// Define math operators for the ImGui vector types
// You're meant to use your own math vector types but I don't think we'll need them
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <spdlog/fmt/chrono.h>
#include <spdlog/fmt/fmt.h>

#include "TextureManager.hpp"
#include "spdlog/spdlog.h"

#include <tracy/Tracy.hpp>

#include "Config.hpp"
#include "Engine.hpp"
#include "Fonts.hpp"

#include "SampleJoltCharacter.h"
#include "Camera.hpp"
#include "CharacterBaseTest.h"
#include "CharacterEntity.hpp"
#include "Sinking.hpp"
#include "ZipLine.hpp"

namespace {
    auto PushBackStyleVar = [](size_t i, std::function<void()> f) {
        f();
        return ++i;
    };

    bool enableTextWindowBorder = false;
    bool enableDeathPopup = true;
    bool enableFinishPopup = true;

    ImVec2 WindowSize(const Context &context) {
        return {static_cast<float>(context.extent.width),
                static_cast<float>(context.extent.height)};
    }

    ImGuiViewport CalcPlayerViewport(VkExtent2D extent, size_t activePlayerCount, size_t playerId) {
        ImGuiViewport viewport = {};

        VkViewport vkViewport = CalcViewport(extent, activePlayerCount, playerId);
        viewport.WorkPos = {vkViewport.x, vkViewport.y};
        viewport.WorkSize = {vkViewport.width, vkViewport.height};
        viewport.Pos = {vkViewport.x, vkViewport.y};
        viewport.Size = {vkViewport.width, vkViewport.height};

        return viewport;
    }

    float playerUiPadding = 50.0f;
    float imageTextPadding = 25.0f;
}

void ImGuiRenderer::Initialize(const Context &context) {
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = static_cast<uint64_t>(ImGuiPoolSizes.size());
    pool_info.pPoolSizes = ImGuiPoolSizes.data();

    // TODO: @DEBUG: This should use the engines error logger to make the user aware if this fails
    VK_CHECK(vkCreateDescriptorPool(context.device, &pool_info, nullptr, &ImGuiRenderer::imGuiDescriptorPool), "Failed to create ImGui descriptor pool.");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO io = ImGui::GetIO();
    (void)io;

    ImGui_ImplGlfw_InitForVulkan(context.mWindow, true);

    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance = context.instance;
    info.PhysicalDevice = context.pDevice;
    info.Device = context.device;
    info.Queue = context.graphicsQueue;
    info.QueueFamily = context.graphicsFamilyIndex;
    info.DescriptorPool = ImGuiRenderer::imGuiDescriptorPool;
    info.MinImageCount = 3;
    info.ImageCount = vkutil::MAX_FRAMES_IN_FLIGHT;
    info.Subpass = 0;
    info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    info.RenderPass = context.renderPass;

    ImGui_ImplVulkan_Init(&info);

    io.Fonts->AddFontDefault();
}

void ImGuiRenderer::AddTextures(TextureManager *textureManager, const std::filesystem::path &path, std::string textureName) {
    // TODO: Cleanup
    {

        // TODO: Return a pointer to the newly created texture?
        textureManager->addTexture(path, textureName);

        Texture *texture = textureManager->GetTexture(textureName);

        // Create Descriptor Set using ImGUI's implementation
        MyTextureData &texData = textureDatas[textureName];
        texData.DS =
            ImGui_ImplVulkan_AddTexture(vkutil::clampToEdgeSamplerAniso, texture->image.imageView,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        texData.Width = texture->image.mWidth;
        texData.Height = texture->image.mHeight;
        // move tex data to th
        SPDLOG_INFO("ImGui loaded {} with, width {} height {}", textureName, texture->image.mWidth, texture->image.mHeight);
    }
}

void ImGuiRenderer::RemoveTextures() {
    // When a scene is unloaded and the TextureManager is cleared, remove all
    // UI textures so we can then immediately load them and add them again.

    // This will be fine if we assume loading a loading screen texture (circle
    // of dots maybe) is instant.

    // TODO: We could have a separate part of texture manager that deals with
    // UI textures to get around this problem. Only clearing non-UI textures.

    // TODO: Slight asymmetry here with how AddTextures uses the TextureManager
    // but this function does not. A better implementation would use the
    // TextureManager and selectively remove textures from it.

    // Free the descriptor set associated with the texture
    // for each texture data in textureDatas
    for (auto& [key, myTexData] : textureDatas)
    {
        ImGui_ImplVulkan_RemoveTexture(myTexData.DS);
    }
    textureDatas.clear();
}

void ImGuiRenderer::BeginMainMenu(const Context &context) {

    // Style var counter for main menu window
    size_t mainMenuWindowSv = 0;
    // No window border
    mainMenuWindowSv = PushBackStyleVar(mainMenuWindowSv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    // Make the window fit the screen exactly
    mainMenuWindowSv = PushBackStyleVar(mainMenuWindowSv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    mainMenuWindowSv = PushBackStyleVar(mainMenuWindowSv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    mainMenuWindowSv = PushBackStyleVar(mainMenuWindowSv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });


    ImVec2 windowSize = WindowSize(context);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowBgAlpha(0.5f);

    // Flags to get a blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings;

    ImGui::Begin(std::string("Main Menu").c_str(), nullptr, flags);

    ImGui::PopStyleVar(mainMenuWindowSv);
    ImGui::PushFont(Fonts::GameFont);

    std::string mainMenuStr = "Main Menu";
    ImVec2 mainMenuStrSize = ImGui::CalcTextSize(mainMenuStr.c_str());
    ImVec2 mainMenuStrOffset = windowSize - mainMenuStrSize;
    ImGui::SetCursorScreenPos(ImVec2(mainMenuStrOffset.x / 2.0f , mainMenuStrOffset.y / 3.0f));
    ImGui::Text("Main Menu");
    ImGui::PopFont();
}

const char* ImGuiRenderer::AddMainMenuPlayerCountSelection(const Context &context,
                                               const std::vector<const char *> &playerCounts,
                                               const char *playerCountSelection) {
    ImVec2 windowSize = WindowSize(context);

    const char *activeItem = playerCountSelection;

    float sceneSelectionDropdownWidth = windowSize.x * 0.25f;
    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);

    ImGui::PushFont(Fonts::SubHeadingFont);
    ImGui::Text("Select Number of Players");
    ImGui::PopFont();

    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);
    ImGui::PushItemWidth(sceneSelectionDropdownWidth);

    ImGui::PushFont(Fonts::TextFont);

    if (ImGui::BeginCombo("##Main Menu Player Count Selection", playerCountSelection)) {
        for (size_t i = 0; i < playerCounts.size(); ++i) {
            bool isSelected = playerCountSelection == playerCounts[i];

            if (ImGui::Selectable(playerCounts[i], isSelected)) {
                activeItem = playerCounts[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
    ImGui::PopFont();

    return activeItem;
}

const std::filesystem::path *
ImGuiRenderer::AddMainMenuSceneSelection(const Context &context,
                                         const std::vector<std::filesystem::path *> &scenePaths,
                                         const std::filesystem::path *scenePathSelection) {
    ImVec2 windowSize = WindowSize(context);

    const std::filesystem::path *activeItem = scenePathSelection;

    float sceneSelectionDropdownWidth = windowSize.x * 0.25f;
    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);

    ImGui::PushFont(Fonts::SubHeadingFont);
    ImGui::Text("Select Level");
    ImGui::PopFont();

    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);
    ImGui::PushItemWidth(sceneSelectionDropdownWidth);

    ImGui::PushFont(Fonts::TextFont);

    if (ImGui::BeginCombo("##Main Menu Scene Selection", scenePathSelection->string().c_str())) {
        for (size_t i = 0; i < scenePaths.size(); ++i) {
            bool isSelected = scenePathSelection == scenePaths[i];

            if (ImGui::Selectable(scenePaths[i]->string().c_str(), isSelected)) {
                activeItem = scenePaths[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();
    ImGui::PopFont();

    return activeItem;
}

const char *ImGuiRenderer::NewPlayerCountSelection(const std::vector<const char *> &playerCounts,
                                                   const char *playerCountSelection) {
    const char *activeItem = playerCountSelection;
    ImGui::PushFont(Fonts::SubHeadingFont);
    ImGui::Text("Select Number of Players");
    ImGui::PopFont();

    ImGui::PushFont(Fonts::TextFont);
    if (ImGui::BeginCombo("##Player Count Selection", playerCountSelection)) {
        for (size_t i = 0; i < playerCounts.size(); ++i) {
            bool isSelected = playerCountSelection == playerCounts[i];

            if (ImGui::Selectable(playerCounts[i], isSelected)) {
                activeItem = playerCounts[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::PopFont();

    return activeItem;
}

const std::filesystem::path *
ImGuiRenderer::NewSceneSelection(const std::vector<std::filesystem::path *> &scenePaths,
                                 const std::filesystem::path *scenePathSelection) {
    const std::filesystem::path *activeItem = scenePathSelection;

    ImGui::PushFont(Fonts::SubHeadingFont);
    ImGui::Text("Select Level");
    ImGui::PopFont();

    ImGui::PushFont(Fonts::TextFont);
    if (ImGui::BeginCombo("##Scene Selection", scenePathSelection->string().c_str())) {
        for (size_t i = 0; i < scenePaths.size(); ++i) {
            bool isSelected = scenePathSelection == scenePaths[i];

            if (ImGui::Selectable(scenePaths[i]->string().c_str(), isSelected)) {
                activeItem = scenePaths[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
    ImGui::PopFont();

    return activeItem;
}

void ImGuiRenderer::AddLoadSceneButton(const std::filesystem::path &pendingScenePath,
                                       size_t pendingPlayerCount) {
    ImGui::PushFont(Fonts::TextFont);
    std::string loadSceneStr = "Load Scene";
    ImGui::SameLine();
    if (ImGui::Button(loadSceneStr.c_str())) {
        SPDLOG_INFO("Load Scene");
        Engine::get().ChangeScene(pendingScenePath, pendingPlayerCount);
    }
    ImGui::PopFont();
}

void ImGuiRenderer::AddQuitButton()
{
    ImGui::PushFont(Fonts::TextFont);
    std::string quitStr = "Quit";
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImGui::SetCursorPosX((windowSize.x - ImGui::CalcTextSize(quitStr.c_str()).x) * 0.5f);
    if (ImGui::Button(quitStr.c_str())) {
        Engine::get().Quit();
    }
    ImGui::PopFont();
}

void ImGuiRenderer::EndMainMenu() {
    ImGui::End();
}

// TODO: Broken for splitscreen
ImVec2 ImGuiRenderer::NewImage(const std::string &name, const ImVec2 &offset, const ImVec2 &imageSize) {
    ZoneScopedN("ImGuiRenderer::NewImage");

    MyTextureData &myTexData = textureDatas[name];

    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Make the window fit the image exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(imageSize);
    ImGui::SetNextWindowPos(ImVec2(offset.x - windowBorderSize, offset.y - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("GUI Texture {}", name).c_str(), nullptr, flags);
    ImGui::Image((ImTextureID)myTexData.DS, imageSize);

    ImGui::PopStyleVar(sv);

    ImGui::End();

    // Return the position of the top right corner of the image
    return ImVec2(offset.x - windowBorderSize + imageSize.x,
                  offset.y - windowBorderSize + imageSize.y);
}

void ImGuiRenderer::NewDeathCounter(const gui::DeathCounterData &data,
                                    size_t activePlayerCount, size_t playerId) {
    ZoneScopedN("ImGuiRenderer::NewDeathCounter");

    ImGui::PushFont(Fonts::InGameFont);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));

    MyTextureData &textureData = textureDatas["skull-white"];
    ImVec2 imageSize = ImVec2(textureData.Width, textureData.Height) * 0.1f;

    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    size_t deathCount = data.deathCount;

    // Format to a width of 4
    // See https://hackingcpp.com/cpp/libs/fmt.html
    std::string str = fmt::format("{:4}", deathCount);

    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Bottom right of viewport. NOTE: hardcoded bottom right positioning
    ImVec2 pos = ImVec2(viewport.WorkPos.x + viewport.WorkSize.x - playerUiPadding, viewport.WorkPos.y + viewport.WorkSize.y - playerUiPadding);
    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // Right horizontal aligned to image and center vertical aligned to image
    ImGui::SetNextWindowPos(ImVec2(pos.x - textSize.x - windowBorderSize,
                                   pos.y - imageSize.y / 2.0f - textSize.y / 2.0f - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Death Counter Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text(str.c_str());

    // Skull
    ImVec2 offset = {pos.x - textSize.x - imageSize.x - imageTextPadding, pos.y - imageSize.y};
    NewImage("skull-white", offset, imageSize);

    ImGui::PopStyleVar(sv);

    ImGui::End();

    ImGui::PopFont();
    ImGui::PopStyleColor();
}

void ImGuiRenderer::NewCoinCounter(const gui::CoinCounterData &data,
                                    size_t activePlayerCount, size_t playerId) {
    ZoneScopedN("ImGuiRenderer::NewDeathCounter");

    ImGui::PushFont(Fonts::InGameFont);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));

    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    size_t coinCount = data.coinCount;

    // Format to a width of 4
    // See https://hackingcpp.com/cpp/libs/fmt.html
    std::string str = fmt::format("{:4}", coinCount);

    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Bottom left of viewport
    ImVec2 pos = ImVec2(viewport.WorkPos.x + playerUiPadding, viewport.WorkPos.y + viewport.WorkSize.y - playerUiPadding);

    // Coin texture
    MyTextureData &textureData = textureDatas["coins-white"];
    ImVec2 imageSize = ImVec2(textureData.Width, textureData.Height) * 0.1f;
    ImVec2 offset = {pos.x, pos.y - imageSize.y};
    ImVec2 bottomRightImageOffset = NewImage("coins-white", offset, imageSize);

    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // Right horizontal aligned to image and center vertical aligned to image
    ImGui::SetNextWindowPos(ImVec2(bottomRightImageOffset.x - windowBorderSize + imageTextPadding,
                                   bottomRightImageOffset.y - imageSize.y / 2.0f - textSize.y / 2.0f - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Coin Counter Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text(str.c_str());

    ImGui::PopStyleVar(sv);
    ImGui::PopStyleColor();

    ImGui::End();

    ImGui::PopFont();
}

void ImGuiRenderer::NewTimer(const gui::TimerData &data,
                             size_t activePlayerCount, size_t playerId) {
    ZoneScopedN("ImGuiRenderer::NewTimer");

    ImGui::PushFont(Fonts::InGameFont);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,255,255));

    MyTextureData &textureData = textureDatas["hourglass-white"];
    ImVec2 imageSize = ImVec2(textureData.Width, textureData.Height) * 0.1f;

    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    float time = data.time;

    // Format to a width of 8 and to a precision of 3
    // See https://hackingcpp.com/cpp/libs/fmt.html
    std::string str = fmt::format("{:%M:%S}", fmt::localtime(time));
    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Top right of viewport. NOTE: hardcoded top right positioning
    ImVec2 pos = ImVec2(viewport.WorkPos.x + viewport.WorkSize.x - playerUiPadding, viewport.WorkPos.y + playerUiPadding);
    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // Right horizontal aligned to image and center vertical aligned to image
    ImGui::SetNextWindowPos(ImVec2(pos.x - textSize.x - windowBorderSize,
                                   pos.y + imageSize.y / 2.0f - textSize.y / 2.0f + windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Timer Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text(str.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::End();

    ImGui::PopFont();
    ImGui::PopStyleColor();

    ImVec2 imageOffset = {pos.x - textSize.x - imageSize.x - imageTextPadding, pos.y};
    NewImage("hourglass-white", imageOffset, imageSize);
}

void ImGuiRenderer::LoadingBar(float progress, ImVec2 position) {

    ImGui::PushFont(Fonts::GameFont);

    const ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImVec2 windowSize = ImVec2(viewport->WorkSize.x, 100.0f);
    ImVec2 windowPos =
        ImVec2(viewport->WorkPos.x,
               viewport->WorkPos.y + viewport->WorkSize.y - windowSize.y);

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs |
        ImGuiWindowFlags_NoBackground;

    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("Loading", nullptr, flags);

    float barWidth = 400.0f;
    float barHeight = 35.0f;
    ImVec2 barSize = ImVec2(barWidth, barHeight);

    ImVec2 barPos = ImVec2((viewport->WorkSize.x - barWidth) * 0.5f, windowSize.y * 0.5f - barHeight * 0.5f);
    ImGui::SetCursorPos(barPos);

    // Style
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(216, 169, 93, 255));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30, 30, 30, 200));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));

    ImGui::ProgressBar(progress / 100.f, barSize);

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
    ImGui::End();

    ImGui::PopFont();
}


void ImGuiRenderer::Image(std::string const &imageName, ImVec2 position, ImVec2 size)
{
    MyTextureData &myTexData = textureDatas[imageName];
    // flip position 0-1 to 1-0
    position = ImVec2{1.f - position.x, 1.f - position.y};
    size_t sv = 0;
    // convert the position and size from relative (0-1) coordinates, to pixel coordinates
    // get the window size
    // Get the main viewport to determine screen dimensions
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Calculate position at the bottom of the screen (ignoring the passed position parameter)
    ImVec2 windowSize = ImVec2(viewport->Size.x, viewport->Size.y);
    position = ImVec2(position.x * windowSize.x, position.y * windowSize.y);
    size = ImVec2(size.x * windowSize.x, size.y * windowSize.y);


    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Make the window fit exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(size);
    ImGui::SetNextWindowPos(ImVec2(position.x - windowBorderSize - size.x, position.y - windowBorderSize - size.y));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin((imageName + "image render").c_str(), nullptr, flags);
    ImGui::Image((ImTextureID)myTexData.DS, size);

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewActivePlayerCountOverride(
    Scene *scene, gui::Settings::ActivePlayerCountOverride &settings) {
#ifndef PLATINUM
    ImGui::Text("Active Player Count Override");
    if (ImGui::BeginCombo("##Active Player Count Override", settings.activeItem)) {
        for (size_t i = 0; i < settings.items.size(); ++i) {
            bool isSelected = settings.activeItem == settings.items[i];

            if (ImGui::Selectable(settings.items[i], isSelected)) {
                settings.activeItem = settings.items[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    if (settings.activeItem) {
        if (strcmp(settings.activeItem, "INACTIVE") == 0) {
            scene->SetActivePlayerCountOverrideInactive();
        } else {
            size_t playerCount = 0;
            [[maybe_unused]] int ret = sscanf(settings.activeItem, "%zu", &playerCount);
            assert(ret);
            scene->SetActivePlayerCountOverride(playerCount);
        }
    }
#endif
}

void ImGuiRenderer::NewCharacterInfo(std::string const &characterName,
                                    float x, float y, float z) {
#ifndef PLATINUM
    if (ImGui::CollapsingHeader(characterName.c_str())) {
        // Add camera position
        ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",x,y,z);
    }
#endif
}

void ImGuiRenderer::Text(std::string const &text, ImVec2 position, ImFont *font,
                         size_t activePlayerCount, size_t playerId) {
    ZoneScopedN("ImGuiRenderer::Text");

    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    // flip position 0-1 to 1-0
    position = ImVec2{1.f - position.x, 1.f - position.y};
    size_t sv = 0;

    // Make sure to push font before text size calculation
    ImGui::PushFont(font);

    // convert the position and size from relative (0-1) coordinates, to pixel coordinates
    // Calculate position at the bottom of the screen (ignoring the passed position parameter)
    ImVec2 windowSize = ImVec2(viewport.Size.x, viewport.Size.y);
    position = ImVec2(position.x * windowSize.x, position.y * windowSize.y);
    ImVec2 textSize = ImGui::CalcTextSize(text.c_str(), nullptr, true);
    // offset the position by half the text size
    position = ImVec2(position.x - textSize.x * 0.5f, position.y - textSize.y * 0.5f);


    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Make the window fit exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    ImGui::SetNextWindowPos(ImVec2(position.x - windowBorderSize, position.y - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    static uint64_t textId = 0;
    ImGui::Begin(fmt::format("text rendering##PlayerId{}TextId{}", playerId, textId).c_str(), nullptr, flags);
    textId++;

    ImGui::Text(text.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::PopFont();

    ImGui::End();
}

void ImGuiRenderer::NewFrame() {
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiRenderer::EndFrame() {
    ImGui::EndFrame();
}

void ImGuiRenderer::Update(Scene *scene)
{
    ZoneScopedN("ImGuiRenderer::Update");

    ImGui::BeginChild("Settings");
    // Display FPS
    ImGui::TextColored(
        ImVec4(0.76, 0.5, 0.0, 1.0), "FPS: (%.1f FPS), %.3f ms/frame",
        ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

    assert(LightManager::getInstance().GetLights().size() > 0);
   auto dir = glm::normalize(LightManager::getInstance().GetLights()[0]->position);

   ImGui::Text("Directional Light: (%.2f, %.2f, %.2f)", dir.x, dir.y, dir.z);

    static bool initialized = false;
    static float SunElevation = 0.0f;
    static float SunAzimuthal = 0.0f;
    static const float distance = 1.0f;

    auto lights = LightManager::getInstance().GetLights();
    if (lights.empty())
        return;

    auto *sunLight = lights[0];

    if (!initialized) {
        SunElevation = 0.89f; // default elevation // -21
        SunAzimuthal = 0.0f; // default azimuth // 45
        //sunLight.view = -43.0f;
        //sunLight.far = 50.0f;
        //sunLight.near = -125.0f;
        initialized = true;
    }

    if (ImGui::CollapsingHeader("Directional Light"))
    {
        ImGui::Text("Sun Angles");
        ImGui::SliderFloat("Elevation - Phi", &SunElevation, 0.0f, 1.5708f, "%.2f");
        ImGui::SliderFloat("Azimuthal - Theta", &SunAzimuthal, -3.141f, 3.141f, "%.2f");

        ImGui::Text("Light Camera Settings");
        ImGui::SliderFloat("View", &sunLight->view, -200.0f, 200.0f, "%.2f");
        ImGui::SliderFloat("Near", &sunLight->near_, -200.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Far", &sunLight->far_, 0.0f, 50.0f, "%.2f");

        ImGui::SliderFloat("Shadow bias: ", &vkutil::ShadowBias, 0.0f, 10.0f);
        ImGui::SliderFloat("Shadow slope: ", &vkutil::ShadowSlope, 0.0f, 10.0f);
    }

    if (ImGui::CollapsingHeader("Lights")) {
        auto lights = LightManager::getInstance().GetLights();
        for (size_t i = 1; i < lights.size() - 1; ++i) {
            if (lights[i]->Type != LightType::Directional) {
                std::string label = "Light " + std::to_string(i) + " Position";
                ImGui::SliderFloat3(label.c_str(), &lights[i]->position.x, -10.0f, 10.0f, "%.2f");
            }
        }
    }

    float sinkingStep = 0.1f;
    float sinkingStepFast = 0.5f;
    ImGui::InputFloat("mSinkingSpeed", &Sinking::mSinkingSpeed, sinkingStep, sinkingStepFast, nullptr, 0);

    {
        float step = 0.1f;
        float stepFast = 0.5f;
        ImGui::InputFloat("Camera::sZoomLevel", &Camera::sZoomLevel, step, stepFast, nullptr, 0);
        ImGui::InputFloat("ZipLine::sZiplineCameraZoomLevel", &ZipLine::sZiplineCameraZoomLevel, step, stepFast, nullptr, 0);
    }

    if (ImGui::CollapsingHeader("UI Adjustment")) {
        ImGui::InputFloat("playerUiPadding", &playerUiPadding, 1.0f, 10.0f, nullptr, 0);
        ImGui::InputFloat("imageTextPadding", &imageTextPadding, 1.0f, 10.0f, nullptr, 0);
    }

    if (ImGui::CollapsingHeader("CharacterSettings")) {
        {
            float step = 0.01f;
            float stepFast = 0.1f;
            ImGui::InputFloat("sCameraUpOffset: ", &Camera::sCameraUpOffset, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sCameraCrouchingUpOffset: ", &Camera::sCameraCrouchingUpOffset, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sCameraRightOffset: ", &Camera::sCameraRightOffset, step, stepFast, nullptr, 0);
        }

        {
            float step = 0.1f;
            float stepFine = 0.01f;
            float stepFast = 0.5f;
            ImGui::InputFloat("sCharacterSpeed: ", &CharacterBaseTest::sCharacterSpeed, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sJumpTime", &CharacterBaseTest::sJumpTime, stepFine, step, nullptr, 0);
            ImGui::InputFloat("sFallTime", &CharacterBaseTest::sFallTime, stepFine, step, nullptr, 0);
        }

        {
            float step = 0.01f;
            float stepFast = 0.1f;
            ImGui::InputFloat("sJumpHeight", &CharacterBaseTest::sJumpHeight, step, stepFast, nullptr, 0);
            CharacterBaseTest::sJumpSpeed = 2.0f * CharacterBaseTest::sJumpHeight / CharacterBaseTest::sJumpTime;
            CharacterBaseTest::sJumpGravity = -2.0f * CharacterBaseTest::sJumpHeight / Square(CharacterBaseTest::sJumpTime);
            CharacterBaseTest::sFallGravity = -2.0f * CharacterBaseTest::sJumpHeight / Square(CharacterBaseTest::sFallTime);
            ImGui::InputFloat("sJumpTimeScale", &CharacterEntity::sJumpTimeScale, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sFallTimeScale", &CharacterEntity::sFallTimeScale, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sFallBlend", &CharacterEntity::sFallBlend, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sHangingBlend", &CharacterEntity::sHangingBlend, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sClimbTimeScale", &CharacterEntity::sClimbTimeScale, step, stepFast, nullptr, 0);
            ImGui::InputFloat("sRunningCrouchTimeScale", &CharacterEntity::sRunningCrouchTimeScale, step, stepFast, nullptr, 0);
        }
    }

    static bool showGraphics = false;
    ImGui::Checkbox("Graphics Settings", &showGraphics);
    if (showGraphics) {

        ImGui::Begin("Graphics");
        ImGui::SetWindowSize(ImVec2(400, 600));
        if (ImGui::CollapsingHeader("SSAO"))
        {
            ImGui::SliderInt("Directions: ", &vkutil::ssaoSettings.NumDirections, 1, 64);
            ImGui::SliderInt("Steps: ", &vkutil::ssaoSettings.NumSteps, 1, 64);
            ImGui::SliderFloat("Radius: ", &vkutil::ssaoSettings.Radius, 0.1f, 10.0f);
            ImGui::SliderFloat("StepSize: ", &vkutil::ssaoSettings.StepSize, 0.0f, 0.1f);
            ImGui::SliderFloat("Intensity: ", &vkutil::ssaoSettings.intensity, 0.0f, 10.0f);
        }

        if (ImGui::CollapsingHeader("SSR"))
        {
            ImGui::SliderFloat("MaxDistance: ", &vkutil::ssrSettings.MaxDistance, 0.0f, 100.0f);
            ImGui::SliderFloat("Thickness: ", &vkutil::ssrSettings.thickness, 0, 1.0f);
        }

        if (ImGui::CollapsingHeader("Fog"))
        {
            float step = 0.001f;
            float stepFast = 0.01f;
            ImGui::InputFloat("Distance: ", &vkutil::fogSettings.MaxDistance, 0.1f, 1.0f, nullptr, 0);
            ImGui::InputFloat("Density: ", &vkutil::fogSettings.Density, step, stepFast, nullptr, 0);
            ImGui::InputFloat("SteppingSize: ", &vkutil::fogSettings.StepSize, 0.01f, 0.1f, nullptr, 0);
            ImGui::SliderInt("Steps: ", &vkutil::fogSettings.MaxSteps, 1, 10);
        }

        if (ImGui::CollapsingHeader("FXAA"))
        {
            ImGui::Checkbox("Enable FXAA: ", &vkutil::fxaaSettings.EnableFXAA);
        }

        if (ImGui::CollapsingHeader("Post Processing"))
        {
            ImGui::SliderFloat("Brightness: ", &vkutil::postProcessingSettings.brightness, 0.0f, 1.0f);
            ImGui::SliderFloat("Contrast: ", &vkutil::postProcessingSettings.contrast, 0.0f, 5.0f);
            ImGui::SliderFloat("Saturation: ", &vkutil::postProcessingSettings.saturation, 0.0f, 2.0f);

            const char *toneMapOptions[] = {"OFF", "Reinhard", "Uncharted2", "ACES"};
            ImGui::Combo("Tone Map", &vkutil::postProcessingSettings.toneMap, toneMapOptions, IM_ARRAYSIZE(toneMapOptions));
        }

        // NOTE: Add more to this if anyone wants to add more debug visuals
        // Just make sure you use the right index in the shader
        if (ImGui::CollapsingHeader("Renderer Debug"))
        {
            const char *types[12] = {"Final", "Normal", "World Position", "Albedo", "Roughness", "Metallic", "Shadows", "Mip visual", "Cascades", "SSAO", "SSR", "Wireframe"};
            ImGui::ListBox("Renderer Debug", &vkutil::rendererDebug.debugMode, types, 12);
        }

        ImGui::End();
    }



    static bool enableTextureDebug = false;
    ImGui::Checkbox("Debug Textures", &enableTextureDebug);
    if (enableTextureDebug)
    {
        ImGui::Begin("Debug Textures");
        for (size_t i = 0; i < textureIDs.size(); ++i) {
            ImGui::Image(textureIDs[i], ImVec2(800 / 2, 600 / 2));
        }
        ImGui::End();
    }

    ImGui::Checkbox("Enable Text Window Border", &enableTextWindowBorder);

    ImGui::Checkbox("Enable Death Popup", &enableDeathPopup);

#ifdef JPH_DEBUG_RENDERER
    ImGui::Checkbox("Enable Physics Debug Renderer", &GlobalConfig::enablePhysicsDebugRenderer);
#endif // JPH_DEBUG_RENDERER

    ImGui::EndChild();
}

void ImGuiRenderer::Render(VkCommandBuffer cmd, const Context &context, uint32_t imageIndex)
{
    ZoneScopedN("ImGuiRenderer::Render");

    ImGui::Render();
    ImDrawData *main_draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(main_draw_data, cmd);
}

void ImGuiRenderer::Shutdown(const Context& context)
{
    // for each texture, remove

    RemoveTextures();

    ImGui_ImplVulkan_Shutdown();
    vkDestroyDescriptorPool(context.device, imGuiDescriptorPool, nullptr);
    vkDestroyRenderPass(context.device, imGuiRenderPass, nullptr);

    ImGui_ImplGlfw_Shutdown();
}

void ImGuiRenderer::AddTexture(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
{
    ImTextureID textureID = ImGui_ImplVulkan_AddTexture(sampler, imageView, imageLayout);
    textureIDs.push_back(static_cast<void*>(textureID));
}


void ImGuiRenderer::ChatWindow(const std::vector<Message> &messages, std::function<void(std::string, std::string)> callback)
{
    ZoneScopedN("ImGuiRenderer::ChatWindow");

    // Fixed upper left position with a drop\-down style window
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
    ImGui::SetNextWindowSize(ImVec2(300, 400), ImGuiCond_Once);
    ImGui::SetNextWindowBgAlpha(0.25f);
    // set the font the subheading font
    ImGui::PushFont(Fonts::TextFont);
    if (ImGui::Begin("Chat Window", nullptr, ImGuiTreeNodeFlags_DefaultOpen))
    {
       static char playerName[64] = "";
        ImGui::Text("Player Name"); ImGui::SameLine();
        ImGui::InputText("##PlayerName", playerName, sizeof(playerName));
        float availableWidth = ImGui::GetContentRegionAvail().x;
        float availableHeight = ImGui::GetContentRegionAvail().y;
        float inputHeight1 = ImGui::GetFrameHeightWithSpacing();
        float inputHeight2 = ImGui::GetFrameHeightWithSpacing();
        float chatHeight = availableHeight - (inputHeight1 + inputHeight2) / 2.0f;
        if (chatHeight < 0.0f)
            chatHeight = 0.0f;
        ImGui::SetNextWindowBgAlpha(0.35f);
       // Hide scrollbar by default, show on hover
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 4.0f);  // Thin scrollbar
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(1,1,1,1));  // Transparent background
        ImGui::BeginChild("ChatMessages", ImVec2(availableWidth, chatHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
        for (const auto &msg : messages)
        {
            ImGui::Text(msg.playerName.c_str());
            ImGui::PushFont(Fonts::TextFontSubtle);
            float textSpace = ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(msg.timestamp.c_str()).x - 20.0f;
            ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + textSpace);
            ImGui::Text(msg.text.c_str());
            ImGui::PopTextWrapPos();
            ImGui::PopFont();
            ImGui::PushFont(Fonts::TextFontSmall);
            float tsWidth = ImGui::CalcTextSize(msg.timestamp.c_str()).x;
            ImGui::SameLine(ImGui::GetWindowWidth() - tsWidth - 10.0f);
            ImGui::Text(msg.timestamp.c_str());
            ImGui::PopFont();
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
        if(messages.size() > messageCount) {
            // Auto-scroll to bottom
            static bool autoScroll = true;
            if (autoScroll)
                ImGui::SetScrollHereY(1.0f);

            messageCount = messages.size();
        }
        ImGui::EndChild();


        // Input field and Send button
        static char inputBuffer[256] = "";
        ImGui::InputText("##ChatInput", inputBuffer, sizeof(inputBuffer));
        // Keep input field and button on the same line regardless of resizing
        ImGui::SameLine();
        if (ImGui::Button("Send"))
        {
            if (strlen(inputBuffer) > 0)
            {
                callback(std::string(playerName), std::string(inputBuffer));
                inputBuffer[0] = '\0';
            }
        }

    }
    ImGui::PopFont();


    ImGui::End();
}
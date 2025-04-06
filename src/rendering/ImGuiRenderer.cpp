#include "Context.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "CharacterEntity.hpp"
#include "RenderPass.hpp"
#include "RenderPassCommon.hpp"
#include "ImGuiRenderer.hpp"
#include "Utils.hpp"

// Define math operators for the ImGui vector types
// You're meant to use your own math vector types but I don't think we'll need them
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <spdlog/fmt/fmt.h>

#include "TextureManager.hpp"
#include "spdlog/spdlog.h"

#include <tracy/Tracy.hpp>

#include "Config.hpp"
#include "Engine.hpp"

namespace {
    auto PushBackStyleVar = [](size_t i, std::function<void()> f) {
        f();
        return ++i;
    };

    bool enableTextWindowBorder = true;
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

        return viewport;
    }
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

    std::string mainMenuStr = "Main Menu";
    ImVec2 mainMenuStrSize = ImGui::CalcTextSize(mainMenuStr.c_str());
    ImVec2 mainMenuStrOffset = windowSize - mainMenuStrSize;
    ImGui::SetCursorScreenPos(ImVec2(mainMenuStrOffset.x / 2.0f , mainMenuStrOffset.y / 3.0f));
    ImGui::Text("Main Menu");
}

const char *
ImGuiRenderer::AddMainMenuPlayerCountSelection(const Context &context,
                                               const std::vector<const char *> &playerCounts,
                                               const char *playerCountSelection) {
    ImVec2 windowSize = WindowSize(context);

    const char *activeItem = playerCountSelection;

    float sceneSelectionDropdownWidth = windowSize.x * 0.25f;
    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);

    ImGui::Text("Select Number of Players");

    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);
    ImGui::PushItemWidth(sceneSelectionDropdownWidth);

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

    ImGui::Text("Select Level");

    ImGui::SetCursorPosX((windowSize.x - sceneSelectionDropdownWidth) / 2.0f);
    ImGui::PushItemWidth(sceneSelectionDropdownWidth);

    if (ImGui::BeginCombo("##Main Menu Scene Selection", scenePathSelection->c_str())) {
        for (size_t i = 0; i < scenePaths.size(); ++i) {
            bool isSelected = scenePathSelection == scenePaths[i];

            if (ImGui::Selectable(scenePaths[i]->c_str(), isSelected)) {
                activeItem = scenePaths[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    return activeItem;
}

const char *ImGuiRenderer::NewPlayerCountSelection(const std::vector<const char *> &playerCounts,
                                                   const char *playerCountSelection) {
    const char *activeItem = playerCountSelection;

    ImGui::Text("Select Number of Players");

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

    return activeItem;
}

const std::filesystem::path *
ImGuiRenderer::NewSceneSelection(const std::vector<std::filesystem::path *> &scenePaths,
                                 const std::filesystem::path *scenePathSelection) {
    const std::filesystem::path *activeItem = scenePathSelection;

    ImGui::Text("Select Level");

    if (ImGui::BeginCombo("##Scene Selection", scenePathSelection->c_str())) {
        for (size_t i = 0; i < scenePaths.size(); ++i) {
            bool isSelected = scenePathSelection == scenePaths[i];

            if (ImGui::Selectable(scenePaths[i]->c_str(), isSelected)) {
                activeItem = scenePaths[i];
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return activeItem;
}

void ImGuiRenderer::AddLoadSceneButton(const std::filesystem::path &pendingScenePath,
                                       size_t pendingPlayerCount) {
    std::string loadSceneStr = "Load Scene";
    ImGui::SameLine();
    if (ImGui::Button(loadSceneStr.c_str())) {
        SPDLOG_INFO("Load Scene");
        Engine::get().ChangeScene(pendingScenePath, pendingPlayerCount);
    }
}

void ImGuiRenderer::EndMainMenu() {
    ImGui::End();
}

void ImGuiRenderer::NewHeartSprite(const ImVec2 &offset, size_t playerId) {
    // TODO: Remove hardcoded image size
    MyTextureData &myTexData = textureDatas["heart"];
    ImVec2 imageSize = ImVec2(myTexData.Width * 0.02f, myTexData.Height * 0.02f);

    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Make the window fit the heart exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(imageSize);
    ImGui::SetNextWindowPos(ImVec2(offset.x - windowBorderSize - imageSize.x, offset.y - windowBorderSize - imageSize.y));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Heart##{}", playerId).c_str(), nullptr, flags);
    ImGui::Image((ImTextureID)myTexData.DS, imageSize);

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewDeathCounter(const gui::DeathCounterData &data,
                                    size_t activePlayerCount, size_t playerId) {
    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    size_t deathCount = data.deathCount;

    // Format to a width of 4
    // See https://hackingcpp.com/cpp/libs/fmt.html
    std::string str = fmt::format("Death Counter {:4}", deathCount);

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
    ImVec2 pos = ImVec2(viewport.WorkPos.x + viewport.WorkSize.x, viewport.WorkPos.y + viewport.WorkSize.y);
    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // NOTE: hardcoded bottom right positioning
    ImGui::SetNextWindowPos(ImVec2(pos.x - textSize.x - windowBorderSize, pos.y - textSize.y - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Death Counter Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    // Heart
    ImVec2 offset = {pos.x - textSize.x, pos.y};
    NewHeartSprite(offset, playerId);

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewDeathPopup(const gui::DeathPopupData &data,
                                  size_t activePlayerCount, size_t playerId) {
    if (!enableDeathPopup) {
        // Early return
        return;
    }

    // If the visible timer has run out
    if (data.visibleTimer <= 0.0f) {
        // Early return
        return;
    }

    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    std::string str = fmt::format("DEATH POPUP");

    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Middle of viewport. NOTE: hardcoded middle of viewport positioning
    ImVec2 pos = ImVec2(viewport.WorkPos.x + viewport.WorkSize.x / 2.0f, viewport.WorkPos.y + viewport.WorkSize.y / 2.0f);
    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // NOTE: hardcoded middle of viewport positioning
    ImGui::SetNextWindowPos(ImVec2(pos.x - textSize.x - windowBorderSize, pos.y - textSize.y - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Death Popup Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewFinishPopup(const gui::FinishPopupData &data,
                                   size_t activePlayerCount, size_t playerId) {
    if (!enableFinishPopup) {
        // Early return
        return;
    }

    // If the visible timer has run out
    if (data.visibleTimer <= 0.0f) {
        // Early return
        return;
    }

    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    std::string str = fmt::format("FINISH POPUP");

    size_t sv = 0;

    float windowBorderSize = 0.0f;
    if (enableTextWindowBorder) {
        // Display a window border for debug purposes
        windowBorderSize = ImGui::GetStyle().WindowBorderSize;
    } else {
        // No window border
        sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); });
    }

    // Middle of viewport. NOTE: hardcoded middle of viewport positioning
    ImVec2 pos = ImVec2(viewport.WorkPos.x + viewport.WorkSize.x / 2.0f, viewport.WorkPos.y + viewport.WorkSize.y / 2.0f);
    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // NOTE: hardcoded middle of viewport positioning
    ImGui::SetNextWindowPos(ImVec2(pos.x - textSize.x - windowBorderSize, pos.y - textSize.y - windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Finish Popup Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewTimer(const gui::TimerData &data,
                             size_t activePlayerCount, size_t playerId) {
    ImGuiViewport viewport = CalcPlayerViewport(Context::get().extent, activePlayerCount, playerId);

    float time = data.time;

    // Format to a width of 8 and to a precision of 3
    // See https://hackingcpp.com/cpp/libs/fmt.html
    std::string str = fmt::format("Timer {:8.3f}", time);

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
    ImVec2 pos = ImVec2(viewport.WorkPos.x + viewport.WorkSize.x, viewport.WorkPos.y);
    ImVec2 textSize = ImGui::CalcTextSize(str.c_str());

    // Make the window fit the text exactly
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0)); });
    sv = PushBackStyleVar(sv, []() { ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); });

    ImGui::SetNextWindowSize(textSize);
    // NOTE: hardcoded top right positioning
    ImGui::SetNextWindowPos(ImVec2(pos.x - textSize.x - windowBorderSize, pos.y + windowBorderSize));
    ImGui::SetNextWindowBgAlpha(0.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;

    ImGui::Begin(fmt::format("Timer Window##{}", playerId).c_str(), nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::LoadingBar(float progress, ImVec2 position)
{
    // Get the main viewport to determine screen dimensions
    const ImGuiViewport* viewport = ImGui::GetMainViewport();

    // Calculate position at the bottom of the screen (ignoring the passed position parameter)
    ImVec2 windowPos = ImVec2(viewport->WorkPos.x, viewport->WorkPos.y + viewport->WorkSize.y - 100.0f);
    ImVec2 windowSize = ImVec2(viewport->WorkSize.x, 100.0f);

    // Size for the progress bar to fit full width with some padding
    ImVec2 progressBarSize = ImVec2(windowSize.x - 20.0f, 20.0f);

    // Flags to get a non-interactable blank window to draw on
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoInputs;
    ImGui::SetNextWindowPos(windowPos);
    ImGui::SetNextWindowSize(windowSize);
    ImGui::Begin("Loading", nullptr, flags);
    // Display a progress bar; progress value should be in the range [0.0f, 1.0f]
    ImGui::ProgressBar(progress / 100.f, progressBarSize);
    ImGui::Text("Loading... %.0f%%", progress);
    ImGui::End();
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

    // Make the window fit the heart exactly
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
}

void ImGuiRenderer::NewCharacterInfo(const CharacterEntity *character) {
    if (ImGui::CollapsingHeader(character->GetName().c_str())) {
        // Add camera position
        ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",
                    character->GetCamera()->GetPosition().x,
                    character->GetCamera()->GetPosition().y,
                    character->GetCamera()->GetPosition().z);
    }
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

   ImGui::Text("Directional Light: (%.2f, %.2f, %.2f)",
        LightManager::getInstance().GetLights()[0]->position.x,
        LightManager::getInstance().GetLights()[0]->position.y,
        LightManager::getInstance().GetLights()[0]->position.z
    );

    static bool initialized = false;
    static float SunElevation = 0.0f;
    static float SunAzimuthal = 0.0f;
    static const float distance = 1.0f;

    auto lights = LightManager::getInstance().GetLights();
    if (lights.empty())
        return;

    auto *sunLight = lights[0];

    if (!initialized) {
        sunLight->view = -43.0f;
        sunLight->far = 50.0f;
        sunLight->near = -125.0f;
        initialized = true;
    }



    if (ImGui::CollapsingHeader("Directional Light"))
    {
        ImGui::Text("Sun Angles");
        ImGui::SliderFloat("Elevation - Phi", &SunElevation, 0.0f, 1.5708f, "%.2f");
        ImGui::SliderFloat("Azimuthal - Theta", &SunAzimuthal, -3.141f, 3.141f, "%.2f");

        ImGui::Text("Light Camera Settings");
        ImGui::SliderFloat("View", &sunLight->view, -200.0f, 200.0f, "%.2f");
        ImGui::SliderFloat("Near", &sunLight->near, -200.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Far", &sunLight->far, 0.0f, 50.0f, "%.2f");

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

    // SSAO settings
    if (ImGui::CollapsingHeader("SSAO"))
    {
        ImGui::SliderInt("Directions: ", &vkutil::ssaoSettings.NumDirections, 1, 64);
        ImGui::SliderInt("Steps: ", &vkutil::ssaoSettings.NumSteps, 1, 64);
        ImGui::SliderFloat("Radius: ", &vkutil::ssaoSettings.Radius, 0.1f, 10.0f);
        ImGui::SliderFloat("StepSize: ", &vkutil::ssaoSettings.StepSize, 0.0f, 0.1f);
        ImGui::SliderFloat("Intensity: ", &vkutil::ssaoSettings.intensity, 0.0f, 10.0f);
    }

    // SSR settings
    //int MaxSteps;
    //int BinarySearchIterations;
    //float MaxDistance;
    //float thickness;
    if (ImGui::CollapsingHeader("SSR"))
    {
        ImGui::SliderInt("MaxSteps: ", &vkutil::ssrSettings.MaxSteps, 1, 500);
        ImGui::SliderFloat("MaxDistance: ", &vkutil::ssrSettings.MaxDistance, 0.0f, 20.0f);
        ImGui::SliderInt("BSIterations: ", &vkutil::ssrSettings.BinarySearchIterations, 0, 100);
        ImGui::SliderFloat("Thickness: ", &vkutil::ssrSettings.thickness, 0, 1.0f);
        ImGui::SliderFloat("StepSize: ", &vkutil::ssrSettings.StepSize, 0.0f, 0.5f);
    }

    static bool enableTextureDebug = false;
    ImGui::Checkbox("Debug Textures", &enableTextureDebug);
    if (enableTextureDebug)
    {
        ImGui::Begin("Debug Textures");
        for (size_t i = 0; i < textureIDs.size(); ++i) {
            ImGui::Image(textureIDs[i], ImVec2(800, 600));
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

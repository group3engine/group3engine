#include "Context.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "RenderPass.hpp"
#include "ImGuiRenderer.hpp"
#include "Utils.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <spdlog/fmt/fmt.h>

#include "TextureManager.hpp"
#include "spdlog/spdlog.h"

#include <tracy/Tracy.hpp>

namespace {
    auto PushBackStyleVar = [](size_t i, std::function<void()> f) {
        f();
        return ++i;
    };

    bool enableTextWindowBorder = true;
    bool enableDeathPopup = true;
    bool enableFinishPopup = true;
}

void ImGuiRenderer::Initialize(const Context &context, TextureManager *textureManager) {
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

    // Load texture
    // TODO: Cleanup
    {
        std::filesystem::path path = std::filesystem::path(CMAKE_SOURCE_DIR) / "assets" / "heart.png";
        std::string textureName = "heart";

        // TODO: Return a pointer to the newly created texture?
        textureManager->addTexture(path, textureName);

        Texture *texture = textureManager->GetTexture(textureName);

        // Create Descriptor Set using ImGUI's implementation
        myTexData.DS =
            ImGui_ImplVulkan_AddTexture(vkutil::clampToEdgeSamplerAniso, texture->image.imageView,
                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        myTexData.Width = texture->image.mWidth;
        myTexData.Height = texture->image.mHeight;
        SPDLOG_INFO("ImGui loaded {} with, width {} height {}", textureName, texture->image.mWidth, texture->image.mHeight);
    }
}

void ImGuiRenderer::NewHeartSprite(const ImVec2 &offset) {
    // TODO: Remove hardcoded image size
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

    ImGui::Begin("Heart", nullptr, flags);
    ImGui::Image((ImTextureID)myTexData.DS, imageSize);

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewDeathCounter(const gui::DeathCounterData &data) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

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
    ImVec2 pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y + viewport->WorkSize.y);
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

    ImGui::Begin("Death Counter Window", nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    // Heart
    ImVec2 offset = {pos.x - textSize.x, pos.y};
    NewHeartSprite(offset);

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewDeathPopup(const gui::DeathPopupData &data) {
    if (!enableDeathPopup) {
        // Early return
        return;
    }

    // If the visible timer has run out
    if (data.visibleTimer <= 0.0f) {
        // Early return
        return;
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();

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
    ImVec2 pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x / 2.0f, viewport->WorkPos.y + viewport->WorkSize.y / 2.0f);
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

    ImGui::Begin("Death Popup Window", nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewFinishPopup(const gui::FinishPopupData &data) {
    if (!enableFinishPopup) {
        // Early return
        return;
    }

    // If the visible timer has run out
    if (data.visibleTimer <= 0.0f) {
        // Early return
        return;
    }

    const ImGuiViewport *viewport = ImGui::GetMainViewport();

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
    ImVec2 pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x / 2.0f, viewport->WorkPos.y + viewport->WorkSize.y / 2.0f);
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

    ImGui::Begin("Finish Popup Window", nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    ImGui::PopStyleVar(sv);

    ImGui::End();
}

void ImGuiRenderer::NewTimer(const gui::TimerData &data) {
    const ImGuiViewport *viewport = ImGui::GetMainViewport();

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
    ImVec2 pos = ImVec2(viewport->WorkPos.x + viewport->WorkSize.x, viewport->WorkPos.y);
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

    ImGui::Begin("Timer Window", nullptr, flags);

    // Text
    ImGui::Text("%s", str.c_str());

    ImGui::PopStyleVar(sv);

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

void ImGuiRenderer::Update(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Camera>& camera)
{
    ZoneScopedN("ImGuiRenderer::Update");

    ImGui::BeginChild("Settings");
    // Display FPS
    ImGui::TextColored(
        ImVec4(0.76, 0.5, 0.0, 1.0), "FPS: (%.1f FPS), %.3f ms/frame",
        ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);

    // Add camera position
    ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)",
        camera->GetPosition().x,
        camera->GetPosition().y,
        camera->GetPosition().z
    );

   ImGui::Text("Directional Light: (%.2f, %.2f, %.2f)",
        scene->GetLights()[0].position.x,
        scene->GetLights()[0].position.y,
        scene->GetLights()[0].position.z
    );

    static bool initialized = false;
    static float SunElevation = 0.0f;
    static float SunAzimuthal = 0.0f;
    static const float distance = 1.0f;

    auto &lights = scene->GetLights();
    if (lights.empty())
        return;

    auto &sunLight = lights[0];

    if (!initialized) {
        SunElevation = 0.89f; // default elevation // -21
        SunAzimuthal = 0.0f; // default azimuth // 45
        sunLight.view = -43.0f;
        sunLight.far = 50.0f;
        sunLight.near = -125.0f;
        initialized = true;
    }

    // Phis is elevation
    // Theta is azimuthal
    const float ElevationPhi = (SunElevation);
    const float AzimuthalTheta = (SunAzimuthal);

    const float x = cosf(ElevationPhi) * cosf(AzimuthalTheta) * distance;
    const float y = sinf(ElevationPhi) * distance;
    const float z = cosf(ElevationPhi) * sinf(AzimuthalTheta) * distance;

    sunLight.position.x = x;
    sunLight.position.y = y;
    sunLight.position.z = z;
    sunLight.position = sunLight.position;

    if (ImGui::CollapsingHeader("Directional Light"))
    {
        ImGui::Text("Sun Angles");
        ImGui::SliderFloat("Elevation - Phi", &SunElevation, 0.0f, 1.5708f, "%.2f");
        ImGui::SliderFloat("Azimuthal - Theta", &SunAzimuthal, -3.141f, 3.141f, "%.2f");

        ImGui::Text("Light Camera Settings");
        ImGui::SliderFloat("View", &sunLight.view, -200.0f, 200.0f, "%.2f");
        ImGui::SliderFloat("Near", &sunLight.near, -200.0f, 1.0f, "%.2f");
        ImGui::SliderFloat("Far", &sunLight.far, 0.0f, 50.0f, "%.2f");

        ImGui::SliderFloat("Shadow bias: ", &vkutil::ShadowBias, 0.0f, 10.0f);
        ImGui::SliderFloat("Shadow slope: ", &vkutil::ShadowSlope, 0.0f, 10.0f);
    }

    if (ImGui::CollapsingHeader("Lights")) {
        auto &lights = scene->GetLights();
        for (size_t i = 1; i < lights.size() - 1; ++i) {
            if (lights[i].Type != LightType::Directional) {
                std::string label = "Light " + std::to_string(i) + " Position";
                ImGui::SliderFloat3(label.c_str(), &lights[i].position.x, -10.0f, 10.0f, "%.2f");
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
    ImGui_ImplVulkan_RemoveTexture(myTexData.DS);

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

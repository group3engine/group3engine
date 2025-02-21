#include "Context.hpp"
#include "Scene.hpp"
#include "Camera.hpp"
#include "RenderPass.hpp"
#include "ImGuiRenderer.hpp"
#include "Utils.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

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

void ImGuiRenderer::Update(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Camera>& camera)
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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


    if (ImGui::CollapsingHeader("Directional Light"))
    {
        auto &lights = scene->GetLights();
        float* SunPosition[3] = { & lights[0].position.x, & lights[0].position.y, & lights[0].position.z };
        ImGui::SliderFloat("X: ", SunPosition[0], -300.0f, 300.0f);
        ImGui::SliderFloat("Y: ", SunPosition[1], -10.0f, 2000.0f);
        ImGui::SliderFloat("Z: ", SunPosition[2], -300.0f, 300.0f);
        ImGui::SliderFloat("View: ", &lights[0].view, -20.0f, 100.0f, "%.2f");
        ImGui::SliderFloat("Near: ", &lights[0].near, 0.1f, 100.0f);
        ImGui::SliderFloat("Far: ", &lights[0].far, 0.1f, 1000.0f);
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


    // TODO: Entities doesn't have a GetPosition or SetPosition to make this simple
    //if (ImGui::CollapsingHeader("Entities")) {
    //    auto &entities = scene->GetEntities();
    //    for (size_t i = 0; i < entities.size(); ++i) {
    //        // Get current position
    //        glm::vec3 pos = entities[i].GetPosition();
    //        // Unique label for each entity
    //        std::string label = "Entity " + std::to_string(i) + " Position";
    //        // Slider to edit position
    //        if (ImGui::SliderFloat3(label.c_str(), &pos.x, -10.0f, 10.0f,
    //                                "%.2f")) {
    //            // If the slider changes the value, update the entity
    //            entities[i].SetPosition(pos);
    //        }
    //    }
    //}

    ImGui::EndChild();

    //ImGui::ShowDemoWindow();
}

void ImGuiRenderer::Render(VkCommandBuffer cmd, const Context &context, uint32_t imageIndex)
{
    ImGui::Render();
    ImDrawData *main_draw_data = ImGui::GetDrawData();
    ImGui_ImplVulkan_RenderDrawData(main_draw_data, cmd);
}

void ImGuiRenderer::Shutdown(const Context& context)
{
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

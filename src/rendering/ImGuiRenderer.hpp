#ifndef RENDERING_IMGUIRENDERER_HPP
#define RENDERING_IMGUIRENDERER_HPP

#include <volk.h>
#include <vector>
#include <functional>
#include <iostream>
#include <memory>

class ImVec2;

class Context;
class Scene;
class Camera;
class TextureManager;

struct MyTextureData {
    VkDescriptorSet DS{};         // Descriptor set: this is what you'll pass to Image()

    int             Width;
    int             Height;
};

namespace gui {
struct DeathCounterData {
    size_t deathCount;
};

struct DeathPopupData {
    float visibleTimer;
};

struct FinishPopupData {
    float visibleTimer;
};

struct TimerData {
    float time;
};
} // namespace gui

namespace ImGuiRenderer
{
    static std::vector<std::function<void()>> ImGuiComponents;
    static std::vector<void *> textureIDs;

    inline void AddImGuiComponent(std::function<void()> func) {
        if (func) { // Check if the function is valid
            ImGuiComponents.push_back(func);
            std::cout << "Added component. Total: "
                        << ImGuiComponents.size() << std::endl;
        } else {
            std::cout << "Failed to add: Component is null!" << std::endl;
        }
    }

    // Todo: When resizing, somehow clear the list and add the new re-sized versions?
    void AddTexture(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);


    static std::vector<VkDescriptorPoolSize> ImGuiPoolSizes = {
        {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
    };

    void NewHeartSprite(const ImVec2 &offset);
    void NewDeathCounter(const gui::DeathCounterData &data);
    void NewDeathPopup(const gui::DeathPopupData &data);
    void NewFinishPopup(const gui::FinishPopupData &data);
    void NewTimer(const gui::TimerData &data);

    void Initialize(const Context &context, TextureManager *textureManager);
    void Shutdown(const Context &context);
    void NewFrame();
    void Update(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Camera>& camera);
    void EndFrame();
    void Render(VkCommandBuffer cmd, const Context& context, uint32_t imageIndex);

    inline VkDescriptorPool imGuiDescriptorPool;
    inline VkRenderPass imGuiRenderPass;

    inline MyTextureData myTexData;
}
#endif // RENDERING_IMGUIRENDERER_HPP

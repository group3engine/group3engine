#ifndef RENDERING_IMGUIRENDERER_HPP
#define RENDERING_IMGUIRENDERER_HPP

#include "Volk.hpp"
#include <vector>
#include <functional>
#include <iostream>
#include <memory>

#include "Context.hpp"
#include "Fonts.hpp"
#include "Themes.hpp"

class ImVec2;
class ImFont;

class Context;
class Scene;
class Camera;
class TextureManager;
class CharacterEntity;

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

namespace Settings {
struct ActivePlayerCountOverride {
    std::vector<const char *> items = {"INACTIVE", "1", "2", "3", "4"};
    const char *activeItem = nullptr;
};
} // namespace Settings
} // namespace gui

struct Message{
    std::string playerName;
    std::string text;
    std::string timestamp;
};

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

    // Load textures with the TextureManager and add UI textures to ImGuiRenderer
    void AddTextures(TextureManager *textureManager, const std::filesystem::path &path, std::string textureName);
    // Remove UI textures from ImGuiRenderer
    void RemoveTextures();

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

    void BeginMainMenu(const Context &context);

    const char *AddMainMenuPlayerCountSelection(const Context &context,
                                                const std::vector<const char *> &playerCounts,
                                                const char *playerCountSelection);

    const std::filesystem::path *
    AddMainMenuSceneSelection(const Context &context,
                              const std::vector<std::filesystem::path *> &scenePaths,
                              const std::filesystem::path *scenePathSelection);

    const char *NewPlayerCountSelection(const std::vector<const char *> &playerCounts,
                                        const char *playerCountSelection);

    const std::filesystem::path *
    NewSceneSelection(const std::vector<std::filesystem::path *> &scenePaths,
                      const std::filesystem::path *scenePathSelection);

    void AddLoadSceneButton(const std::filesystem::path &pendingScenePath,
                            size_t pendingPlayerCount);
    void AddQuitButton();

    void EndMainMenu();

    // Player UI
    void NewHeartSprite(const ImVec2 &offset, size_t playerId);
    void NewDeathCounter(const gui::DeathCounterData &data, size_t activePlayerCount, size_t playerId);
    void NewDeathPopup(const gui::DeathPopupData &data, size_t activePlayerCount, size_t playerId);
    void NewFinishPopup(const gui::FinishPopupData &data, size_t activePlayerCount, size_t playerId);
    void NewTimer(const gui::TimerData &data, size_t activePlayerCount, size_t playerId);

    void LoadingBar(float progress, ImVec2 position);

    void Image(std::string const &imageName, ImVec2 position, ImVec2 size);

    void Text(std::string const &text, ImVec2 position, ImFont *font, size_t activePlayerCount, size_t playerId);

    void ChatWindow(const std::vector<Message> &messages, std::function<void(std::string, std::string)> callback);


    void NewActivePlayerCountOverride(Scene *scene, gui::Settings::ActivePlayerCountOverride &settings);

    void NewCharacterInfo(const CharacterEntity *character);

    void Initialize(const Context &context);
    void Shutdown(const Context &context);
    void NewFrame();
    void Update(Scene *scene);
    void EndFrame();
    void Render(VkCommandBuffer cmd, const Context& context, uint32_t imageIndex);

    inline VkDescriptorPool imGuiDescriptorPool;
    inline VkRenderPass imGuiRenderPass;

    // map of strings to texture data
    inline std::unordered_map<std::string, MyTextureData> textureDatas {};

    // the themes class
    inline Themes themes;

    // the number of messages, so we can scroll when a new message comes in
    inline size_t messageCount = 0;
}
#endif // RENDERING_IMGUIRENDERER_HPP

#pragma once
#include "Buffer.hpp"
#include "Camera.hpp"
#include "Image.hpp"
#include "Volk.hpp"

#include "Config.hpp"
inline size_t SKYBOX_WIDTH = 2;
inline size_t SKYBOX_HEIGHT = 2;
// Skybox
class Context;
class Skybox {
  public:
    Skybox(Context &context, Scene *scene, VkRenderPass renderpass);
    ~Skybox();

    void Execute(VkCommandBuffer cmd, size_t playerCount, size_t playerId);

    Image& GetSkyBoxImage() { return m_Skybox; }

  private:
    void CreatePipeline();
    void BuildDescriptorSetLayouts();
    void BuildDescriptors();
    void LoadCubemapFace(std::filesystem::path facePath, char **pixelData);

    Context &context;
    Scene *m_Scene;
    Image m_Skybox;
    Image m_RenderTarget;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;

    VkDescriptorSetLayout mPlayerDescriptorSetLayout;
    std::array<std::vector<VkDescriptorSet>, GlobalConfig::maxPlayers> mPlayerDescriptorSets;

    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<VkDescriptorSet> mDescriptorSets;

    Buffer m_vertexBuffer;
};

// https://github.com/KhronosGroup/Vulkan-Tools/blob/main/cube/cube.cpp
static const std::array<Vertex, 36> cubeVertices = {

    Vertex{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f}, {0, 0, 0}},

    Vertex{{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, -1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, -1.0f}, {1.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{-1.0f, -1.0f, 1.0f}, {0.0f, 0.0f}, {0, 0, 0}},

    Vertex{{1.0f, -1.0f, -1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, -1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}, {0, 0, 0}},

    Vertex{{-1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0, 0, 0}},

    Vertex{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, -1.0f}, {0.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, 1.0f, -1.0f}, {1.0f, 0.0f}, {0, 0, 0}},

    Vertex{{-1.0f, -1.0f, -1.0f}, {0.0f, 0.0f}, {0, 0, 0}},
    Vertex{{-1.0f, -1.0f, 1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f,  -1.0f, -1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, -1.0f}, {0.0f, 1.0f}, {0, 0, 0}},
    Vertex{{-1.0f, -1.0f, 1.0f}, {1.0f, 1.0f}, {0, 0, 0}},
    Vertex{{1.0f, -1.0f, 1.0f}, {1.0f, 0.0f}, {0, 0, 0}},
};
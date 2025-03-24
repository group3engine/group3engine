#pragma once
#include "Buffer.hpp"
#include "Camera.hpp"
#include "Image.hpp"
#include "Volk.hpp"

// Skybox
class Context;
class Skybox {
  public:
    Skybox(Context &context, std::shared_ptr<Camera> camera,
           VkRenderPass renderpass);
    ~Skybox();

    void Execute(VkCommandBuffer cmd);
    void Resize();

    Image& GetSkyBoxImage() { return m_Skybox; }

  private:
    void CreatePipeline();
    void BuildDescriptors();
    void CreateRenderPass();
    void CreateFramebuffer();
    void LoadCubemapFace(std::filesystem::path facePath, char **pixelData);

    Context &context;
    std::shared_ptr<Camera> camera;
    Image m_Skybox;

    Image m_RenderTarget;

    VkPipeline m_Pipeline;
    VkPipelineLayout m_PipelineLayout;
    VkRenderPass m_RenderPass;
    VkFramebuffer m_Framebuffer;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    std::vector<VkDescriptorSet> m_descriptorSets;

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
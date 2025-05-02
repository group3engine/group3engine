#pragma once

#include <memory>
#include <vector>
#include <array>

#include "Volk.hpp"

#include "Bloom.hpp"
#include "Camera.hpp"
#include "Composite.hpp"
#include "DepthPrepass.hpp"
#include "ForwardPass.hpp"
#include "PresentPass.hpp"
#include "Scene.hpp"
#include "ShadowMap.hpp"
#include "ImGuiRenderer.hpp"
#include "SSAO.hpp"
#include "SSR.hpp"
#include "SH2.hpp"
#include "Fog.hpp"
#include "FXAA.hpp"
#include "Config.hpp"

class Context;


struct FreedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocator allocator = VK_NULL_HANDLE;
};

class Renderer {
  public:
    Renderer(Context &context, Scene *scene);

    void CreateRenderPasses();

    void Destroy();

    void Render();
    void RenderUIOnly();
    void BeginFrame(VkCommandBuffer primaryCmd);
    void EndFrame(VkCommandBuffer primaryCmd);
    void Update(double deltaTime);

    Scene *m_scene;


    std::array<VkCommandBuffer, NUM_DRAW_THREADS> &GetSecondaryCommandBuffers() { return m_secondaryCommandBuffers[vkutil::currentFrame]; }
    VkCommandBuffer GetPrimaryCommandBuffer() { return m_primaryCommandBuffers[vkutil::currentFrame]; }

    Context &GetContext() const { return context; }

    DepthPrepass *GetDepthPrepass() const { return m_DepthPrepass.get(); }
    ShadowMap *GetShadowMap() const { return m_ShadowMap.get(); }
    ForwardPass *GetForwardPass() const { return m_ForwardPass.get(); }
    //GBuffer *GetGBuffer() const { return m_GBuffer.get(); }
    SSAO *GetSSAO() const { return m_SSAO.get(); }
    SSR *GetSSR() const { return m_SSR.get(); }
    Bloom *GetBloomPass() const { return m_BloomPass.get(); }
    Composite *GetCompositePass() const { return m_CompositePass.get(); }
    PresentPass *GetPresentPass() const { return m_PresentPass.get(); }
    Fog *GetFogPass() const { return m_Fog.get();}
    FXAA *GetFXAAPass() const { return m_FXAA.get(); }

    void AddCameras();

    uint32_t GetImageIndex() const { return mImageIndex; }

  private:
    void CreateResources();
    void CreateFences();
    void CreateSemaphores();
    void CreateCommandPool();
    void AllocateCommandBuffers();

    void Submit();
    void Present(uint32_t imageIndex);

  public:
    std::vector<std::vector<FreedBuffer>> mFreedBuffers;

  private:
    Context &context;
    std::vector<VkFence> m_Fences;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<std::array<VkCommandBuffer, NUM_DRAW_THREADS>> m_secondaryCommandBuffers;
    std::vector<VkCommandBuffer> m_primaryCommandBuffers;
    std::vector<VkCommandPool> m_primaryCommandPool;
    std::vector<std::array<VkCommandPool, NUM_DRAW_THREADS>> m_secondaryCommandPools;

    std::unique_ptr<DepthPrepass> m_DepthPrepass;
    std::unique_ptr<ForwardPass> m_ForwardPass;
    std::unique_ptr<SSAO> m_SSAO;
    std::unique_ptr<SSR> m_SSR;
    std::unique_ptr<Fog> m_Fog;
    std::unique_ptr<ShadowMap> m_ShadowMap;
    std::unique_ptr<Bloom> m_BloomPass;
    std::unique_ptr<Composite> m_CompositePass;
    std::unique_ptr<FXAA> m_FXAA;
    std::unique_ptr<PresentPass> m_PresentPass;

    std::vector<Camera *> m_cameras;

    uint32_t mImageIndex = 0;
    std::vector<Buffer> m_DebugUniform;
};

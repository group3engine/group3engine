#pragma once

#include <memory>
#include <vector>

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
#include "GBuffer.hpp"

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

    void BeginFrame(VkCommandBuffer cmd);
    void EndFrame(VkCommandBuffer cmd);
    void Update(double deltaTime);

    Scene *m_scene;


    // TODO: Check if we are calling this from within a frame
    VkCommandBuffer GetCommandBuffer() const { return m_commandBuffers[vkutil::currentFrame]; }

    Context &GetContext() const { return context; }

    const DepthPrepass *GetDepthPrepass() const { return m_DepthPrepass.get(); }
    const ShadowMap *GetShadowMap() const { return m_ShadowMap.get(); }
    const ForwardPass *GetForwardPass() const { return m_ForwardPass.get(); }
    const GBuffer *GetGBuffer() const { return m_GBuffer.get(); }
    const SSAO *GetSSAO() const { return m_SSAO.get(); }
    const SSR *GetSSR() const { return m_SSR.get(); }
    const Bloom *GetBloomPass() const { return m_BloomPass.get(); }
    const Composite *GetCompositePass() const { return m_CompositePass.get(); }
    const PresentPass *GetPresentPass() const { return m_PresentPass.get(); }

    Camera *GetCamera() const { return m_scene->GetActiveCamera(); }

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
    std::vector<VkCommandBuffer> m_commandBuffers;
    std::vector<VkCommandPool> m_commandPool;

    std::unique_ptr<DepthPrepass> m_DepthPrepass;
    std::unique_ptr<ForwardPass> m_ForwardPass;
    std::unique_ptr<GBuffer> m_GBuffer;
    std::unique_ptr<SSAO> m_SSAO;
    std::unique_ptr<SSR> m_SSR;
    std::unique_ptr<ShadowMap> m_ShadowMap;
    std::unique_ptr<Bloom> m_BloomPass;
    std::unique_ptr<Composite> m_CompositePass;
    std::unique_ptr<PresentPass> m_PresentPass;

    std::vector<Camera *> m_cameras;

    uint32_t mImageIndex = 0;
};

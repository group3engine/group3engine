#include "Renderer.hpp"

#include <filesystem>

#include <glm/gtc/random.hpp>

#include <tracy/TracyVulkan.hpp>

#include "Context.hpp"
#include "Light.hpp"
#include "Utils.hpp"
#include "SampleGLTFFilePaths.hpp"
#include <imgui.h>

namespace {
// This should be placed elsewhere. Put here for simplicity while testing
// Don't really need to define these, can pass the pos, dir, up directly to camera constructor
// Camera default values
constexpr glm::vec3 cameraPos = glm::vec3(1.0f, 1.0f, 1.0f); // 1.0f, 2.0f, -24.0f
constexpr glm::vec3 cameraDir = glm::vec3(1.0f, 1.0f, -1.0f);
constexpr glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0);
} // namespace

Renderer::Renderer(Context &context, std::shared_ptr<Scene> scene)
    : m_scene(scene), context{context} {
    std::printf("Launching Renderer\n");
    vkutil::renderType = vkutil::RenderType::FORWARD;

    CreateResources();

    // Samplers
    vkutil::repeatSamplerAniso = vkutil::CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
    vkutil::repeatSampler = vkutil::CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
    vkutil::clampToEdgeSamplerAniso = vkutil::CreateSampler(context, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FALSE, VK_COMPARE_OP_GREATER);

    // Camera
    m_camera = std::make_shared<Camera>(context, cameraPos, glm::normalize(cameraPos + cameraDir), up, context.extent.width / (float)context.extent.height);

    // GLFW callbacks
    glfwSetWindowUserPointer(context.mWindow, m_camera.get());
}

void Renderer::CreateRenderPasses() {
    // Renderer passes
    m_ShadowMap = std::make_unique<ShadowMap>(context, m_scene);
    m_DepthPrepass = std::make_unique<DepthPrepass>(context, m_scene, m_camera);
    m_ForwardPass = std::make_unique<ForwardPass>(context, m_ShadowMap->GetRenderTarget(), m_DepthPrepass->GetRenderTarget(), m_scene, m_camera);
    m_GBuffer = std::make_unique<GBuffer>(context, m_scene, m_camera);
    m_SSAO = std::make_unique<SSAO>(context, m_ForwardPass->GetDepthTarget(), m_ForwardPass->GetRenderTarget(), m_camera);
    m_SSR = std::make_unique<SSR>(context, m_ForwardPass->GetDepthTarget(), m_ForwardPass->GetRenderTarget(), m_GBuffer->GetMetallicRoughnessTarget(), m_ForwardPass->GetSkybox()->GetSkyBoxImage(), m_camera);
    m_BloomPass = std::make_unique<Bloom>(context, m_ForwardPass->GetBrightnessTarget());
    m_CompositePass = std::make_unique<Composite>(context, m_ForwardPass->GetRenderTarget(), m_BloomPass->GetRenderTarget(), m_SSAO->GetRenderTarget(), m_SSR->GetRenderTarget());
    m_PresentPass = std::make_unique<PresentPass>(context, m_CompositePass->GetRenderTarget());

    // ImGui
    ImGuiRenderer::Initialize(context);
    ImGuiRenderer::AddTextures(m_scene->GetTextureManager());
    // // TODO: This will cause a validation error if you re-size the window. Just needs to be updated when re-sized
    ImGuiRenderer::AddTexture(vkutil::clampToEdgeSamplerAniso, m_ShadowMap->GetRenderTarget().imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL);
}

void Renderer::RebuildSceneDescriptors() {
    m_ShadowMap->RebuildDescriptors();
    m_ForwardPass->RebuildDescriptors();
}

void Renderer::Destroy() {
    vkDeviceWaitIdle(context.device);

    ImGuiRenderer::Shutdown(context);

    m_DepthPrepass.reset();
    m_ForwardPass.reset();
    m_GBuffer.reset();
    m_ShadowMap.reset();
    m_BloomPass.reset();
    m_CompositePass.reset();
    m_SSAO.reset();
    m_SSR.reset();
    m_PresentPass.reset();
    m_camera.reset();

    vkDestroySampler(context.device, vkutil::repeatSamplerAniso, nullptr);
    vkDestroySampler(context.device, vkutil::repeatSampler, nullptr);
    vkDestroySampler(context.device, vkutil::clampToEdgeSamplerAniso, nullptr);

    for (auto &fence : m_Fences) {
        vkDestroyFence(context.device, fence, nullptr);
    }

    for (auto &semaphore : m_imageAvailableSemaphores) {
        vkDestroySemaphore(context.device, semaphore, nullptr);
    }

    for (auto &semaphore : m_renderFinishedSemaphores) {
        vkDestroySemaphore(context.device, semaphore, nullptr);
    }

    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        vkFreeCommandBuffers(context.device, m_commandPool[i], 1, &m_commandBuffers[i]);
    }

    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroyCommandPool(context.device, m_commandPool[i], nullptr);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; ++i) {
        TracyVkDestroy(context.tracyContexts[i]);
    }
}

void Renderer::CreateResources() {
    CreateFences();
    CreateSemaphores();
    CreateCommandPool();
    AllocateCommandBuffers();
}

void Renderer::CreateFences() {
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        // Fence
        VkFenceCreateInfo fenceInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT};

        VkFence fence = VK_NULL_HANDLE;
        VK_CHECK(vkCreateFence(context.device, &fenceInfo, nullptr, &fence), "Failedd to create Fence.");
        m_Fences.push_back(std::move(fence));
    }
}

void Renderer::CreateSemaphores() {
    // Image available semaphore
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        VkSemaphore semaphore = VK_NULL_HANDLE;
        VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &semaphore), "Failed to create image available semaphore");
        m_imageAvailableSemaphores.push_back(std::move(semaphore));
    }

    // Render finished sempahore
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkSemaphoreCreateInfo semaphoreInfo = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};

        VkSemaphore semaphore = VK_NULL_HANDLE;
        VK_CHECK(vkCreateSemaphore(context.device, &semaphoreInfo, nullptr, &semaphore), "Failed to create render finished semaphore");
        m_renderFinishedSemaphores.push_back(std::move(semaphore));
    }
}

void Renderer::CreateCommandPool() {
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkCommandPoolCreateInfo cmdPool{};
        cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPool.queueFamilyIndex = context.graphicsFamilyIndex;

        VkCommandPool commandPool = VK_NULL_HANDLE;
        VK_CHECK(vkCreateCommandPool(context.device, &cmdPool, nullptr, &commandPool), "Failed to create command pool");
        m_commandPool.push_back(std::move(commandPool));
    }
}

void Renderer::AllocateCommandBuffers() {
    for (size_t i = 0; i < (size_t)vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        // Allocate command buffers from command pool
        VkCommandBufferAllocateInfo cmdAlloc{};
        cmdAlloc.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAlloc.commandPool = m_commandPool[i];
        cmdAlloc.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmdAlloc.commandBufferCount = 1;

        VkCommandBuffer cmd = VK_NULL_HANDLE;
        VK_CHECK(vkAllocateCommandBuffers(context.device, &cmdAlloc, &cmd), "Failed to allocate command buffer");
        m_commandBuffers.push_back(cmd);

        // Calibrated context function pointers
        auto pfnVkGetPhysicalDeviceCalibrateableTimeDomainsEXT = vkGetPhysicalDeviceCalibrateableTimeDomainsEXT;
        auto pfnVkGetCalibratedTimestampsEXT = vkGetCalibratedTimestampsEXT;

        auto *tracyContext = TracyVkContextCalibrated(
            context.pDevice, context.device, context.graphicsQueue, cmd,
            pfnVkGetPhysicalDeviceCalibrateableTimeDomainsEXT, pfnVkGetCalibratedTimestampsEXT);

        context.tracyContexts.push_back(tracyContext);
    }
}

void Renderer::Render() {
    {
        ZoneScopedN("vkWaitForFences");

        vkWaitForFences(context.device, 1, &m_Fences[vkutil::currentFrame], VK_TRUE, UINT64_MAX);
    }

    uint32_t index;
    VkResult getImageIndex;
    {
        ZoneScopedN("vkAcquireNextImageKHR");

        getImageIndex = vkAcquireNextImageKHR(context.device, context.swapchain, UINT64_MAX, m_imageAvailableSemaphores[vkutil::currentFrame], VK_NULL_HANDLE, &index);
    }

    if (getImageIndex == VK_ERROR_OUT_OF_DATE_KHR) {
        // Recreate swapchain
        context.RecreateSwapchain();
        m_DepthPrepass->Resize();
        m_ShadowMap->Resize();
        m_ForwardPass->Resize();
        m_GBuffer->Resize();
        m_SSAO->Resize();
        m_SSR->Resize();
        m_BloomPass->Resize();
        m_CompositePass->Resize();
        m_PresentPass->Resize();

    } else if (getImageIndex != VK_SUCCESS && getImageIndex != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("Failed to aquire swapchain image");
    }

    {
        ZoneScopedN("vkResetFences");

        vkResetFences(context.device, 1, &m_Fences[vkutil::currentFrame]);
    }

    {
        ZoneScopedN("vkResetCommandBuffer");

        vkResetCommandBuffer(m_commandBuffers[vkutil::currentFrame], 0);
    }

    VkCommandBuffer &cmd = m_commandBuffers[vkutil::currentFrame];

    {
        ZoneScopedN("vk::Execute");

        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

        VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo), "Failed to begin command buffer");

        {
            ZoneScopedN("vk::Upload");

            m_camera->Upload(cmd);

            m_scene->UploadLights(cmd);

            // Upload animation data to GPU
            for (auto *entity : m_scene->GetEntities()) {
                if (entity->HasAnimator()) {
                    entity->GetAnimator().UploadJointBuffer(cmd);
                }
            }
        }

        {
            TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "vk::Frame", tracy::Color::Crimson);

            m_ShadowMap->Execute(cmd);
            m_DepthPrepass->Execute(cmd);
            m_ForwardPass->Execute(cmd);
            m_GBuffer->Execute(cmd);
            m_SSAO->Execute(cmd);
            m_SSR->Execute(cmd);
            m_BloomPass->Execute(cmd);
            m_CompositePass->Execute(cmd);
            m_PresentPass->Execute(cmd, index);
        }

        // Periodically collect the GPU events
        TracyVkCollect(context.tracyContexts[vkutil::currentFrame], cmd);

        vkEndCommandBuffer(cmd);
    }

    Submit();
    Present(index);

    vkutil::currentFrame = (vkutil::currentFrame + 1) % vkutil::MAX_FRAMES_IN_FLIGHT;
}

void Renderer::Submit() {
    ZoneScopedN("Renderer::Submit");
    ZoneValue(vkutil::currentFrame);

    VkPipelineStageFlags waitStage = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSubmitInfo subtmitInfo = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_imageAvailableSemaphores[vkutil::currentFrame],
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &m_commandBuffers[vkutil::currentFrame],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &m_renderFinishedSemaphores[vkutil::currentFrame]};

    VkResult result = vkQueueSubmit(context.graphicsQueue, 1, &subtmitInfo, m_Fences[vkutil::currentFrame]);

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to submit command buffers");
    }
}

void Renderer::Present(uint32_t imageIndex) {
    ZoneScopedN("Renderer::Present");

    VkPresentInfoKHR presentInfo = {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &m_renderFinishedSemaphores[vkutil::currentFrame],
        .swapchainCount = 1,
        .pSwapchains = &context.swapchain,
        .pImageIndices = &imageIndex,
    };

    VkResult result = vkQueuePresentKHR(context.presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // Recreate the swapchain
        context.RecreateSwapchain();
        m_DepthPrepass->Resize();
        m_ShadowMap->Resize();
        m_ForwardPass->Resize();
        m_GBuffer->Resize();
        m_SSAO->Resize();
        m_SSR->Resize();
        m_BloomPass->Resize();
        m_CompositePass->Resize();
        m_PresentPass->Resize();
    }
}

void Renderer::Update(double deltaTime) {
    ZoneScopedN("Renderer::Update");

    m_camera->Update(context.extent.width, context.extent.height, deltaTime);

    ImGuiRenderer::Update(m_scene, m_camera);

    m_SSAO->Update();
    m_SSR->Update();
    // Update passes
    m_ShadowMap->Update();
    m_ForwardPass->Update();
    m_PresentPass->Update();
}

//
// Created by thomas on 03/02/25.
//
#include "framebuffers.hpp"
lut::Framebuffer create_framebuffer(
    lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
    std::vector<VkImageView> aAttachments C5_DBGNAME_DEFN()) {
    VkFramebufferCreateInfo fbInfo{};
    fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fbInfo.renderPass = aRenderPass;
    fbInfo.attachmentCount = static_cast<uint32_t>(aAttachments.size());
    fbInfo.pAttachments = aAttachments.data();
    fbInfo.width = aWindow.swapchainExtent.width;
    fbInfo.height = aWindow.swapchainExtent.height;
    fbInfo.layers = 1;

    VkFramebuffer fb = VK_NULL_HANDLE;
    if (auto const res =
            vkCreateFramebuffer(aWindow.device, &fbInfo, nullptr, &fb);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create framebuffer\n"
            "vkCreateFramebuffer() returned %s",
            lut::to_string(res).c_str());
    }
    C5_DEBUG_SET_NAME(aWindow.device, fb, VK_OBJECT_TYPE_FRAMEBUFFER);

    return lut::Framebuffer(aWindow.device, fb);
}

void create_swapchain_framebuffers(
    lut::VulkanWindow &aWindow, VkRenderPass aRenderPass) {
    aWindow.swapFramebuffers.clear();
    for (std::size_t i = 0; i < aWindow.swapViews.size(); i++) {
        std::vector<VkImageView> attachments = {aWindow.swapViews[i]};
        lut::Framebuffer fb = create_framebuffer(aWindow, aRenderPass,attachments);
        aWindow.swapFramebuffers.emplace_back(std::move(fb));
    }
}
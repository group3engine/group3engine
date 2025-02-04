//
// Created by thomas on 29/01/25.
//

#ifndef VULKANTIME_POSTPROCESSRENDERPASS_CPP
#define VULKANTIME_POSTPROCESSRENDERPASS_CPP

#include "renderpasses.hpp"


// create a simple render pass with just a colour attachment
lut::RenderPass create_postprocess_render_pass(
    lut::VulkanWindow const &aWindow, bool presents, VkFormat aDepthFormat,
    bool hasDepth C5_DBGNAME_DEFN()) {
    VkAttachmentDescription attachments[2]{};
    // depth attachment

    attachments[0].format = aWindow.swapchainFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = presents
                                     ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
                                     : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachments[1].format = aDepthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference subpassAttachments[2]{};
    subpassAttachments[0].attachment = 0;  // referring to attachment 0
    subpassAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    subpassAttachments[1].attachment = 1;  // referring to attachment 1 (depth)
    subpassAttachments[1].layout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpasses[1]{};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpassAttachments;
    if (hasDepth) {
        subpasses[0].pDepthStencilAttachment = &subpassAttachments[1];
    }

    // requires a subpass dependency to ensurace that the first transition
    // happens after the presentation engine is done with it.

    VkSubpassDependency deps[4]{};
    // set up a dependency on the external subpass to finish writing to the
    // image before we start writing to it
    deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].srcAccessMask = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].dstSubpass = 0;
    deps[0].dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                           VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    // set up a dependency on the external subpass to finish writing to the
    // image before we start reading from it
    deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[1].dstSubpass = 0;
    deps[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    // set up a dependency on the external subpass to finish writing to the
    // depth buffer before we start reading from it
    deps[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[2].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[2].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[2].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[2].dstSubpass = 0;
    deps[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    // set up a subpass depdency for the depth buffer
    deps[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[3].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[3].srcAccessMask = VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[3].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[3].dstSubpass = 0;
    deps[3].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[3].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                           VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    VkRenderPassCreateInfo passInfo{};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.attachmentCount = hasDepth ? 2 : 1;
    passInfo.pAttachments = attachments;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = subpasses;
    passInfo.dependencyCount = 4;
    passInfo.pDependencies = deps;

    VkRenderPass rpass = VK_NULL_HANDLE;
    if (auto const res =
            vkCreateRenderPass(aWindow.device, &passInfo, nullptr, &rpass);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create render pass\n"
            "vkCreateRenderPass() returned %s",
            lut::to_string(res).c_str());
    }

    C5_DEBUG_SET_NAME(aWindow.device, rpass, VK_OBJECT_TYPE_RENDER_PASS);

    return lut::RenderPass(aWindow.device, rpass);
}
#endif  // VULKANTIME_POSTPROCESSRENDERPASS_CPP

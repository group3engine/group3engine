//
// Created by thomas on 30/01/25.
//
#include "renderpasses.hpp"

// creates a simple render pass with a color and a depth attachment
lut::RenderPass create_render_pass_to_texture(lut::VulkanWindow const &aWindow, VkFormat aDepthFormat C5_DBGNAME_DEFN())
{
    VkAttachmentDescription attachments[2]{};
    attachments[0].format = aWindow.swapchainFormat;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // layout for render textures
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    attachments[1].format = aDepthFormat;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;


    VkAttachmentReference subpassAttachments[1]{};
    subpassAttachments[0].attachment = 0; // referring to attachment 0
    subpassAttachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachment{};
    depthAttachment.attachment = 1;
    depthAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    VkSubpassDescription subpasses[1]{};
    subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpassAttachments;
    subpasses[0].pDepthStencilAttachment = &depthAttachment;

    // requires a subpass dependency to ensurace that the first transition happens after the presentation engine is done with it.

    VkSubpassDependency deps[5]{};
    deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[0].srcAccessMask = 0;
    deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[0].dstSubpass = 0;
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[1].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[1].dstSubpass = 0;
    deps[1].dstAccessMask =
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    // add a depedency to ensure that the render texture has been finished reading from
    deps[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[2].srcSubpass = VK_SUBPASS_EXTERNAL;
    deps[2].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[2].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    deps[2].dstSubpass = 0;
    deps[2].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[2].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    deps[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[3].srcSubpass = 0;
    deps[3].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    deps[3].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    deps[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    deps[3].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

    deps[4].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps[4].srcSubpass = 0;
    deps[4].dstSubpass = VK_SUBPASS_EXTERNAL;
    deps[4].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    deps[4].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
    deps[4].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    deps[4].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

    VkRenderPassCreateInfo passInfo{};
    passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    passInfo.attachmentCount = 2;
    passInfo.pAttachments = attachments;
    passInfo.subpassCount = 1;
    passInfo.pSubpasses = subpasses;
    passInfo.dependencyCount = 5;
    passInfo.pDependencies = deps;

    VkRenderPass rpass = VK_NULL_HANDLE;
    if (auto const res = vkCreateRenderPass(aWindow.device, &passInfo, nullptr, &rpass); VK_SUCCESS != res)
    {
        throw lut::Error("Can't create render pass\n"
            "vkCreateRenderPass() returned %s", lut::to_string(res).c_str()
        );
    }

    C5_DEBUG_SET_NAME(aWindow.device, rpass, VK_OBJECT_TYPE_RENDER_PASS);

    return lut::RenderPass(aWindow.device, rpass);
}

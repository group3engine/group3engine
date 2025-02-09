//
// Created by thomas on 21/01/25.
//

#include "ShadowLightManager.hpp"
#include "to_string.hpp"
#include "vkutil.hpp"
#include "error.hpp"

namespace GraphicsThings
{


    void ShadowLightManager::create_all_shadow_descriptor_layout(const lut::VulkanWindow &aWindow)
    {
        VkDescriptorSetLayoutBinding bindings[1]{};
        // the shadow image
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[0].descriptorCount = MAX_LIGHTS;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        bindings[0].pImmutableSamplers = nullptr;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = sizeof(bindings) / sizeof(bindings[0]);
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                         res)
        {
            throw lut::Error("Can't create descriptor set layout\n"
                             "vkCreateDescriptorSetLayout() returned %s", lut::to_string(res).c_str()
            );
        }
        allShadowMapDescriptorSetLayout = lut::DescriptorSetLayout(aWindow.device, layout);

    }

    void ShadowLightManager::create_all_shadow_descriptor_set(const lut::VulkanWindow &aWindow)
    {
        allShadowMapDescriptorSet = lut::alloc_descriptor_set(aWindow, shadowDescriptorPool,
                                                              allShadowMapDescriptorSetLayout.handle);

        // create the vector of image infos and project matrix buffers
        std::vector<VkDescriptorImageInfo> imageInfos(MAX_LIGHTS);
        for (size_t i = 0; i < MAX_LIGHTS; i++)
        {
            imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            imageInfos[i].imageView = shadowLightViews[i].handle;
            imageInfos[i].sampler = shadowSampler.handle;

        }

        VkWriteDescriptorSet desc[1]{};
        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = allShadowMapDescriptorSet;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        desc[0].descriptorCount = MAX_LIGHTS;
        desc[0].pImageInfo = imageInfos.data();


        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);
    }

    void ShadowLightManager::create_shadow_render_pass(const lut::VulkanWindow &aWindow C5_DBGNAME_DEFN())
    {
        VkAttachmentDescription attachments[1]{}; // just the depth attachment
        attachments[0].format = glsl::kDepthFormat;
        attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
        attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        attachments[0].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

        VkAttachmentReference subpassAttachments[1]{};
        subpassAttachments[0].attachment = 0; // referring to attachment 0
        subpassAttachments[0].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpasses[1]{};
        subpasses[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpasses[0].pDepthStencilAttachment = &subpassAttachments[0];

        VkSubpassDependency deps[2]{};
        deps[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        deps[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        deps[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        deps[0].dstSubpass = 0;
        deps[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;


        deps[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps[1].srcSubpass = 0;
        deps[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        deps[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        deps[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        deps[1].dstAccessMask = 0;
        deps[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;


        VkRenderPassCreateInfo passInfo{};
        passInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        passInfo.attachmentCount = 1;
        passInfo.pAttachments = attachments;
        passInfo.subpassCount = 1;

        passInfo.pSubpasses = subpasses;
        passInfo.dependencyCount = 2;
        passInfo.pDependencies = deps;

        VkRenderPass rpass = VK_NULL_HANDLE;
        if (auto const res = vkCreateRenderPass(aWindow.device, &passInfo, nullptr, &rpass); VK_SUCCESS != res)
        {
            throw lut::Error("Can't create render pass\n"
                             "vkCreateRenderPass() returned %s", lut::to_string(res).c_str()
            );
        }

        C5_DEBUG_SET_NAME(aWindow.device, rpass, VK_OBJECT_TYPE_RENDER_PASS);

        shadowRenderPass = lut::RenderPass(aWindow.device, rpass);
    }

    void ShadowLightManager::create_shadow_map_pipeline_layout()
    {
        // define the push constant for the model matrix
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(glm::mat4);


        VkDescriptorSetLayout layouts[] = {matrixDescriptorSetLayout.handle};
        VkPipelineLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = sizeof(layouts) / sizeof(layouts[0]);
        layoutInfo.pSetLayouts = layouts;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstant;

        VkPipelineLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreatePipelineLayout(window->device, &layoutInfo, nullptr, &layout); VK_SUCCESS != res)
        {
            throw lut::Error("Can't create pipeline layout\n"
                             "vkCreatePipelineLayout() returned %s", lut::to_string(res).c_str()
            );
        }
        shadowMapPipelineLayout = lut::PipelineLayout(window->device, layout);
    }

    lut::Sampler ShadowLightManager::create_shadow_sampler(const lut::VulkanContext &aContext C5_DBGNAME_DEFN())
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.f;


        // enable compare operations with compare op less
        samplerInfo.compareEnable = VK_TRUE;
        samplerInfo.compareOp = VK_COMPARE_OP_LESS;
        // enable bias
        // not needed for now


        // set the border color to black
        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;


        VkSampler sampler = VK_NULL_HANDLE;
        if (auto const res = vkCreateSampler(aContext.device, &samplerInfo, nullptr, &sampler); VK_SUCCESS != res)
        {
            throw lut::Error("Unable to create sampler\n vkCreateSampler() returned %s", lut::to_string(res).c_str());
        }

        C5_DEBUG_SET_NAME(aContext.device, sampler, VK_OBJECT_TYPE_SAMPLER);
        return lut::Sampler(aContext.device, sampler);
    }

    ShadowLightManager::ShadowLightManager(lut::VulkanWindow *aWindow, lut::Allocator *aAllocator)
    {
        window = aWindow;
        allocator = aAllocator;


        shadowSampler = create_shadow_sampler(*aWindow);
        shadowDescriptorPool = lut::create_descriptor_pool(*aWindow);

        commandPool = lut::create_command_pool(*aWindow, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT |
                                                         VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        // create the image views and images for the shadow maps
        for (size_t i = 0; i < MAX_LIGHTS; i++)
        {
            std::tie(shadowLightImages[i], shadowLightViews[i]) = create_depth_buffer(*aWindow, *aAllocator);
        }

        // create the descriptor set for the shadow maps
        create_all_shadow_descriptor_layout(*aWindow);
        create_all_shadow_descriptor_set(*aWindow);

        // create the render pass for the shadow maps
        create_shadow_render_pass(*aWindow C5_DBGNAME_ARG("Shadow Render Pass"));

        // generate the descriptor layout for the projection matrix
        create_single_matrix_descriptor_layout(*aWindow);
        // create the pipeline for the shadow map
        create_shadow_map_pipeline_layout();

        // create the pipeline for the shadow maps
        delete shadowMapPipeline;

        shadowMapPipeline = new Pipeline(*aWindow, &(shadowMapPipelineLayout.handle), shadowRenderPass.handle,
                                         create_shadow_pipeline, KShadowMapVertexShaderPath,
                                         KShadowMapFragmentShaderPath, shadowMapExtent);
    }

    void ShadowLightManager::create_single_matrix_descriptor_layout(const lut::VulkanWindow &aWindow)
    {
        VkDescriptorSetLayoutBinding bindings[1]{};
        bindings[0].binding = 0;
        bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        bindings[0].descriptorCount = 1;
        bindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = bindings;

        VkDescriptorSetLayout layout = VK_NULL_HANDLE;
        if (auto const res = vkCreateDescriptorSetLayout(aWindow.device, &layoutInfo, nullptr, &layout); VK_SUCCESS !=
                                                                                                         res)
        {
            throw lut::Error("Can't create descriptor set layout\n"
                             "vkCreateDescriptorSetLayout() returned %s", lut::to_string(res).c_str()
            );
        }
        matrixDescriptorSetLayout = lut::DescriptorSetLayout(aWindow.device, layout);

    }

    std::tuple<lut::Image, lut::ImageView>
    ShadowLightManager::create_depth_buffer(const lut::VulkanWindow & aWindow, const lut::Allocator & aAllocator) const
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = glsl::kDepthFormat;
        imageInfo.extent.width = shadowMapExtent.width;
        imageInfo.extent.height = shadowMapExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.mipLevels = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr);
                VK_SUCCESS != res)
        {
            throw lut::Error("Can't create depth buffer image\n"
                             "vmaCreateImage() returned %s", lut::to_string(res).c_str()
            );
        }

        // create the image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = glsl::kDepthFormat;
        viewInfo.components = VkComponentMapping{};
        viewInfo.subresourceRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1};
        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView(aWindow.device, &viewInfo, nullptr, &view); VK_SUCCESS != res)
        {
            throw lut::Error("Can't create depth buffer image view\n"
                             "vkCreateImageView() returned %s", lut::to_string(res).c_str()
            );
        }


        return {lut::Image(aAllocator.allocator, image, allocation), lut::ImageView{aWindow.device, view}};
    }

    ShadowLightManager::~ShadowLightManager()
    {
        delete shadowMapPipeline;

    }

    lut::Pipeline
    create_shadow_pipeline(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
                                        VkPipelineLayout aPipelineLayout,
                                        char const *vertShaderPath, char const *fragShaderPath, VkExtent2D shadowMapExtent)
    {
        // load shader modules
        lut::ShaderModule vert = lut::load_shader_module(aWindow, vertShaderPath);
        lut::ShaderModule frag = lut::load_shader_module(aWindow, fragShaderPath);
        // define shader stages in the pipeline
        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vert.handle;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = frag.handle;
        stages[1].pName = "main";

        // define the vertex buffer inputs (just the position)
        VkVertexInputBindingDescription vertexInputs[1]{};
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(float) * 3;
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription vertexAttributes[1]{};
        vertexAttributes[0].binding = 0;
        vertexAttributes[0].location = 0;
        vertexAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        vertexAttributes[0].offset = 0;

        VkPipelineVertexInputStateCreateInfo inputInfo{};
        inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        inputInfo.vertexBindingDescriptionCount = 1;
        inputInfo.pVertexBindingDescriptions = vertexInputs;
        inputInfo.vertexAttributeDescriptionCount = 1;
        inputInfo.pVertexAttributeDescriptions = vertexAttributes;

        // define the depth buffer
        VkPipelineDepthStencilStateCreateInfo depthInfo{};
        depthInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthInfo.depthTestEnable = VK_TRUE;
        depthInfo.depthWriteEnable = VK_TRUE;
        depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
        depthInfo.minDepthBounds = 0.f;
        depthInfo.maxDepthBounds = 1.f;

        // define which primative the input is assembled into for rasterisation
        VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
        assemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        assemblyInfo.primitiveRestartEnable = VK_FALSE;
        // no tesselation stage
        // define the viewport and scissor rectangle
        VkViewport viewport{};
        viewport.x = 0.f;
        viewport.y = 0.f;
        viewport.width = float(shadowMapExtent.width);
        viewport.height = float(shadowMapExtent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = VkOffset2D{0, 0};
        scissor.extent = shadowMapExtent;
        VkPipelineViewportStateCreateInfo viewportInfo{};
        viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportInfo.viewportCount = 1;
        viewportInfo.pViewports = &viewport;
        viewportInfo.scissorCount = 1;
        viewportInfo.pScissors = &scissor;

        // define rasterisation options
        VkPipelineRasterizationStateCreateInfo rasterInfo{};
        rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterInfo.depthClampEnable = VK_FALSE;
        rasterInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;

        rasterInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        // set the depth bias (disable for now, not needed in my current scene)
        rasterInfo.depthBiasEnable = VK_FALSE;
        rasterInfo.depthBiasConstantFactor = 1.75f;
        rasterInfo.depthBiasClamp = 0.0f;
        rasterInfo.depthBiasSlopeFactor = 1.75f;
        rasterInfo.lineWidth = 1.f; // required

        // define multisampling state
        VkPipelineMultisampleStateCreateInfo samplingInfo{};
        samplingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        // no colour blending as no colour attachments

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = stages;
        pipelineInfo.pVertexInputState = &inputInfo;
        pipelineInfo.pInputAssemblyState = &assemblyInfo;
        pipelineInfo.pViewportState = &viewportInfo;
        pipelineInfo.pRasterizationState = &rasterInfo;
        pipelineInfo.pMultisampleState = &samplingInfo;
        pipelineInfo.pDepthStencilState = &depthInfo;
        pipelineInfo.layout = aPipelineLayout;
        pipelineInfo.renderPass = aRenderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        VkPipeline pipeline = VK_NULL_HANDLE;

        if (auto const res = vkCreateGraphicsPipelines(aWindow.device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
                                                       &pipeline);
                VK_SUCCESS != res)
        {
            throw lut::Error("Can't create graphics pipeline\n"
                             "vkCreateGraphicsPipelines() returned %s", lut::to_string(res).c_str()
            );
        }

        return lut::Pipeline(aWindow.device, pipeline);
    }
}
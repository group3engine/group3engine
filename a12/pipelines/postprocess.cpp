//
// Created by thomas on 30/01/25.
//
#include "pipeline.hpp"


// create a pipeline layout for the post process pipeline
lut::PipelineLayout create_postprocess_pipeline_layout(
    lut::VulkanContext const &aContext, VkDescriptorSetLayout *aLayouts,
    size_t aNumLayouts) {
    VkPipelineLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    layoutInfo.setLayoutCount = aNumLayouts;
    layoutInfo.pSetLayouts = aLayouts;
    layoutInfo.pushConstantRangeCount = 0;
    layoutInfo.pPushConstantRanges = nullptr;

    VkPipelineLayout layout = VK_NULL_HANDLE;
    if (auto const res = vkCreatePipelineLayout(aContext.device, &layoutInfo,
                                                nullptr, &layout);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create pipeline layout\n"
            "vkCreatePipelineLayout() returned %s",
            lut::to_string(res).c_str());
    }
    return lut::PipelineLayout(aContext.device, layout);
}


// post process pipeline
lut::Pipeline create_postprocess_pipeline(lut::VulkanWindow const &aWindow,
                                          VkRenderPass aRenderPass,
                                          VkPipelineLayout aPipelineLayout,
                                          char const *vertShaderPath,
                                          char const *fragShaderPath,
                                          bool aEnableDepthTest) {
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

    VkPipelineVertexInputStateCreateInfo inputInfo{};
    inputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // define the depth buffer
    VkPipelineDepthStencilStateCreateInfo depthInfo{};
    depthInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthInfo.depthTestEnable = aEnableDepthTest ? VK_TRUE : VK_FALSE;
    depthInfo.depthWriteEnable = aEnableDepthTest ? VK_TRUE : VK_FALSE;
    depthInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    depthInfo.minDepthBounds = 0.f;
    depthInfo.maxDepthBounds = 1.f;
    // define which primative the input is assembled into for rasterisation
    VkPipelineInputAssemblyStateCreateInfo assemblyInfo{};
    assemblyInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    assemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    assemblyInfo.primitiveRestartEnable = VK_FALSE;
    // no tesselation stage
    // define the viewport and scissor rectangle
    VkViewport viewport{};
    viewport.x = 0.f;
    viewport.y = 0.f;
    viewport.width = static_cast<float>(aWindow.swapchainExtent.width);
    viewport.height = static_cast<float>(aWindow.swapchainExtent.height);
    viewport.minDepth = 0.f;
    viewport.maxDepth = 1.f;

    VkRect2D scissor{};
    scissor.offset = VkOffset2D{0, 0};
    scissor.extent = VkExtent2D{aWindow.swapchainExtent.width,
                                aWindow.swapchainExtent.height};

    VkPipelineViewportStateCreateInfo viewportInfo{};
    viewportInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportInfo.viewportCount = 1;
    viewportInfo.pViewports = &viewport;
    viewportInfo.scissorCount = 1;
    viewportInfo.pScissors = &scissor;

    // define rasterisation options
    VkPipelineRasterizationStateCreateInfo rasterInfo{};
    rasterInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterInfo.depthClampEnable = VK_FALSE;
    rasterInfo.rasterizerDiscardEnable = VK_FALSE;
    rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterInfo.cullMode = VK_CULL_MODE_NONE;
    rasterInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterInfo.depthBiasEnable = VK_FALSE;
    rasterInfo.lineWidth = 1.f;  // required

    // define multisampling state
    VkPipelineMultisampleStateCreateInfo samplingInfo{};
    samplingInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    samplingInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // define blend state
    // we define one blend state per colour attachment

    VkPipelineColorBlendAttachmentState blendStates[1]{};
    blendStates[0].blendEnable = VK_FALSE;
    blendStates[0].colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    VkPipelineColorBlendStateCreateInfo blendInfo{};
    blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendInfo.logicOpEnable = VK_FALSE;
    blendInfo.attachmentCount = 1;
    blendInfo.pAttachments = blendStates;

    // Create the pipeline
    VkGraphicsPipelineCreateInfo pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;

    pipeInfo.stageCount = 2;  // vertex and fragment shader
    pipeInfo.pStages = stages;

    pipeInfo.pVertexInputState = &inputInfo;
    pipeInfo.pInputAssemblyState = &assemblyInfo;
    pipeInfo.pTessellationState = nullptr;  // no tesselation
    pipeInfo.pViewportState = &viewportInfo;
    pipeInfo.pRasterizationState = &rasterInfo;
    pipeInfo.pMultisampleState = &samplingInfo;
    pipeInfo.pDepthStencilState = &depthInfo;
    pipeInfo.pColorBlendState = &blendInfo;
    pipeInfo.pDynamicState = nullptr;  // no dynamic state

    pipeInfo.layout = aPipelineLayout;
    pipeInfo.renderPass = aRenderPass;
    pipeInfo.subpass = 0;  // index of the subpass in the render pass

    VkPipeline pipe = VK_NULL_HANDLE;
    if (auto const res = vkCreateGraphicsPipelines(
            aWindow.device, VK_NULL_HANDLE, 1, &pipeInfo, nullptr, &pipe);
        VK_SUCCESS != res) {
        throw lut::Error(
            "Can't create graphics pipeline\n"
            "vkCreateGraphicsPipelines() returned %s",
            lut::to_string(res).c_str());
    }

    return lut::Pipeline(aWindow.device, pipe);
}
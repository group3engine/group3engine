//
// Created by thomas on 29/12/24.
//

#include <iostream>
#include "ShadowLight.hpp"


namespace GraphicsThings
{




    // initialise all the vulkan stuff (depth buffers, descriptor sets, etc) needed per shadow casting light source
    void ShadowLight::initialise_mapping(const lut::VulkanWindow &aWindow, const lut::Allocator &aAllocator)
    {
        // create the command buffer
        mCommandBuffer = lut::alloc_command_buffer(aWindow, mShadowLightManager->commandPool.handle);
        // create the semaphore and fence
        mSemaphore = lut::create_semaphore(aWindow);
        mFence = lut::create_fence(aWindow, VK_FENCE_CREATE_SIGNALED_BIT);


        // create the shadow map projection matrix ubo
        mShadowProjUBO = lut::create_buffer(aAllocator, sizeof(glm::mat4),
                                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            0,
                                            VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
        // create the descriptor set for the biased projection matrix
        mShadowProjectionDescriptorSet = create_single_matrix_descriptor_set(*mShadowLightManager->window, mShadowLightManager->shadowDescriptorPool,
                                                                             mShadowProjUBO);


        // create the frame buffer for the shadow map
        create_shadow_map_framebuffer(aWindow);


    }



    // static function to initialise all the vulkan stuff (sampler, etc) needed by all the shadow casting light sources, and store the window and allocator so the individual light sources can allocate themselves on construction
    ShadowLight::ShadowLight(const glm::vec4 &aPosition, const glm::vec4 &aColor, ShadowLightManager * aShadowLightManager) : Light()
    {
        mPosition = aPosition;
        mColor = aColor;
        mShadowLightManager = aShadowLightManager;
        mIsShadow = true;


        enableShadow();
        // init the shadow projection matrix to the identity
        mShadowProjectionMatrix = glm::mat4(1);
        initialise_mapping(*mShadowLightManager->window, *mShadowLightManager->allocator);

    }


    // static function to initialise all the vulkan stuff (sampler, etc) needed by all the shadow casting light sources, and store the window and allocator so the individual light sources can allocate themselves on construction
    ShadowLight::ShadowLight(const glm::vec4 &aPosition, const glm::vec4 &aColor, const glm::vec3 & aDirection, ShadowLightManager * aShadowLightManager) : Light()
    {
        mPosition = aPosition;
        mColor = aColor;
        mDirection = aDirection;
        mIsDirectional = true;
        mShadowLightManager = aShadowLightManager;
        mIsShadow = true;


        enableShadow();
        // init the shadow projection matrix to the identity
        mShadowProjectionMatrix = glm::mat4(1);
        initialise_mapping(*mShadowLightManager->window, *mShadowLightManager->allocator);
    }


    void ShadowLight::update()
    {
        // if not directional
        if (!mIsDirectional) {
            // update the shadow projection matrix (used for rendering the shadow maps)
            glm::mat4 shadowView = glm::translate(-glm::vec3(mPosition));
            // rotation matrix by theta and phi
            glm::mat4 rotation = glm::rotate(theta, glm::vec3(0, 1, 0)) *
                                 glm::rotate(phi, glm::vec3(1, 0, 0));
            shadowView = rotation * shadowView;
            mShadowProjectionMatrix = glm::perspectiveRH_ZO(
                lut::Radians(90.f).value(), 1.0f, 0.5f, 100.f);
            mShadowProjectionMatrix = mShadowProjectionMatrix * shadowView;
        }
        else
        {
                // update the shadow projection matrix (used for rendering the shadow maps)
                glm::mat4 shadowView = glm::lookAtRH(glm::vec3(mPosition), glm::vec3(mPosition) - mDirection, glm::vec3(0, 1, 0));
                mShadowProjectionMatrix = glm::orthoRH_ZO(-50.0f, 50.0f, -50.0f, 50.0f, 0.5f, 100.0f) * shadowView;
        }
        // we also need a biased matrix for converting points from world space to ndc shadow space when calculating shadows.
        // equivalent of mat.xy = mat.xy *= 0.5 + 0.5
        glm::mat4 bias = glm::mat4(
                0.5f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.5f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.5f, 0.5f, 0.0f, 1.0f
        );
        mNdcShadowProjectionMatrix = bias * mShadowProjectionMatrix;
        upload_uniforms();
    }


    void ShadowLight::upload_uniforms()
    {
        lut::buffer_barrier(mCommandBuffer, mShadowProjUBO.buffer, VK_ACCESS_UNIFORM_READ_BIT,
                            VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT);

        vkCmdUpdateBuffer(mCommandBuffer, mShadowProjUBO.buffer, 0, sizeof(glm::mat4), &mShadowProjectionMatrix);

        lut::buffer_barrier(mCommandBuffer, mShadowProjUBO.buffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                            VK_ACCESS_UNIFORM_READ_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT);
    }

    void ShadowLight::disableShadow()
    {
        if (mEnabled)
        {
            mShadowLightManager->numShadowLights--;
            mShadowLightManager->shadowLights[mShadowLightNumber] = mShadowLightManager->shadowLights[mShadowLightManager->numShadowLights];
            mShadowLightManager->shadowLights[mShadowLightNumber]->mShadowLightNumber = mShadowLightNumber;
            Light::disable();
        }
    }

    void ShadowLight::enableShadow()
    {
        if (!mEnabled)
        {
#ifndef NDEBUG
            if (mShadowLightManager->numShadowLights >= MAX_LIGHTS)
            {
                throw std::runtime_error("Too many lights :/, try increasing MAX_LIGHTS in ./a12/glsl.hpp");
            }
#endif
            mShadowLightManager->shadowLights[mShadowLightManager->numShadowLights] = this;
            mShadowLightNumber = (int) mShadowLightManager->numShadowLights;
            mShadowLightManager->numShadowLights++;
            Light::enable();
        }
    }

    // barrier to make sure the previous shadow map has been rendered before we start rendering the next one
    void ShadowLight::wait_on_shadow_map()
    {
// make sure the frame done fence is signalled
        if (auto const res = vkWaitForFences(mShadowLightManager->window->device, 1, &mFence.handle, VK_TRUE,
                                             std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res)
        {
            throw lut::Error("Can't wait for fence\n"
                             "vkWaitForFences() returned %s", lut::to_string(res).c_str()
            );
        }

        // reset the fence

        if (auto const res = vkResetFences(mShadowLightManager->window->device, 1, &mFence.handle); VK_SUCCESS != res)
        {
            throw lut::Error("Can't reset fence\n"
                             "vkResetFences() returned %s", lut::to_string(res).c_str()
            );
        }
    }

    // sets the command buffer to record the shadow map
    void ShadowLight::begin_recording_shadow_map()
    {
        // begin recording commands
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        beginInfo.pInheritanceInfo = nullptr;
        if (auto const res = vkBeginCommandBuffer(mCommandBuffer, &beginInfo); VK_SUCCESS != res)
        {
            throw lut::Error("Can't begin command buffer\n"
                             "vkBeginCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }



    }

    // sets the command buffer to be able to be submitted (called after recording)
    void ShadowLight::end_recording_shadow_map()
    {
        // end recording commands
        if (auto const res = vkEndCommandBuffer(mCommandBuffer); VK_SUCCESS != res)
        {
            throw lut::Error("Can't end command buffer\n"
                             "vkEndCommandBuffer() returned %s", lut::to_string(res).c_str()
            );
        }

    }

    void ShadowLight::submit_shadow_map()
    {

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &mCommandBuffer;

        // don't wait for anything
        submitInfo.waitSemaphoreCount = 0;
        submitInfo.pWaitSemaphores = nullptr;
        submitInfo.pWaitDstStageMask = nullptr;


        if (auto const res = vkQueueSubmit(mShadowLightManager->window->graphicsQueue, 1, &submitInfo,
                                           mFence.handle); VK_SUCCESS != res)
        {
            throw lut::Error("Can't submit command buffer\n"
                             "vkQueueSubmit() returned %s", lut::to_string(res).c_str()
            );

        }

    }

    VkDescriptorSet
    ShadowLight::create_single_matrix_descriptor_set(lut::VulkanWindow const &aWindow, lut::DescriptorPool const &aPool,
                                                     lut::Buffer const &aBuffer) const
    {
        VkDescriptorSet set = lut::alloc_descriptor_set(aWindow, aPool, mShadowLightManager->matrixDescriptorSetLayout.handle);

        VkWriteDescriptorSet desc[1]{};
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = aBuffer.buffer;
        bufferInfo.range = VK_WHOLE_SIZE;

        desc[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        desc[0].dstSet = set;
        desc[0].dstBinding = 0;
        desc[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        desc[0].descriptorCount = 1;
        desc[0].pBufferInfo = &bufferInfo;

        constexpr auto numSets = sizeof(desc) / sizeof(desc[0]);
        vkUpdateDescriptorSets(aWindow.device, numSets, desc, 0, nullptr);

        return set;
    }


    // record and submit the shadow map for all the meshes given
    void ShadowLight::record_shadow_map(std::vector<StandardMesh const *> const &aMeshes)
    {

        wait_on_shadow_map();
        begin_recording_shadow_map();
        update();
        VkRenderPassAttachmentBeginInfo renderPassAttachmentBeginInfo{};
        renderPassAttachmentBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_ATTACHMENT_BEGIN_INFO;
        renderPassAttachmentBeginInfo.attachmentCount = 1;
        renderPassAttachmentBeginInfo.pAttachments = &mShadowLightManager->shadowLightViews[mLightNumber].handle;
        // begin the render pass
        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.pNext = &renderPassAttachmentBeginInfo;
        renderPassInfo.renderPass = mShadowLightManager->shadowRenderPass.handle;
        renderPassInfo.framebuffer = mShadowMapFramebuffer.handle;
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = mShadowLightManager->shadowMapExtent;
        VkClearValue clearValues[1] = {};
        clearValues[0].depthStencil.depth = 1.0f;
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = clearValues;
        vkCmdBeginRenderPass(mCommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // bind the pipeline
        vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *mShadowLightManager->shadowMapPipeline->get_pipeline());
        // bind the scene descriptor set
        vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mShadowLightManager->shadowMapPipelineLayout.handle, 0, 1, &mShadowProjectionDescriptorSet, 0,
                                nullptr);
        // for each opaque mesh
        for (auto mesh: aMeshes)
        {
            mesh->record_draw_shadow(mCommandBuffer);
        }

        // end the render pass
        vkCmdEndRenderPass(mCommandBuffer);
        end_recording_shadow_map();
        submit_shadow_map();
    }


    // create the framebuffer for the shadow map
    void ShadowLight::create_shadow_map_framebuffer(lut::VulkanWindow const &aWindow C5_DBGNAME_DEFN())
    {
        // attachments not needed - imageless framebuffer
//        VkImageView attachments[1] = {
//                aDepthView
//        };

        VkFramebufferAttachmentImageInfo attachmentImageInfo{};
        attachmentImageInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENT_IMAGE_INFO;
        attachmentImageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        attachmentImageInfo.width = mShadowLightManager->shadowMapExtent.width;
        attachmentImageInfo.height = mShadowLightManager->shadowMapExtent.height;
        attachmentImageInfo.layerCount = 1;
        attachmentImageInfo.viewFormatCount = 1;
        attachmentImageInfo.pViewFormats = &glsl::kDepthFormat;

        VkFramebufferAttachmentsCreateInfo fbAttachmentsInfo{};
        fbAttachmentsInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_ATTACHMENTS_CREATE_INFO;
        fbAttachmentsInfo.attachmentImageInfoCount = 1;
        fbAttachmentsInfo.pAttachmentImageInfos = &attachmentImageInfo;

        VkFramebufferCreateInfo fbInfo{};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.pNext = &fbAttachmentsInfo;
        fbInfo.flags |= VK_FRAMEBUFFER_CREATE_IMAGELESS_BIT;
        fbInfo.renderPass = mShadowLightManager->shadowRenderPass.handle;
        fbInfo.attachmentCount = 1;
        fbInfo.pAttachments = nullptr;
        fbInfo.width = mShadowLightManager->shadowMapExtent.width;
        fbInfo.height = mShadowLightManager->shadowMapExtent.height;
        fbInfo.layers = 1;

        VkFramebuffer fb = VK_NULL_HANDLE;
        if (auto const res = vkCreateFramebuffer(aWindow.device, &fbInfo, nullptr, &fb); VK_SUCCESS != res)
        {
            throw lut::Error("Can't create framebuffer\n"
                             "vkCreateFramebuffer() returned %s", lut::to_string(res).c_str()
            );
        }
        C5_DEBUG_SET_NAME(aWindow.device, fb, VK_OBJECT_TYPE_FRAMEBUFFER);

        mShadowMapFramebuffer = lut::Framebuffer(aWindow.device, fb);
    }




    glm::mat4 ShadowLight::getNdcShadowProjectionMatrix()
    {
        return mNdcShadowProjectionMatrix;
    }


    lut::Pipeline
    ShadowLight::create_shadow_pipeline(lut::VulkanWindow const &aWindow, VkRenderPass aRenderPass,
                           VkPipelineLayout aPipelineLayout,
                           char const *vertShaderPath, char const *fragShaderPath) const
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
        viewport.width = float(mShadowLightManager->shadowMapExtent.width);
        viewport.height = float(mShadowLightManager->shadowMapExtent.height);
        viewport.minDepth = 0.f;
        viewport.maxDepth = 1.f;

        VkRect2D scissor{};
        scissor.offset = VkOffset2D{0, 0};
        scissor.extent = mShadowLightManager->shadowMapExtent;
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
} // Graphicsthings
#include "Context.hpp"
#include "PrefilterSkybox.hpp"
#include "Pipeline.hpp"

PrefilterSkybox::PrefilterSkybox(Context &context, Image &skybox)
    : context{context}, skybox{skybox}
{
    constexpr uint32_t width = 128;
    constexpr uint32_t height = 128;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

    mipLevelImages.resize(mipLevels);

    m_PrefilteredSkybox = CreateImageTexture2D(
        "PrefilteredSkybox",
        context,
        width,
        height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        mipLevels,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        6
    );

    m_BRDFLut = CreateImageTexture2D(
        "BRDFLut",
        context,
        512, 512,
        VK_FORMAT_R16G16_SFLOAT,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1
    );

    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd)
    {
        vkutil::ImageBarrier(
            cmd,
            m_PrefilteredSkybox.image,
            VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 6 }
        );
    });

    uint32_t mipWidth = width;
    uint32_t mipHeight = height;
    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd) {

        // Transition the image layout to be SRC_OPTIMAL -> Should already be in this layout
        for (uint32_t face = 0; face < 6; face++)
        {
            mipWidth = width;
            mipHeight = height;

            for (uint32_t level = 1; level < mipLevels; level++)
            {
                VkImageBlit blit = {};
                blit.srcSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, level - 1, face, 1};
                blit.srcOffsets[0] = {0, 0, 0};
                blit.srcOffsets[1] = {int32_t(mipWidth), int32_t(mipHeight), 1};

                mipWidth >>= 1;
                if (mipWidth == 0)
                    mipWidth = 1;
                mipHeight >>= 1;
                if (mipHeight == 0)
                    mipHeight = 1;

                blit.dstSubresource = VkImageSubresourceLayers{VK_IMAGE_ASPECT_COLOR_BIT, level, face, 1};
                blit.dstOffsets[0] = {0, 0, 0};
                blit.dstOffsets[1] = {int32_t(mipWidth), int32_t(mipHeight), 1};

                vkutil::ImageBarrier(
                    cmd,
                    m_PrefilteredSkybox.image,
                    0,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    { VK_IMAGE_ASPECT_COLOR_BIT, level, 1, face, 1 }
                );

                vkCmdBlitImage(
                    cmd,
                    m_PrefilteredSkybox.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    m_PrefilteredSkybox.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1,
                    &blit,
                    VK_FILTER_LINEAR
                );


                vkutil::ImageBarrier(
                    cmd, m_PrefilteredSkybox.image,
                    VK_ACCESS_TRANSFER_WRITE_BIT,
                    VK_ACCESS_TRANSFER_READ_BIT,
                    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VK_PIPELINE_STAGE_TRANSFER_BIT,
                    VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, level, 1, face, 1}
                );
            }
        }

    });

    // Need to create an image view for each mip level of the main PrefilteredSkybox image
    for (uint32_t i = 0; i < mipLevelImages.size(); i++)
    {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_PrefilteredSkybox.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.subresourceRange = {};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = i;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6;
        VK_CHECK(vkCreateImageView(context.device, &viewInfo, nullptr, &mipLevelImages[i].imageView), "Failed to create image view for mip level: " + i);
    }

    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd) {

         vkutil::ImageBarrier(
            cmd,
            m_PrefilteredSkybox.image,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 6 }
         );

         vkutil::ImageBarrier(
             cmd,
             m_BRDFLut.image,
             VK_ACCESS_NONE,
             VK_ACCESS_SHADER_WRITE_BIT,
             VK_IMAGE_LAYOUT_UNDEFINED,
             VK_IMAGE_LAYOUT_GENERAL,
             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
             VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
             VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
         );

    });

    // Create prefilter resources
    BuildPrefilterDescriptorSets();
    CreatePrefilterPipeline();

    // Create BRDF LUT resources
    BuildBRDFLUTDescriptorSets();
    CreateBRDFLUTPipeline();
}

PrefilterSkybox::~PrefilterSkybox()
{
    m_PrefilteredSkybox.Destroy(context.device);
    m_BRDFLut.Destroy(context.device);
    for (auto &image : mipLevelImages) {
        image.Destroy(context.device);
    }
    vkDestroyPipeline(context.device, m_PrefilterSkyboxPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PrefilterSkyboxPipelineLayout, nullptr);

    vkDestroyPipeline(context.device, m_brdfLUTPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_brdfLUTPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_PrefilterSkyboxDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_brdfLUTDescriptorSetLayout, nullptr);
}

void PrefilterSkybox::Execute(VkCommandBuffer cmd)
{
    ZoneScopedN("Prefilter::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "Prefilter", tracy::Color::Gold);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "Prefilter Cubemap");
#endif // !DEBUG
    for (uint32_t mip = 0; mip < mipLevels; mip++)
    {
        vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrefilterSkyboxPipelineLayout, 0, 1, &descriptorSets[vkutil::currentFrame], 0, nullptr);
        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_PrefilterSkyboxPipeline);
        PushConstants pushConstants = {};
        pushConstants.mipLevel = mip;
        pushConstants.roughness = static_cast<float>(mip) / static_cast<float>(mipLevels - 1);
        vkCmdPushConstants(cmd, m_PrefilterSkyboxPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(PushConstants), &pushConstants);
        vkCmdDispatch(cmd, 16, 16   , 6);
    }

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif

    // Dispatch for BRDF LUT texture
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_brdfLUTPipelineLayout, 0, 1, &brdfLUTDescriptorSets[vkutil::currentFrame], 0, nullptr);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_brdfLUTPipeline);
    vkCmdDispatch(cmd, 64, 64, 1);
}

void PrefilterSkybox::TransitionResources(VkCommandBuffer cmd)
{
        vkutil::ImageBarrier(
            cmd,
            m_PrefilteredSkybox.image,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 6}
        );

        vkutil::ImageBarrier(
            cmd,
            m_BRDFLut.image,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
        );
}

void PrefilterSkybox::CreatePrefilterPipeline() {

   std::vector<VkPushConstantRange> pushConstants = {

        {
         .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
         .offset = 0,
         .size = sizeof(PushConstants)
        },
    };

   // Needs push constant to push current mip + roughness or something
   auto pipelineResult = PipelineBuilder(context.device, PipelineType::COMPUTE, VertexBinding::NONE, 0)
        .AddShader(assetsPath / "shaders" / "Prefilter.comp.spv", ShaderType::COMPUTE)
        .SetPipelineLayout({{m_PrefilterSkyboxDescriptorSetLayout}}, pushConstants)
        .Build();

    m_PrefilterSkyboxPipeline = pipelineResult.first;
    m_PrefilterSkyboxPipelineLayout = pipelineResult.second;
}

void PrefilterSkybox::CreateBRDFLUTPipeline()
{
   auto pipelineResult = PipelineBuilder(context.device, PipelineType::COMPUTE, VertexBinding::NONE, 0)
        .AddShader(assetsPath / "shaders" / "BRDFLUT.comp.spv", ShaderType::COMPUTE)
        .SetPipelineLayout({{m_brdfLUTDescriptorSetLayout}})
        .Build();

    m_brdfLUTPipeline = pipelineResult.first;
    m_brdfLUTPipelineLayout = pipelineResult.second;
}

void PrefilterSkybox::BuildPrefilterDescriptorSets()
{
    descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT), // Skybox
        vkutil::CreateDescriptorBinding(1, mipLevelImages.size(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Prefiltered skybox
    };

    m_PrefilterSkyboxDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_PrefilterSkyboxDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);


    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = skybox.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(mipLevelImages.size());
    for (uint32_t i = 0; i < mipLevelImages.size(); i++)
    {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = mipLevelImages[i].imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };

        imageInfos.push_back(imageInfo);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkutil::BulkImageUpdate(context, 1, imageInfos, descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }
}
void PrefilterSkybox::BuildBRDFLUTDescriptorSets()
{
    brdfLUTDescriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
    };

    m_brdfLUTDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_brdfLUTDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, brdfLUTDescriptorSets);

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {

        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = m_BRDFLut.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, brdfLUTDescriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }
}


#include "Context.hpp"
#include "IrradianceMap.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"

IrradianceMap::IrradianceMap(Context &context, Image &skybox) : context{context}, skybox{skybox}
{
    m_width = 32;
    m_height = 32;
    m_IrradianceMap = CreateImageTexture2D(
        "IrradianceMap",
        context,
        m_width,
        m_height,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        6
    );

    m_layers.resize(6);

    // Create 6 imageviews + framebuffers for each face
    for (uint32_t face = 0; face < 6; face++) {

        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        viewInfo.image = m_IrradianceMap.image;
        viewInfo.subresourceRange = {};
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = face; // first layer of the image that this image view will use
        viewInfo.subresourceRange.layerCount = 1; // Access to only its own layer
        VK_CHECK(vkCreateImageView(context.device, &viewInfo, nullptr, &m_layers[face].imageView), "Failed to create image view for irradiance map layer: " + face);
    }

    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd) {

         vkutil::ImageBarrier(
            cmd,
            m_IrradianceMap.image,
            0,
            VK_ACCESS_SHADER_WRITE_BIT,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
         );
    });

    BuildDescriptorSets();
    CreatePipeline();
}

IrradianceMap::~IrradianceMap()
{
    for (auto& layer : m_layers)
    {
        layer.Destroy(context.device);
    }
    m_IrradianceMap.Destroy(context.device);
    vkDestroyPipeline(context.device, m_IrradianceMapPipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_IrradianceMapPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, m_IrradianceMapDescriptorSetLayout, nullptr);
}

void IrradianceMap::Transition(VkCommandBuffer cmd)
{

    vkutil::ImageBarrier(
        cmd,
        m_IrradianceMap.image,
        VK_ACCESS_SHADER_WRITE_BIT,
        VK_ACCESS_SHADER_READ_BIT,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
    );

}

void IrradianceMap::Execute(VkCommandBuffer cmd)
{
    ZoneScopedN("IrradianceMap::Execute");
    TracyVkZoneC(context.tracyContexts[vkutil::currentFrame], cmd, "IrradianceMap", tracy::Color::Gold);

#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "IrradianceMap");
#endif // !DEBUG

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_IrradianceMapPipelineLayout, 0, 1, &descriptorSets[vkutil::currentFrame], 0, nullptr);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, m_IrradianceMapPipeline);
    vkCmdDispatch(cmd, m_width / 8, m_height / 8, 6);


#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif
}

void IrradianceMap::CreatePipeline() {

   // Needs push constant to push current mip + roughness or something
   auto pipelineResult = PipelineBuilder(context.device, PipelineType::COMPUTE, VertexBinding::NONE, 0)
        .AddShader(assetsPath / "shaders" / "IrradianceMap.comp.spv", ShaderType::COMPUTE)
        .SetPipelineLayout({{m_IrradianceMapDescriptorSetLayout}})
        .Build();

    m_IrradianceMapPipeline = pipelineResult.first;
    m_IrradianceMapPipelineLayout = pipelineResult.second;
}

void IrradianceMap::BuildDescriptorSets()
{
    descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        vkutil::CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_COMPUTE_BIT), // Skybox
        vkutil::CreateDescriptorBinding(1, m_layers.size(), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT) // Prefiltered skybox
    };

    m_IrradianceMapDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    vkutil::AllocateDescriptorSets(context, context.descriptorPool, m_IrradianceMapDescriptorSetLayout, vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);


    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = skybox.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        vkutil::UpdateDescriptorSet(context, 0, imageInfo, descriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    }

    std::vector<VkDescriptorImageInfo> imageInfos;
    imageInfos.reserve(m_layers.size());
    for (uint32_t i = 0; i < m_layers.size(); i++)
    {
        VkDescriptorImageInfo imageInfo = {
            .sampler = vkutil::clampToEdgeSamplerAniso,
            .imageView = m_layers[i].imageView,
            .imageLayout = VK_IMAGE_LAYOUT_GENERAL
        };

        imageInfos.push_back(imageInfo);
    }

    for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++)
    {
        vkutil::BulkImageUpdate(context, 1, imageInfos, descriptorSets[i], VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
    }
}



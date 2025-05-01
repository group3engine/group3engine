#include "Skybox.hpp"

#include "Context.hpp"
#include "Scene.hpp"
#include "Utils.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include <stb_image.h>
#include <stdexcept>

#include "RenderPassCommon.hpp"

Skybox::Skybox(Context& context, Scene *scene, VkRenderPass renderpass) :
    context{context}, m_Scene{scene}, m_RenderPass{renderpass}
{
    // When transitioning this image, the subresource in subresourceRange needs to be set to 6
    // to transition all layers of the imag, otherwise you transition only the first

    m_Skybox = CreateImageTexture2D(
        "Skybox",
        context,
        2048,
        2048,
        VK_FORMAT_R32G32B32A32_SFLOAT,
        VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT,
        1,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
        6
    );

    // TODO: The skybox path should be user provided earlier during engine init
    // This pass will then use that provided path to load skybox
    // This will prevent users from having to navigate into renderer code to change paths
    float* faceTextureData[6]; // Stores the pixel data from stb
    LoadCubemapFace(assetsPath / "Skybox/HDR/px.hdr",   &faceTextureData[0]);
    LoadCubemapFace(assetsPath / "Skybox/HDR/nx.hdr",   &faceTextureData[1]);
    LoadCubemapFace(assetsPath / "Skybox/HDR/py.hdr",   &faceTextureData[2]);
    LoadCubemapFace(assetsPath / "Skybox/HDR/ny.hdr",   &faceTextureData[3]);
    LoadCubemapFace(assetsPath / "Skybox/HDR/pz.hdr",   &faceTextureData[4]);
    LoadCubemapFace(assetsPath / "Skybox/HDR/nz.hdr",   &faceTextureData[5]);

    constexpr uint32_t width = 2048;
    constexpr uint32_t height = 2048;
    VkDeviceSize imageSize = width * height * (4 * sizeof(float)); // Size of a single face
    VkDeviceSize totalSize = imageSize * 6; // Size of total num of faces

    // Copy the pixel data to the staging buffer to store all face images data
    Buffer stagingBuffer = CreateBuffer("stagingBuffer", context, totalSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    void* data = nullptr;
    if (vmaMapMemory(context.allocator, stagingBuffer.allocation, &data) == VK_SUCCESS)
    {
        uint8_t* bufferData = static_cast<uint8_t*>(data);
        for (uint32_t i = 0; i < 6; i++)
        {
            assert(faceTextureData[i] != nullptr);
            std::memcpy(bufferData + (imageSize * i), faceTextureData[i], imageSize);
        }

    }
    vmaUnmapMemory(context.allocator, stagingBuffer.allocation);

    for (size_t i = 0; i < 6; i++)
    {
        stbi_image_free(faceTextureData[i]);
        faceTextureData[i] = nullptr;
    }

    std::vector<VkBufferImageCopy> copyRegions(6);
    for (uint32_t face = 0; face < 6; face++)
    {
        copyRegions[face].bufferOffset = imageSize * face;
        copyRegions[face].bufferRowLength = 0;
        copyRegions[face].bufferImageHeight = 0;

        copyRegions[face].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegions[face].imageSubresource.mipLevel = 0;
        copyRegions[face].imageSubresource.baseArrayLayer = face;
        copyRegions[face].imageSubresource.layerCount = 1;

        copyRegions[face].imageOffset = { 0,0,0 };
        copyRegions[face].imageExtent = { width, height, 1 };
    }

    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd)
    {
            vkutil::ImageBarrier(
                cmd,
                m_Skybox.image,
                0,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_UNDEFINED,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
            );

            vkCmdCopyBufferToImage(cmd, stagingBuffer.buffer, m_Skybox.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

            vkutil::ImageBarrier(
                cmd,
                m_Skybox.image,
                VK_ACCESS_TRANSFER_WRITE_BIT,
                VK_ACCESS_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VK_PIPELINE_STAGE_TRANSFER_BIT,
                VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
            );
    });


    vkutil::ExecuteSingleTimeCommands(context, [&](VkCommandBuffer cmd) {

         vkutil::ImageBarrier(
            cmd,
            m_Skybox.image,
            VK_ACCESS_TRANSFER_READ_BIT,
            VK_ACCESS_SHADER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6 }
         );
    });

    // Create the vertex buffer for the cube map
    VkDeviceSize vertexSize = sizeof(cubeVertices[0]) * cubeVertices.size();
    CreateAndUploadBuffer(context, cubeVertices.data(), vertexSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, m_vertexBuffer);

    stagingBuffer.Destroy();

    BuildDescriptorSetLayouts();
    BuildDescriptors();
    CreatePipeline();
}

Skybox::~Skybox()
{
    m_Skybox.Destroy(context.device);
    m_vertexBuffer.Destroy();
    vkDestroyPipeline(context.device, m_Pipeline, nullptr);
    vkDestroyPipelineLayout(context.device, m_PipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, mPlayerDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(context.device, mDescriptorSetLayout, nullptr);
}

void Skybox::Execute(VkCommandBuffer cmd, size_t playerCount, size_t playerId)
{
#ifdef _DEBUG
    vkutil::RenderPassLabel(cmd, "Skybox");
#endif // !DEBUG

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);

    VkViewport viewport = CalcViewport(context.extent, playerCount, playerId);
    vkCmdSetViewport(cmd, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {static_cast<int32_t>(viewport.x), static_cast<int32_t>(viewport.y)};
    scissor.extent = {static_cast<uint32_t>(viewport.width),
                        static_cast<uint32_t>(viewport.height)};
    vkCmdSetScissor(cmd, 0, 1, &scissor);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 0, 1,
                            &mPlayerDescriptorSets[playerId][vkutil::currentFrame], 0, nullptr);

    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_PipelineLayout, 1, 1,
                            &mDescriptorSets[vkutil::currentFrame], 0, nullptr);

    vkutil::MeshPushConstants pc = {};
    pc.ModelMatrix = glm::mat4(1.0f);
    vkCmdPushConstants(cmd, m_PipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(vkutil::MeshPushConstants), &pc);

    // Set up push constants
    VkDeviceSize offset[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer.buffer, offset);
    vkCmdDraw(cmd, 36, 1, 0, 0);

#ifdef _DEBUG
    vkutil::EndRenderPassLabel(cmd);
#endif // !DEBUG
}

void Skybox::CreatePipeline()
{
    std::vector<VkPushConstantRange> pushConstants = {

        {.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
         .offset = 0,
         .size = sizeof(vkutil::MeshPushConstants)
        },
    };

    auto skyboxPiplineRes = PipelineBuilder(context.device, PipelineType::GRAPHICS, VertexBinding::BIND, 0)
        .AddShader(assetsPath / "shaders/" / "skybox.vert.spv", ShaderType::VERTEX)
        .AddShader(assetsPath / "shaders/" / "skybox.frag.spv", ShaderType::FRAGMENT)
        .SetInputAssembly(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST)
        .SetDynamicState({ {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR} })
        .SetRasterizationState(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE)
        .SetPipelineLayout({ mPlayerDescriptorSetLayout, mDescriptorSetLayout }, pushConstants)
        .SetSampling(VK_SAMPLE_COUNT_1_BIT)
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .AddBlendAttachmentState()
        .SetDepthState(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL)
        .SetRenderPass(m_RenderPass)
        .Build();

    m_Pipeline = skyboxPiplineRes.first;
    m_PipelineLayout = skyboxPiplineRes.second;
}

void Skybox::BuildDescriptorSetLayouts() {
    // Build player descriptor set layout
    {
        // Set = 0, binding 0 = cameraUBO, binding = 1 = textures
        std::vector<VkDescriptorSetLayoutBinding> bindings = {vkutil::CreateDescriptorBinding(
            0, 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)};

        mPlayerDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }

    // Build descriptor set layout
    {
        std::vector<VkDescriptorSetLayoutBinding> bindings = {vkutil::CreateDescriptorBinding(
            0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)};

        mDescriptorSetLayout = vkutil::CreateDescriptorSetLayout(context, bindings);
    }
}

void Skybox::BuildDescriptors()
{
    for (size_t playerId = 0; playerId < GlobalConfig::maxPlayers; ++playerId) {
        auto &descriptorSets = mPlayerDescriptorSets[playerId];

        descriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mPlayerDescriptorSetLayout,
                                       vkutil::MAX_FRAMES_IN_FLIGHT, descriptorSets);

        // Camera Transform UBO
        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m_Scene->GetCameraBuffers(playerId)[i].buffer;
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(CameraTransform);
            vkutil::UpdateDescriptorSet(context, 0, bufferInfo, descriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        }
    }

    {
        mDescriptorSets.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
        vkutil::AllocateDescriptorSets(context, context.descriptorPool, mDescriptorSetLayout,
                                       vkutil::MAX_FRAMES_IN_FLIGHT, mDescriptorSets);

        for (size_t i = 0; i < vkutil::MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorImageInfo imageInfo = {.sampler = vkutil::clampToEdgeSamplerAniso,
                                               .imageView = m_Skybox.imageView,
                                               .imageLayout =
                                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL};

            vkutil::UpdateDescriptorSet(context, 0, imageInfo, mDescriptorSets[i],
                                        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
    }
}

void Skybox::LoadCubemapFace(std::filesystem::path facePath, float** pixelData)
{
    int w, h, texChannels;
    stbi_set_flip_vertically_on_load(0);
    //stbi_uc* pixels = stbi_load(facePath.string().c_str(), &w, &h, &texChannels, 4);
    float *data = stbi_loadf(facePath.string().c_str(), &w, &h, &texChannels, STBI_rgb_alpha);

    const uint32_t width = static_cast<uint32_t>(w);
    const uint32_t height = static_cast<uint32_t>(h);

    if (!data)
    {
        // TODO: change this to the engine logging library instead but idk how to use it yet
        throw std::runtime_error("Failed to load skybox face: " + facePath.string());
    }

    *pixelData = (data);
}

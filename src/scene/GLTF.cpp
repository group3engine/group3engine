#include "Context.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#include <glm/gtc/type_ptr.hpp>

#include <spdlog/spdlog.h>

#include "GLTF.hpp"
#include "Utils.hpp"

inline void CGLTFCheck(cgltf_result result, std::string_view msg) {
    if (result != cgltf_result_success) {
        SPDLOG_ERROR(msg);
        SPDLOG_ERROR("cgltf_result={}", static_cast<int>(result));
        std::exit(EXIT_FAILURE);
    }
}

vk::GLTFModel vk::LoadGLTF(const Context& context, std::filesystem::path filepath)
{
    // Convert directory separators to preferred directory separator
    // Slight try at cross-platform for Windows
    filepath.make_preferred();

    //vk::GLTFModel model = {};
    vk::GLTFModel model(context);

    cgltf_options options = {};
    cgltf_data* data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filepath.c_str(), &data);
    CGLTFCheck(result, "Failed to parse glTF: " + filepath.string());

    SPDLOG_DEBUG("Material count: {}", data->materials_count);

    //Each mesh primitive should have a material index
    result = cgltf_load_buffers(&options, data, filepath.c_str());
    CGLTFCheck(result, "Failed to load buffers of glTF: " + filepath.string());

    result = cgltf_validate(data);
    CGLTFCheck(result, "Failed to validate glTF: " + filepath.string());

    // get the number of mesh primitives
    size_t meshPrimitivesCount = 0;
    for (size_t mi = 0; mi < data->meshes_count; ++mi) {
        meshPrimitivesCount += data->meshes[mi].primitives_count;
    }

    for (size_t mi = 0; mi < data->meshes_count; ++mi) {
        cgltf_mesh& gltfMesh = data->meshes[mi];
        Mesh mesh{ .name = "", .meshPrimitives = {} };
        if (gltfMesh.name) {
            mesh.name = gltfMesh.name;
        }

        std::vector<MeshData> meshDatas;

        // Mesh has primities. Primitives is just positions, tex, normals which make up the mesh 
        for (size_t pi = 0; pi < gltfMesh.primitives_count; ++pi) {
            
            const auto& gltfPrimitive = gltfMesh.primitives[pi];
            MeshPrimitive meshPrimitive{};
            // Mesh primitive is basically a mesh e.g. curtain in sponza 
            // Positions
            if (const cgltf_accessor* pos =
                cgltf_find_accessor(&gltfPrimitive, cgltf_attribute_type_position, 0)) {
                size_t count = pos->count * 3;
                meshPrimitive.positions.resize(count);
                assert(cgltf_num_components(pos->type) == 3);
                cgltf_accessor_unpack_floats(pos, meshPrimitive.positions.data(), count);
            }

            // Normals
            if (const cgltf_accessor* nrm =
                cgltf_find_accessor(&gltfPrimitive, cgltf_attribute_type_normal, 0)) {
                size_t count = nrm->count * 3;
                meshPrimitive.normals.resize(count);
                assert(cgltf_num_components(nrm->type) == 3);
                cgltf_accessor_unpack_floats(nrm, meshPrimitive.normals.data(), count);
            }

            // Texcoords
            if (const cgltf_accessor* tex =
                cgltf_find_accessor(&gltfPrimitive, cgltf_attribute_type_texcoord, 0)) {
                size_t count = tex->count * 2;
                meshPrimitive.texcoords.resize(count);
                assert(cgltf_num_components(tex->type) == 2);
                cgltf_accessor_unpack_floats(tex, meshPrimitive.texcoords.data(), count);
            }

            MeshData meshData(context);

            // Indices
            meshData.indices.resize(gltfPrimitive.indices->count);
            cgltf_accessor_unpack_indices(gltfPrimitive.indices, meshData.indices.data(),
                sizeof(uint32_t), meshData.indices.size());

            for (size_t i = 0; i < meshPrimitive.positions.size(); i += 3)
            {
                Vertex vertex = {};
                vertex.pos = { meshPrimitive.positions[i], meshPrimitive.positions[i + 1], meshPrimitive.positions[i + 2] };
                vertex.normal = { meshPrimitive.normals[i], meshPrimitive.normals[i + 1], meshPrimitive.normals[i + 2] };

                // tex coord index
                size_t texIndex = (i / 3) * 2;
                if (texIndex + 1 < meshPrimitive.texcoords.size()) {
                    vertex.tex = { meshPrimitive.texcoords[texIndex], meshPrimitive.texcoords[texIndex + 1] };
                }
                else {
                    vertex.tex = { 0.0f, 0.0f }; 
                }

                meshData.vertices.push_back(vertex);
            }

            // Each material has a descriptor set
            // If each material is stored in array and the mesh has a material index
            // We would just need to index into the material array, 
            // bind the descriptor set for that material 
            // perhaps use set = 1 for materials

            // Material 
            cgltf_material* material = gltfPrimitive.material;
            // std::printf("Diff: %s\n", material->pbr_metallic_roughness.base_color_texture.texture->image->uri);
            // find index
            int matIndex = -1;
            for (size_t i = 0; i < data->materials_count; i++)
            {
                if (&data->materials[i] == material)
                {
                    matIndex = static_cast<uint32_t>(i);
                    break;
                }
            }

            // Get the texture paths for this meshes material  

            // need to get other textures and multipliers
            const cgltf_pbr_metallic_roughness& pbr = material->pbr_metallic_roughness;

            std::filesystem::path assetsPath = std::filesystem::current_path() / "assets";

            if (pbr.base_color_texture.texture) {
                std::filesystem::path albedoPath = filepath.parent_path() / pbr.base_color_texture.texture->image->uri;
                meshData.textures.push_back(albedoPath.make_preferred());
            }
            else
            {
                std::string defaultAlbedo = "white_pixel.png";
                std::filesystem::path albedoPath = assetsPath / defaultAlbedo;
                meshData.textures.push_back(albedoPath.make_preferred());
            }

            std::filesystem::path metallicRoughness;
            if (pbr.metallic_roughness_texture.texture == NULL) {
                std::string defaultRoughness = "white_pixel.png";
                metallicRoughness = assetsPath / defaultRoughness;
            }
            else
            {
                metallicRoughness = filepath.parent_path() / material->pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri;
            }

            meshData.textures.push_back(metallicRoughness.make_preferred());

            meshData.materialIndex = matIndex;
            model.name = filepath.string();
            meshDatas.push_back(std::move(meshData));
        }

        model.meshes.push_back(std::move(meshDatas));
    }

    model.entities.reserve(data->nodes_count);
    for (cgltf_size ni = 0; ni < data->nodes_count; ++ni) {
        const auto &gltf_node = data->nodes[ni];

        Entity entity{};

        float m[16];
        cgltf_node_transform_world(&gltf_node, m);

        entity.modelMatrix = glm::make_mat4x4(m);

        if (gltf_node.mesh) {
            ptrdiff_t meshIndex = gltf_node.mesh - data->meshes;
            assert(meshIndex >= 0  && meshIndex < data->nodes_count);

            entity.meshData = &model.meshes[meshIndex];
        }

        model.entities.push_back(entity);
    }

    cgltf_free(data);

    return (model);
}


// ======================== Material ======================== 
vk::Material::Material(vk::Context& context) : context{ context }, isValid{ false } {}


void vk::Material::Destroy()
{
    for (auto& img : textures)
    {
        img.Destroy(context.device);
    }
}

// ======================== Material Manager ========================
void vk::MaterialManager::Setup(Context& context)
{
    CreateDescriptorLayout(context);
}

void vk::MaterialManager::Destroy(Context& context)
{
    for (auto& material : materials)
    {
        material.Destroy();
    }

    vkDestroyDescriptorSetLayout(context.device, vk::materialDescriptorSetLayout, nullptr);
}

void vk::MaterialManager::CreateDescriptorLayout(Context& context)
{
    std::vector<VkDescriptorSetLayoutBinding> bindings = {
        CreateDescriptorBinding(0, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT),
        CreateDescriptorBinding(1, 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
    };

    vk::materialDescriptorSetLayout = CreateDescriptorSetLayout(context, bindings);
}

void vk::MaterialManager::BuildMaterials(Context& context)
{
    materialDescriptorSets.resize(materials.size());

    for (size_t i = 0; i < materials.size(); i++)
    {
        AllocateDescriptorSet(context, context.descriptorPool, vk::materialDescriptorSetLayout, 1, materialDescriptorSets[i]);

        // For all textures this current material needs to point to 
        // For each img in textures, create a descriptor image info for the descriptor to point to 
        for (size_t img = 0; img < materials[i].textures.size(); img++) {

            VkDescriptorImageInfo imageInfo = {
                .sampler = repeatSamplerAniso,
                .imageView = materials[i].textures[img].imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            UpdateDescriptorSet(context, static_cast<uint32_t>(img), imageInfo, materialDescriptorSets[i], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
        }
    }
}

void vk::MaterialManager::LoadTexturesForMaterial(uint32_t matIndex, const MeshData& mesh, vk::Context& context)
{
    materials[matIndex].textures.resize(mesh.textures.size());

    for (size_t i = 0; i < mesh.textures.size(); i++)
    {
        VkFormat FORMAT = (i == 0) ? VK_FORMAT_R8G8B8A8_SRGB : VK_FORMAT_R8G8B8A8_UNORM;
        materials[matIndex].textures[i] = std::move(LoadTextureFromDisk(mesh.textures[i], context, FORMAT));
    }
}

// ======================== Mesh Data ========================
vk::MeshData::MeshData(const Context& context) : context{ context } {};

vk::MeshData::~MeshData()
{
    vertexBuffer.Destroy();
    indexBuffer.Destroy();
}

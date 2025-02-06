#include "cgltf.h"
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <stb_image.h>

#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"

struct MeshPrimitive {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texcoords;
    // TODO: Bone weights
    std::vector<uint32_t> indices;
};

struct Mesh {
    std::string name;
    std::vector<MeshPrimitive> meshPrimitives;
};

struct Texture {
    
};

struct Image {
    stbi_uc *data;
};

enum class AlphaMode {
    eOpaque,
    eMask,
    eBlend
};

struct PBRMetallicRoughnessMaterial {
    Texture *baseColorTexture;
    Texture *metallicRoughnessTexture;

    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;
};

struct Material {
    std::string name;
    bool hasPBRMetallicRoughness;
    PBRMetallicRoughnessMaterial pbrMetallicRoughness;
};

Material LoadMaterialDefault() {
    PBRMetallicRoughnessMaterial pbrMetallicRoughness{
        .baseColorFactor = {1, 1, 1, 1},
        .baseColorTexture = nullptr,
        .metallicFactor = 0.0f,
        .roughnessFactor = 1.0f,
        .metallicRoughnessTexture = nullptr
    };

    return {.name = "default",
            .hasPBRMetallicRoughness = true,
            .pbrMetallicRoughness = pbrMetallicRoughness};
}

Image LoadCGLTFImage(const cgltf_image* image,  std::string filePath) {
    // flip images vertically by default
    stbi_set_flip_vertically_on_load(1);

    // Construct texture file path
    std::string textureFilePath = filePath + std::string(image->uri);

    // load base image
    int baseWidthi, baseHeighti, baseChannelsi;
    stbi_uc *data =
        stbi_load(textureFilePath.c_str(), &baseWidthi, &baseHeighti, &baseChannelsi, 4 /*RGBA*/);
    if (!data) {
        std::printf("Can't load image '%s'\n", textureFilePath.c_str());
    }

    auto const baseWidth = std::uint32_t(baseWidthi);
    auto const baseHeight = std::uint32_t(baseHeighti);

    // create staging buffer and copy image data to it
    auto const sizeInBytes = baseWidth * baseHeight * 4;

    return {data};
}

int main() {
    cgltf_options options = {};
    cgltf_data *data = nullptr;
    std::string aFilepath = "../assets/BoxTextured.gltf";
    cgltf_result result = cgltf_parse_file(&options, aFilepath.c_str(), &data);
    if (result != cgltf_result_success) {
        std::cout << "Failed to parse file.\n";
        return 0;
    }

    size_t total_primitives = 0;

    for (size_t mi = 0; mi < data->meshes_count; ++mi)
        total_primitives += data->meshes[mi].primitives_count;

    std::cout << "data->meshes_count=" << data->meshes_count << '\n';
    std::cout << "data->materials_count=" << data->materials_count << '\n';
    std::cout << "data->buffers_count=" << data->buffers_count << '\n';
    std::cout << "data->images_count=" << data->images_count << '\n';
    std::cout << "data->textures_count=" << data->textures_count << '\n';
    std::cout << "total_primitives=" << total_primitives << '\n';

    result = cgltf_load_buffers(&options, data, aFilepath.c_str());
    if (result != cgltf_result_success) {
        std::cout << "Failed to load buffers file.\n";
    }

    // NOTE: optional
    result = cgltf_validate(data);
    if (result != cgltf_result_success) {
        std::cout << "Parsed glTF not valid\n";
    }

    // Based on https://github.com/zeux/niagara/blob/master/src/scene.cpp
    std::vector<Mesh> meshes;
    meshes.reserve(data->meshes_count);
    for (size_t mi = 0; mi < data->meshes_count; ++mi) {
        const auto &gltfMesh = data->meshes[mi];
        Mesh mesh{.name = gltfMesh.name, .meshPrimitives = {}};

        for (size_t pi = 0; pi < gltfMesh.primitives_count; ++pi) {
            const auto &gltfPrimitive = gltfMesh.primitives[pi];
            MeshPrimitive meshPrimitive{};

            // Positions
            if (const cgltf_accessor *pos =
                    cgltf_find_accessor(&gltfPrimitive, cgltf_attribute_type_position, 0)) {
                size_t count = pos->count * 3;
                meshPrimitive.positions.resize(count);
                assert(cgltf_num_components(pos->type) == 3);
                cgltf_accessor_unpack_floats(pos, meshPrimitive.positions.data(), count);
            }

            // Normals
            if (const cgltf_accessor *nrm =
                    cgltf_find_accessor(&gltfPrimitive, cgltf_attribute_type_normal, 0)) {
                size_t count = nrm->count * 3;
                meshPrimitive.normals.resize(count);
                assert(cgltf_num_components(nrm->type) == 3);
                cgltf_accessor_unpack_floats(nrm, meshPrimitive.normals.data(), count);
            }

            // Texcoords
            if (const cgltf_accessor *tex =
                    cgltf_find_accessor(&gltfPrimitive, cgltf_attribute_type_texcoord, 0)) {
                size_t count = tex->count * 2;
                meshPrimitive.texcoords.resize(count);
                assert(cgltf_num_components(tex->type) == 2);
                cgltf_accessor_unpack_floats(tex, meshPrimitive.texcoords.data(), count);
            }

            // Indices
            meshPrimitive.indices.resize(gltfPrimitive.indices->count);
            cgltf_accessor_unpack_indices(gltfPrimitive.indices, meshPrimitive.indices.data(),
                                          sizeof(uint32_t), meshPrimitive.indices.size());

            mesh.meshPrimitives.push_back(meshPrimitive);
        }

        meshes.push_back(mesh);
    }

    std::vector<Material> materials;
    size_t gltfMaterialsCount = data->materials_count;
    size_t defaultMaterialsCount = 1;
    size_t materialsCount = gltfMaterialsCount + defaultMaterialsCount;
    materials.reserve(materialsCount);

    materials.push_back(LoadMaterialDefault());

    for (size_t i = 0; i < gltfMaterialsCount; ++i) {
        Material material = LoadMaterialDefault();

        material.name = data->materials[i].name;

        if (data->materials[i].has_pbr_metallic_roughness) {
            material.hasPBRMetallicRoughness = true;

            // Load base color texture (albedo)
            if (data->materials[i].pbr_metallic_roughness.base_color_texture.texture) {
                Image imAlbedo = LoadCGLTFImage(
                    data->materials[i].pbr_metallic_roughness.base_color_texture.texture->image,
                    "../assets/");

                // TODO: Load to GPU
                // TODO: Init material.pbrMetallicRougness here
                // if (imAlbedo.data != NULL) {
                //     model.materials[j].maps[MATERIAL_MAP_ALBEDO].texture =
                //         LoadTextureFromImage(imAlbedo);
                //     UnloadImage(imAlbedo);
                // }

                free(imAlbedo.data);
            }

            material.pbrMetallicRoughness.baseColorFactor =
                glm::vec4(data->materials[i].pbr_metallic_roughness.base_color_factor[0],
                          data->materials[i].pbr_metallic_roughness.base_color_factor[1],
                          data->materials[i].pbr_metallic_roughness.base_color_factor[2],
                          data->materials[i].pbr_metallic_roughness.base_color_factor[3]);

            // Load metallic/roughness material properties
            float roughness = data->materials[i].pbr_metallic_roughness.roughness_factor;
            material.pbrMetallicRoughness.roughnessFactor = roughness;

            float metallic = data->materials[i].pbr_metallic_roughness.metallic_factor;
            material.pbrMetallicRoughness.metallicFactor = metallic;
        }

        materials.push_back(material);
    }

    std::cout << "materials.size()=" << materials.size() << '\n';

    const auto &material = materials[1];
    std::cout << "material.name=" << material.name << '\n';
    std::cout << "material.hasPBRMetallicRoughness=" << material.hasPBRMetallicRoughness << '\n';
    std::cout << "material.pbrMetallicRoughness.baseColorFactor=" << glm::to_string(material.pbrMetallicRoughness.baseColorFactor) << '\n';
    std::cout << "material.pbrMetallicRoughness.metallicFactor=" << material.pbrMetallicRoughness.metallicFactor << '\n';
    std::cout << "material.pbrMetallicRoughness.roughnessFactor=" << material.pbrMetallicRoughness.roughnessFactor << '\n';

    std::cout << "meshes.size()=" << meshes.size() << '\n';
    std::cout << "meshPrimitives.size()=" << meshes[0].meshPrimitives.size() << '\n';
    std::cout << "meshPrimitives.positions.size()=" << meshes[0].meshPrimitives[0].positions.size() << '\n';
    std::cout << "meshPrimitives.normals.size()=" << meshes[0].meshPrimitives[0].normals.size() << '\n';
    std::cout << "meshPrimitives.texcoords.size()=" << meshes[0].meshPrimitives[0].texcoords.size() << '\n';
    std::cout << "meshPrimitives.indices.size()=" << meshes[0].meshPrimitives[0].indices.size() << '\n';
    std::cout << *std::max_element(meshes[0].meshPrimitives[0].indices.begin(), meshes[0].meshPrimitives[0].indices.end()) << '\n';

    cgltf_free(data);

    std::cout << "Hello world!" << '\n';
    return 0;
}

//
// Created by thomas on 05/02/25.
//

#include "ResourceManager.hpp"

#include <iostream>
#include <string>

#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "Entity.hpp"
#include "cgltf_write.h"
#include "glm/gtc/type_ptr.hpp"

#include <stb_image.h>

Material LoadMaterialDefault() {
    PBRMetallicRoughnessMaterial pbrMetallicRoughness{.baseColorTexture = nullptr,
                                                      .metallicRoughnessTexture = nullptr,
                                                      .baseColorFactor = {1, 1, 1, 1},
                                                      .metallicFactor = 0.0f,
                                                      .roughnessFactor = 1.0f};

    return {.name = "default",
            .hasPBRMetallicRoughness = true,
            .pbrMetallicRoughness = pbrMetallicRoughness};
}

Image LoadCGLTFImage(const std::string& texturePath) {
    // flip images vertically by default
    stbi_set_flip_vertically_on_load(1);

    // load base image
    int baseWidthi, baseHeighti, baseChannelsi;
    stbi_uc *data =
        stbi_load(texturePath.c_str(), &baseWidthi, &baseHeighti, &baseChannelsi, 4 /*RGBA*/);


    if (!data) {
        std::printf("Can't load image '%s'\n", texturePath.c_str());
        assert(data);
    }

    auto const baseWidth = std::uint32_t(baseWidthi);
    auto const baseHeight = std::uint32_t(baseHeighti);

    // create staging buffer and copy image data to it
    auto const sizeInBytes = baseWidth * baseHeight * 4;

    return {data, baseWidth, baseHeight, sizeInBytes};
}

int LoadGLTF(std::string aFilepath, MeshManager &aMeshManager, MaterialManager &aMaterialManager,
             TextureManager &aTextureManager, VkPipelineLayout aPipeLayout, std::vector<Entity>& aEntities, bool aIsDebug) {
    cgltf_options options = {};
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, aFilepath.c_str(), &data);
    if (result != cgltf_result_success) {
        std::cout << "Failed to parse file.\n";
        return GLTF_LOAD_FAIL;
    }
    if (aIsDebug) {
        size_t total_primitives = 0;

        for (size_t mi = 0; mi < data->meshes_count; ++mi)
            total_primitives += data->meshes[mi].primitives_count;

        std::cout << "data->meshes_count=" << data->meshes_count << '\n';
        std::cout << "data->materials_count=" << data->materials_count << '\n';
        std::cout << "data->buffers_count=" << data->buffers_count << '\n';
        std::cout << "data->images_count=" << data->images_count << '\n';
        std::cout << "data->textures_count=" << data->textures_count << '\n';
        std::cout << "total_primitives=" << total_primitives << '\n';
    }
    result = cgltf_load_buffers(&options, data, aFilepath.c_str());
    if (result != cgltf_result_success) {
        std::cout << "Failed to load buffers file.\n";
        return GLTF_LOAD_FAIL;
    }
    // NOTE: optional
    result = cgltf_validate(data);
    if (result != cgltf_result_success) {
        std::cout << "Parsed glTF not valid\n";
        return GLTF_LOAD_FAIL;
    }
    // Based on https://github.com/zeux/niagara/blob/master/src/scene.cpp
    // load the meshes

    aMeshManager.reserveMeshes(data->meshes_count);
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

        aMeshManager.addMesh(mesh);
    }

    size_t gltfMaterialsCount = data->materials_count;
    size_t defaultMaterialsCount = 1;
    size_t materialsCount = gltfMaterialsCount + defaultMaterialsCount;
    aMaterialManager.ReserveMaterials(materialsCount);

    aMaterialManager.AddMaterial(LoadMaterialDefault());

    for (size_t i = 0; i < gltfMaterialsCount; ++i) {
        Material material = LoadMaterialDefault();

        material.name = data->materials[i].name;

        // PBR Metallic Roughness
        if (data->materials[i].has_pbr_metallic_roughness) {
            material.hasPBRMetallicRoughness = true;
            std::string textureFolders = aFilepath.substr(0, aFilepath.find_last_of("/") + 1);
            // Load base color texture (albedo)
            if (data->materials[i].pbr_metallic_roughness.base_color_texture.texture) {
                std::string texturePath =
                    textureFolders +
                    data->materials[i]
                        .pbr_metallic_roughness.base_color_texture.texture->image->uri;
                Image imAlbedo = LoadCGLTFImage(texturePath);

                aTextureManager.addTexture(texturePath, imAlbedo);

                material.pbrMetallicRoughness.baseColorTexture =
                    aTextureManager.GetTexture(texturePath);

                free(imAlbedo.data);
            }
            // load metallic roughness texture
            if (data->materials[i].pbr_metallic_roughness.metallic_roughness_texture.texture) {
                std::string texturePath =
                    textureFolders +
                    data->materials[i]
                        .pbr_metallic_roughness.metallic_roughness_texture.texture->image->uri;
                Image imMetallicRoughness = LoadCGLTFImage(texturePath);

                aTextureManager.addTexture(texturePath, imMetallicRoughness);

                material.pbrMetallicRoughness.metallicRoughnessTexture =
                    aTextureManager.GetTexture(texturePath);

                free(imMetallicRoughness.data);
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
        // TODO: create material descriptor set
        aMaterialManager.AddMaterial(material);
    }

    // set up entities
    // for each node
    for (size_t ni = 0; ni < data->nodes_count; ni++) {
        const auto &gltfNode = data->nodes[ni];
        // add an entity
        aEntities.push_back(Entity(aPipeLayout));
        Entity &entity = aEntities.back();
        // set the name
        if(gltfNode.name) {
            entity.SetName(gltfNode.name);
        }
        // get the transform
        if (gltfNode.has_matrix) {
            entity.SetTransform(glm::make_mat4(gltfNode.matrix));
        } else {
            glm::vec3 translation, scale;
            glm::quat rotation;
            if (gltfNode.has_translation) {
                translation = {gltfNode.translation[0], gltfNode.translation[1],
                               gltfNode.translation[2]};
            } else {
                translation = {0, 0, 0};
            }
            if (gltfNode.has_rotation) {
                rotation = {gltfNode.rotation[0], gltfNode.rotation[1], gltfNode.rotation[2],
                            gltfNode.rotation[3]};
            } else {
                rotation = {0, 0, 0, 1};
            }
            if (gltfNode.has_scale) {
                scale = {gltfNode.scale[0], gltfNode.scale[1], gltfNode.scale[2]};
            } else {
                scale = {1, 1, 1};
            }
            Transform transform = {
                .translation = translation, .rotation = rotation, .scale = scale};
            entity.SetTransform(transform);
        }
        // add the mesh
        if (gltfNode.mesh) {
            // get the index of the mesh
            int meshIndex = static_cast<int>(gltfNode.mesh - data->meshes);
            entity.AddMesh(aMeshManager.getMesh(meshIndex));
        }
    }
    // go through each entity and add its parent
    for (size_t ni = 0; ni < data->nodes_count; ni++) {
        const auto &gltfNode = data->nodes[ni];
        if (gltfNode.parent) {
            int parentIndex = static_cast<int>(gltfNode.parent - data->nodes);
            assert(parentIndex < (int)data->nodes_count);
            aEntities[ni].SetParent(&aEntities[parentIndex]);
        }
    }

    if (aIsDebug) {
        aMeshManager.debugOuptutMeshes();
        aMaterialManager.DebugOutputMeshes();
    }

    cgltf_free(data);
    return GLTF_LOAD_SUCCESS;
}

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

namespace vk {

Material LoadMaterialDefault() {
    PBRMetallicRoughnessMaterial pbrMetallicRoughness{
        nullptr, nullptr, {{1, 1, 1, 1}, 0.0f, 1.0f, 0.0f}, "", ""};
    Material material;
    material.name = "default";
    material.hasPBRMetallicRoughness = true;
    material.pbrMetallicRoughness = pbrMetallicRoughness;
    material.descriptorSet = VK_NULL_HANDLE;
    material.alphaCutout = false;
    return material;
}

std::string DecodeURI(std::string_view uri, std::string_view gltfPath) {
    std::vector<char> path(strlen(uri.data()) + strlen(gltfPath.data()) + 1);

    cgltf_combine_paths(path.data(), gltfPath.data(), uri.data());

    // after combining, the tail of the resulting path is a uri; decode_uri
    // converts it into path
    cgltf_decode_uri(path.data() + strlen(path.data()) - strlen(uri.data()));

    return {path.data()};
}

int LoadGLTF(std::filesystem::path aFilepath, MeshManager &aMeshManager,
             MaterialManager &aMaterialManager, TextureManager &aTextureManager,
             std::vector<Entity> &aEntities, bool aIsDebug) {
    // Convert directory separators to preferred directory separator
    // Slight try at cross-platform for Windows
    aFilepath.make_preferred();

    cgltf_options options = {};
    cgltf_data *data = nullptr;
    cgltf_result result =
        cgltf_parse_file(&options, aFilepath.string().c_str(), &data);
    if (result != cgltf_result_success) {
        std::cout << "Failed to parse file.\n";
        std::exit(EXIT_FAILURE);
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
    result = cgltf_load_buffers(&options, data, aFilepath.string().c_str());
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
    size_t gltfMaterialsCount = data->materials_count;
    size_t defaultMaterialsCount = 1;
    size_t materialsCount = gltfMaterialsCount + defaultMaterialsCount;
    aMaterialManager.ReserveMaterials(materialsCount);
    Material defaultMaterial = LoadMaterialDefault();
    defaultMaterial.pbrMetallicRoughness.baseColorTexture =
        aTextureManager.GetTexture("white");
    defaultMaterial.pbrMetallicRoughness.metallicRoughnessTexture =
        aTextureManager.GetTexture("white");
    defaultMaterial.name = "default";
    defaultMaterial.pbrMetallicRoughness.baseColorTextureName = "white";
    defaultMaterial.pbrMetallicRoughness.metallicRoughnessTextureName = "white";

    aMaterialManager.AddMaterial(defaultMaterial);

    // Load materials
    for (size_t i = 0; i < gltfMaterialsCount; ++i) {
        const auto &gltfMaterial = data->materials[i];

        Material material = LoadMaterialDefault();
        if (gltfMaterial.name)
            material.name = gltfMaterial.name;

        // PBR Metallic Roughness
        if (gltfMaterial.has_pbr_metallic_roughness) {
            material.hasPBRMetallicRoughness = true;

            // Load base color texture
            if (gltfMaterial.pbr_metallic_roughness.base_color_texture
                    .texture) {
                std::string imageBaseColorFileName =
                    gltfMaterial.pbr_metallic_roughness.base_color_texture
                        .texture->image->uri;
                std::string imageBaseColorName =
                    DecodeURI(imageBaseColorFileName, aFilepath.string());

                aTextureManager.addTexture(imageBaseColorName,
                                           imageBaseColorFileName);
                material.pbrMetallicRoughness.baseColorTexture =
                    aTextureManager.GetTexture(imageBaseColorFileName);
                material.pbrMetallicRoughness.baseColorTextureName =
                    imageBaseColorFileName;

            } else {
                material.pbrMetallicRoughness.baseColorTexture =
                    aTextureManager.GetTexture("white");
                material.pbrMetallicRoughness.baseColorTextureName = "white";
            }
            if (gltfMaterial.alpha_mode == cgltf_alpha_mode_mask) {
                material.alphaCutout = true;
                material.pbrMetallicRoughness.pbrMaterialNumbers.alphaCutoff =
                    gltfMaterial.alpha_cutoff;
            } else if (gltfMaterial.alpha_mode == cgltf_alpha_mode_blend) {
                material.hasPBRMetallicRoughness = false;
            } else {
                material.alphaCutout = false;
            }

            // Load metallic roughness texture
            if (gltfMaterial.pbr_metallic_roughness.metallic_roughness_texture
                    .texture) {
                std::string imageMetallicRoughnessFileName =
                    gltfMaterial.pbr_metallic_roughness
                        .metallic_roughness_texture.texture->image->uri;
                std::string imageMetallicRoughnessName = DecodeURI(
                    imageMetallicRoughnessFileName, aFilepath.string());

                aTextureManager.addTexture(imageMetallicRoughnessName,
                                           imageMetallicRoughnessFileName);
                material.pbrMetallicRoughness.metallicRoughnessTexture =
                    aTextureManager.GetTexture(imageMetallicRoughnessFileName);

                material.pbrMetallicRoughness.metallicRoughnessTextureName =
                    imageMetallicRoughnessFileName;

            } else {
                material.pbrMetallicRoughness.metallicRoughnessTexture =
                    aTextureManager.GetTexture("white");

                material.pbrMetallicRoughness.metallicRoughnessTextureName =
                    "white";
            }

            material.pbrMetallicRoughness.pbrMaterialNumbers.baseColorFactor =
                glm::vec4(
                    gltfMaterial.pbr_metallic_roughness.base_color_factor[0],
                    gltfMaterial.pbr_metallic_roughness.base_color_factor[1],
                    gltfMaterial.pbr_metallic_roughness.base_color_factor[2],
                    gltfMaterial.pbr_metallic_roughness.base_color_factor[3]);

            material.pbrMetallicRoughness.pbrMaterialNumbers.metallicFactor =
                gltfMaterial.pbr_metallic_roughness.metallic_factor;

            material.pbrMetallicRoughness.pbrMaterialNumbers.roughnessFactor =
                gltfMaterial.pbr_metallic_roughness.roughness_factor;
        } else {
            material.hasPBRMetallicRoughness = false;
        }
        // TODO: create material descriptor set
        aMaterialManager.AddMaterial(material);
    }

    // load the meshes

    // get the number of mesh primitives
    size_t meshPrimitivesCount = 0;
    for (size_t mi = 0; mi < data->meshes_count; ++mi) {
        meshPrimitivesCount += data->meshes[mi].primitives_count;
    }

    aMeshManager.reserveMeshes(data->meshes_count, meshPrimitivesCount);
    for (size_t mi = 0; mi < data->meshes_count; ++mi) {
        const auto &gltfMesh = data->meshes[mi];
        Mesh mesh{.name = "", .meshPrimitives = {}};
        if (gltfMesh.name) {
            mesh.name = gltfMesh.name;
        }

        for (size_t pi = 0; pi < gltfMesh.primitives_count; ++pi) {
            const auto &gltfPrimitive = gltfMesh.primitives[pi];
            MeshPrimitive meshPrimitive{};
            // temporary positions, normals, texcoords
            std::vector<float> positions;
            std::vector<float> normals;
            std::vector<float> texcoords;
            std::vector<float> joints;
            std::vector<float> weights;

            // Positions
            if (const cgltf_accessor *pos = cgltf_find_accessor(
                    &gltfPrimitive, cgltf_attribute_type_position, 0)) {
                size_t count = pos->count * 3;
                positions.resize(count);
                assert(cgltf_num_components(pos->type) == 3);
                cgltf_accessor_unpack_floats(pos, positions.data(), count);
            }

            // Normals
            if (const cgltf_accessor *nrm = cgltf_find_accessor(
                    &gltfPrimitive, cgltf_attribute_type_normal, 0)) {
                size_t count = nrm->count * 3;
                normals.resize(count);
                assert(cgltf_num_components(nrm->type) == 3);
                cgltf_accessor_unpack_floats(nrm, normals.data(), count);
            }

            // Texcoords
            if (const cgltf_accessor *tex = cgltf_find_accessor(
                    &gltfPrimitive, cgltf_attribute_type_texcoord, 0)) {
                size_t count = tex->count * 2;
                texcoords.resize(count);
                assert(cgltf_num_components(tex->type) == 2);
                cgltf_accessor_unpack_floats(tex, texcoords.data(), count);
            } else {
                texcoords.resize(positions.size() / 3 * 2);
                for (size_t i = 0; i < texcoords.size(); i += 2) {
                    texcoords[i] = 0.0f;
                    texcoords[i + 1] = 0.0f;
                }
            }
            // joints
            if (const cgltf_accessor *jointsAccessor = cgltf_find_accessor(
                    &gltfPrimitive, cgltf_attribute_type_joints, 0)) {
                size_t count = jointsAccessor->count * 4;
                joints.resize(count);
                assert(cgltf_num_components(jointsAccessor->type) == 4);
                cgltf_accessor_unpack_floats(jointsAccessor, joints.data(),
                                             count);
            }
            // weights
            if (const cgltf_accessor *weightsAccessor = cgltf_find_accessor(
                    &gltfPrimitive, cgltf_attribute_type_weights, 0)) {
                size_t count = weightsAccessor->count * 4;
                weights.resize(count);
                assert(cgltf_num_components(weightsAccessor->type) == 4);
                cgltf_accessor_unpack_floats(weightsAccessor, weights.data(),
                                             count);
            }

            // load up the vertices
            meshPrimitive.vertices.resize(positions.size() / 3);
            for (size_t i = 0; i < meshPrimitive.vertices.size(); i++) {
                meshPrimitive.vertices[i].pos = {positions[i * 3],
                                                 positions[i * 3 + 1],
                                                 positions[i * 3 + 2]};
                meshPrimitive.vertices[i].normal = {
                    normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]};
                meshPrimitive.vertices[i].tex = {texcoords[i * 2],
                                                 texcoords[i * 2 + 1]};
            }

            // Indices
            meshPrimitive.indices.resize(gltfPrimitive.indices->count);
            cgltf_accessor_unpack_indices(
                gltfPrimitive.indices, meshPrimitive.indices.data(),
                sizeof(uint32_t), meshPrimitive.indices.size());

            // get the material index
            int materialIndex =
                static_cast<int>(gltfPrimitive.material - data->materials) +
                static_cast<int>(defaultMaterialsCount);
            meshPrimitive.material =
                aMaterialManager.GetMaterial(materialIndex);
            // if the material doesn't have pbrMetallicRoughness, skip the mesh
            // primitive
            if (!meshPrimitive.material->hasPBRMetallicRoughness) {
                continue;
            }

            mesh.meshPrimitives.push_back(meshPrimitive);
        }

        aMeshManager.addMesh(mesh);
    }

    // set up entities
    // for each node
    for (size_t ni = 0; ni < data->nodes_count; ni++) {
        const auto &gltfNode = data->nodes[ni];

        // add an entity
        aEntities.emplace_back();
        Entity &entity = aEntities.back();
        // set the name
        if (gltfNode.name) {
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
                rotation = {gltfNode.rotation[3], gltfNode.rotation[0],
                            gltfNode.rotation[1], gltfNode.rotation[2]};
            } else {
                rotation = {1, 0, 0, 0};
            }
            if (gltfNode.has_scale) {
                scale = {gltfNode.scale[0], gltfNode.scale[1],
                         gltfNode.scale[2]};
            } else {
                scale = {1, 1, 1};
            }
            Transform transform = {.translation = translation,
                                   .rotation = rotation,
                                   .scale = scale * 10.f};
            entity.SetTransform(transform);
        }
        // add the mesh
        if (gltfNode.mesh) {
            // get the index of the mesh
            int meshIndex = static_cast<int>(gltfNode.mesh - data->meshes);
            entity.AddMesh(aMeshManager.getMesh(meshIndex));
        }

        //        // check if the node has a light
        //        if (gltfNode.light != nullptr) {
        //            // get the location of the light
        //            glm::vec4 lightLocation = {gltfNode.translation[0],
        //                                       gltfNode.translation[1],
        //                                       gltfNode.translation[2], 1.0};
        //            // get the color of the light (and multiply the intensity)
        //            glm::vec4 lightColor = {gltfNode.light->color[0],
        //                                    gltfNode.light->color[1],
        //                                    gltfNode.light->color[2], 1.0};
        //            lightColor *= gltfNode.light->intensity;
        //            lightColor.a = 1.0;
        //            // if its a point light
        //            if (gltfNode.light->type == cgltf_light_type_point) {
        //                new Engine::Light(lightLocation, lightColor);
        //            }
        //            // if its a directional light
        //            else if (gltfNode.light->type ==
        //            cgltf_light_type_directional) {
        //                // the direction is the negative z axis of the
        //                rotation matrix
        //                // convert the quaternion to a rotation matrix
        //                glm::mat4 rotationMatrix =
        //                glm::mat4_cast(glm::normalize(
        //                    glm::quat(gltfNode.rotation[3],
        //                    gltfNode.rotation[0],
        //                              gltfNode.rotation[1],
        //                              gltfNode.rotation[2])));
        //                // the direction is the negative z axis of the
        //                rotation matrix glm::vec3 direction =
        //                -glm::vec3(rotationMatrix[2]);
        //
        //                new Engine::Light(lightLocation, direction,
        //                lightColor);
        //            }
        //        }
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
    //    if (aIsDebug) {
    aMeshManager.debugOuptutMeshes();
    aMaterialManager.DebugOutputMaterials();
    //    }

    cgltf_free(data);

    return GLTF_LOAD_SUCCESS;
}
} // namespace vk
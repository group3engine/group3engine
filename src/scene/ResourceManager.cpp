//
// Created by thomas on 05/02/25.
//

#include "ResourceManager.hpp"

#include <iostream>
#include <span>
#include <string>

#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION

#include "Entity.hpp"
#include "EntitySorter.hpp"
#include "Animation.hpp"
#include "Scene.hpp"
#include "Skin.hpp"
#include "LightManager.hpp"
#include "cgltf_write.h"
#include "glm/gtc/type_ptr.hpp"

#include <glm/gtx/io.hpp>

#include <spdlog/spdlog.h>

#include <json.hpp>

#include "Debugging.hpp"

namespace ResourceLoader {

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
             std::vector<Entity *> &aEntities, bool aIsDebug,
             std::vector<Animation> &aAnimations, std::vector<Skin> &aSkins,
             std::unordered_map<Entity *, std::vector<Entity *>> &aCharacterEntities,
             Scene *aScene) {
    // Convert directory separators to preferred directory separator
    // Slight try at cross-platform for Windows
    aFilepath.make_preferred();

    cgltf_options options = {};
    cgltf_data *data = nullptr;
    cgltf_result result =
        cgltf_parse_file(&options, aFilepath.string().c_str(), &data);
    if (result != cgltf_result_success) {
        SPDLOG_INFO("Failed to parse file. {}", aFilepath.string());
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
    defaultMaterial.normalTexture = aTextureManager.GetTexture("normal");
    defaultMaterial.normalTextureName = "normal";
    defaultMaterial.emissiveTexture = aTextureManager.GetTexture("white");
    defaultMaterial.emissiveTextureName = "white";

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
        // load the normal map if there is one
        if (gltfMaterial.normal_texture.texture) {
            std::string normalFileName =
                    gltfMaterial.normal_texture.texture->image->uri;
            std::string normalName = DecodeURI(
                    normalFileName, aFilepath.string());

            aTextureManager.addTexture(normalName,
                                       normalFileName);
            material.normalTexture =
                    aTextureManager.GetTexture(normalFileName);

            material.normalTextureName =
                    normalFileName;
        }
        else
        {
            material.normalTexture = aTextureManager.GetTexture("normal");
            material.normalTextureName = "normal";
        }

        material.doubleSided = gltfMaterial.double_sided;

        if (gltfMaterial.emissive_texture.texture)
        {
            std::string emissiveFileName =
                    gltfMaterial.emissive_texture.texture->image->uri;
            std::string emissiveName = DecodeURI(
                    emissiveFileName, aFilepath.string());

            aTextureManager.addTexture(emissiveName,
                                       emissiveFileName);
            material.emissiveTexture =
                    aTextureManager.GetTexture(emissiveFileName);

            material.emissiveTextureName =
                    emissiveFileName;
        }
        else
        {
            material.emissiveTexture = aTextureManager.GetTexture("white");
            material.emissiveTextureName = "white";
        }
        material.pbrMetallicRoughness.pbrMaterialNumbers.emissiveFactor =
                glm::vec4(gltfMaterial.emissive_factor[0],
                          gltfMaterial.emissive_factor[1],
                          gltfMaterial.emissive_factor[2],
                            0.f);
        if(gltfMaterial.has_emissive_strength)
        {
            material.pbrMetallicRoughness.pbrMaterialNumbers.emissiveFactor *= gltfMaterial.emissive_strength.emissive_strength;
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
            std::vector<float> tangents;
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

            // tangents
            if (const cgltf_accessor *tngent = cgltf_find_accessor(
                    &gltfPrimitive, cgltf_attribute_type_tangent, 0)) {
                size_t count = tngent->count * 4;
                tangents.resize(count);
                assert(cgltf_num_components(tngent->type) == 4);
                cgltf_accessor_unpack_floats(tngent, tangents.data(), count);
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

                glm::vec3 normal = {normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]};
                // Robustness check to prevent nans after normalizing
                if (normal == glm::vec3(0, 0, 0)) {
                    normal = glm::vec3(0.001f, 0.001f, 0.001f);
                }
                meshPrimitive.vertices[i].normal = normal;

                meshPrimitive.vertices[i].tex = {texcoords[i * 2],
                                                 texcoords[i * 2 + 1]};

                // if the mesh has tangents
                if (!tangents.empty()){
                    glm::vec4 tangent = {tangents[i * 4], tangents[i * 4 + 1],
                                         tangents[i * 4 + 2], tangents[i * 4 + 3]};
                    // Robustness check to prevent nans after normalizing
                    if (glm::vec3(tangent.x, tangent.y, tangent.z) == glm::vec3(0, 0, 0)) {
                        tangent.x = 0.001f;
                        tangent.y = 0.001f;
                        tangent.z = 0.001f;
                    }
                    meshPrimitive.vertices[i].tangent = tangent;
                } else {
                    SPDLOG_ERROR("Mesh primitive in {} is missing tangents. The glTF may have "
                                 "failed to export due to ngons or other issues.",
                                 gltfMesh.name);
                    assert(false);
                }

                // if the mesh has joints and weights
                if (!joints.empty() && !weights.empty()) {
                    meshPrimitive.vertices[i].joints = {joints[i * 4],
                                                       joints[i * 4 + 1],
                                                       joints[i * 4 + 2],
                                                       joints[i * 4 + 3]};
                    meshPrimitive.vertices[i].weights = {weights[i * 4],
                                                        weights[i * 4 + 1],
                                                        weights[i * 4 + 2],
                                                        weights[i * 4 + 3]};
                }
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
            if(gltfPrimitive.material == nullptr) {
                materialIndex = 0;
            }
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

        struct Extras {
            std::string entityType = "default";
            std::string physicsType = "static";
            std::string colliderType = "fallback_shape";
            std::vector<std::string> tags;
            std::unordered_map<std::string, float> float_properties;
            bool is_sensor = false;
            bool is_solid = false;
            bool is_invisible = false;
        } extras;

        if (gltfNode.extras.data) {
            std::span<char> v{gltfNode.extras.data, strlen(gltfNode.extras.data)};
            nlohmann::json json = nlohmann::json::parse(v);

            for (auto &[key, value] : json.items()) {
                if (key == "entity_type") {
                    extras.entityType = value;
                } else if (key == "physics_type") {
                    extras.physicsType = value;
                } else if (key == "collider_type") {
                    extras.colliderType = value;
                } else if (key == "tags") {
                    std::string tags = value;
                    // Split on pipes
                    std::istringstream tokenStream(tags);
                    std::string token;
                    while (std::getline(tokenStream, token, '|')) {
                        extras.tags.push_back(token);
                    }
                } else if (key == "is_sensor") {
                    extras.is_sensor = value;
                } else if (key == "is_solid") {
                    extras.is_solid = value;
                } else if (key == "is_invisible") {
                    extras.is_invisible = value;
                } else if (key == "float_properties") {
                    // Float properties are currently stored as an array of json
                    // objects. One json object per property/key-value pair
                    assert(value.is_array());
                    for (auto &object : value) {
                        // Float properties are arbitrary values, no error checking keys
                        for (auto &[propertyKey, propertyValue] : object.items()) {
                            extras.float_properties[propertyKey] = propertyValue;
                        }
                    }
                } else {
                    SPDLOG_ERROR("Unexpected token while parsing extras.");
                    exit(EXIT_FAILURE);
                }
            }
        }

        // select the entity type based on the extra
        Entity* entityPtr = CreateNewEntity(extras.entityType);
        aEntities.emplace_back(entityPtr);
        Entity &entity = *entityPtr;

        entity.InitID(aScene->PostIncrementEntityCount());

        if(extras.physicsType == "static")
        {
            entity.SetPhysicsType(PhysicsType::STATIC);
        }
        else if(extras.physicsType == "kinematic")
        {
            entity.SetPhysicsType(PhysicsType::KINEMATIC);
        }
        else if(extras.physicsType == "dynamic")
        {
            entity.SetPhysicsType(PhysicsType::DYNAMIC);
        }

        if (extras.colliderType == "convex_hull_shape") {
            entity.SetColliderType(ColliderType::ConvexHullShape);
        } else if (extras.colliderType == "mesh_shape") {
            entity.SetColliderType(ColliderType::MeshShape);
        } else if (extras.colliderType == "fallback_shape") {
            entity.SetColliderType(ColliderType::Fallback);
        } else {
            entity.SetColliderType(ColliderType::Fallback);
            SPDLOG_ERROR("Unaccounted for collider type {}", extras.colliderType);
            DEBUG_BREAK();
        }

        // set the name
        if (gltfNode.name) {
            entity.SetName(gltfNode.name);
        }
        // add the tags
        for (const auto &tag : extras.tags) {
            entity.AddTag(tag);
        }
        // set the float properties
        entity.SetFloatProperties(extras.float_properties);
        // set the sensor
        if (extras.is_sensor) {
            entity.SetAsSensor();
        }
        // set the solid
        if (!extras.is_solid) {
            entity.SetAsNotSolid();
        }
        // set the invisible
        if (extras.is_invisible) {
            entity.SetAsInvisible();
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
                rotation = {gltfNode.rotation[0], gltfNode.rotation[1],
                            gltfNode.rotation[2], gltfNode.rotation[3]};
            } else {
                rotation = {0, 0, 0, 1};
            }
            if (gltfNode.has_scale) {
                scale = {gltfNode.scale[0], gltfNode.scale[1],
                         gltfNode.scale[2]};
            } else {
                scale = {1, 1, 1};
            }
            Transform transform = {.translation = translation,
                                   .rotation = rotation,
                                   .scale = scale};
            transform.UpdateMatrix();
            entity.SetTransform(transform);
        }
        // add the mesh
        if (gltfNode.mesh) {
            // get the index of the mesh
            int meshIndex = static_cast<int>(gltfNode.mesh - data->meshes);
            entity.AddMesh(aMeshManager.getMesh(meshIndex));
        }
        // check if the node has a light
        if (gltfNode.light != nullptr) {
            // get the world transform of the node
            float worldTransform[16];
            cgltf_node_transform_world(&gltfNode, worldTransform);
            // get the translation of the world transform
            glm::vec3 translation = {worldTransform[12],
                                        worldTransform[13], worldTransform[14]};
            // get the location of the light
            glm::vec4 lightLocation = {translation, 1.0};
            // get the color of the light (and multiply the intensity)
            glm::vec4 lightColor = {gltfNode.light->color[0],
                                    gltfNode.light->color[1],
                                    gltfNode.light->color[2], 1.0};
            lightColor *= gltfNode.light->intensity / 1000.f;
            lightColor.w = 1.0;
            // if its a point light
            if (gltfNode.light->type == cgltf_light_type_point) {
                Light pointLight;
                pointLight.Type = LightType::Point;
                pointLight.position = lightLocation;
                pointLight.colour = lightColor;
                LightManager::getInstance().SetPointLight(&pointLight);
            }
                // if its a directional light
            else if (gltfNode.light->type ==
                     cgltf_light_type_directional) {
                // the direction is the negative z axis of the
                // rotation matrix
                // convert the quaternion to a rotation matrix
                glm::mat4 rotationMatrix =
                        glm::mat4_cast(glm::normalize(
                                glm::quat(gltfNode.rotation[0],
                                          gltfNode.rotation[1],
                                          gltfNode.rotation[2],
                                          gltfNode.rotation[3])));
                // the direction is the negative z axis of the
                // rotation matrix
                glm::vec3 direction = glm::vec3(rotationMatrix[2]);

                Light directionalLight {};
                directionalLight.Type = LightType::Directional;
                directionalLight.position = glm::vec4(direction, 1.f);
                directionalLight.colour = lightColor;
                LightManager::getInstance().SetDirectionalLight(&directionalLight);

            }
        }

    }
    // last, create the root node
    Entity *root = new Entity();
    root->SetName("root");
    root->SetTransform(glm::mat4(1.f));
    aEntities.push_back(root);
    // go through each entity and add its parent
    for (size_t ni = 0; ni < data->nodes_count; ni++) {
        const auto &gltfNode = data->nodes[ni];
        if (gltfNode.parent) {
            int parentIndex = static_cast<int>(gltfNode.parent - data->nodes);
            assert(parentIndex < (int)data->nodes_count);
            aEntities[ni]->SetParent(aEntities[parentIndex]);
        }
        else
        {
            aEntities[ni]->SetParent(root);
        }
    }
    // update the children from the root node
    root->SetTransform(glm::mat4(1.f));
    // add skins
    for (size_t i = 0; i < data->skins_count; i++) {
        const auto &gltfSkin = data->skins[i];
        Skin skin;
        skin.SetName(gltfSkin.name);
        skin.ResizeJoints(gltfSkin.joints_count);
        // get the "skeleton" node
        Entity *skeleton = nullptr;
        if(gltfSkin.skeleton != nullptr) {
            skeleton = aEntities[gltfSkin.skeleton - data->nodes];
        }
        skin.SetRoot(skeleton);
        // get the inverse bind matrices
        std::vector<glm::mat4> inverseBindMatrices;

        for (size_t j = 0; j < gltfSkin.joints_count; ++j) {
            float inverse_bind_matrix[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

            if (gltfSkin.inverse_bind_matrices) {
                cgltf_accessor_read_float(gltfSkin.inverse_bind_matrices, j, inverse_bind_matrix, 16);

                auto m = glm::make_mat4(inverse_bind_matrix);

                inverseBindMatrices.push_back(m);
            }
        }

        // get the joints
        std::vector<Entity *> joint_nodes;
        joint_nodes.resize(gltfSkin.joints_count);
        for (size_t j = 0; j < gltfSkin.joints_count; j++) {
            joint_nodes[j] = aEntities[gltfSkin.joints[j] - data->nodes];
        }
        // add the joints
        for (size_t j = 0; j < gltfSkin.joints_count; j++) {
            Joint joint{};
            joint.entity = joint_nodes[j];
            joint.inverseBindMatrix = inverseBindMatrices[j];
            skin.AddJoint(joint);
        }
        skin.ComputeRoot();
        aSkins.push_back(skin);
    }


    // add animations
    std::vector<Animation *> animationPointers;
    aAnimations.reserve(data->animations_count);
    for (size_t i = 0; i < data->animations_count; i++) {
        const auto &gltfAnimation = data->animations[i];
        aAnimations.emplace_back();
        Animation* animation = &aAnimations.back();
        animation->SetName(gltfAnimation.name);
        // add the samplers
        animation->ResizeSamplers(gltfAnimation.samplers_count);
        for (size_t j = 0; j < gltfAnimation.samplers_count; j++) {
            const auto &gltfSampler = gltfAnimation.samplers[j];
            Sampler sampler;
            assert(gltfSampler.interpolation == cgltf_interpolation_type_linear ||
                   gltfSampler.interpolation == cgltf_interpolation_type_step);
            sampler.interpolation =
                static_cast<Interpolation>(gltfSampler.interpolation);
            sampler.keyframes.resize(gltfSampler.input->count);
            std::vector<float> times;
            times.resize(gltfSampler.input->count);
            cgltf_accessor_unpack_floats(gltfSampler.input, times.data(),
                                         sampler.keyframes.size());
            for (size_t k = 0; k < sampler.keyframes.size(); k++) {
                sampler.keyframes[k].time = times[k];
            }
            std::vector<float> values;
            bool vec3 = false;
            if (gltfSampler.output->type == cgltf_type_vec3) {
                vec3 = true;
            } else {
                assert(gltfSampler.output->type == cgltf_type_vec4);
            }
            int outputSize = vec3 ? 3 : 4;
            outputSize *= static_cast<int>(gltfSampler.output->count);
            values.resize(outputSize);
            cgltf_accessor_unpack_floats(gltfSampler.output, values.data(),
                                         values.size());
            // if output size is 4 * input size, then it is a vec4, otherwise it
            // is a vec3
            if (vec3) {
                for (size_t k = 0; k < sampler.keyframes.size(); k++) {
                    sampler.keyframes[k].value = {values[k * 3],
                                                  values[k * 3 + 1],
                                                  values[k * 3 + 2], 1.0f};
                }
            } else {
                for (size_t k = 0; k < sampler.keyframes.size(); k++) {
                    sampler.keyframes[k].value = glm::normalize(glm::vec4{
                        values[k * 4], values[k * 4 + 1], values[k * 4 + 2],
                        values[k * 4 + 3]});
                }
            }

            animation->AddSampler(sampler);
        }
        // add the channels
        animation->ResizeChannels(gltfAnimation.channels_count);
        // create a vector of the entities targeted by this animation
        std::vector<Entity *> entitiesInAnimation;
        for (size_t j = 0; j < gltfAnimation.channels_count; j++) {
            const auto &gltfChannel = gltfAnimation.channels[j];
            Channel channel = {};
            channel.target = aEntities[gltfChannel.target_node - data->nodes];
            entitiesInAnimation.push_back(channel.target);
            channel.targetIndex = -1;
            channel.sampler = static_cast<size_t>(gltfChannel.sampler -
                                               gltfAnimation.samplers);
            switch (gltfChannel.target_path) {
            case cgltf_animation_path_type_translation:
                channel.transformChannel = TransformChannel::TRANSLATION;
                break;
            case cgltf_animation_path_type_rotation:
                channel.transformChannel = TransformChannel::ROTATION;
                break;
            case cgltf_animation_path_type_scale:
                channel.transformChannel = TransformChannel::SCALE;
                break;
            default:
                // we don't support other channels, so skip
                continue;
            }
            animation->AddChannel(channel);
        }
        // find the skin that corresponds to this animation. Error if there isn't one
        // for each skin, call the IsTheAnimationForThisSkin function
        bool foundSkin = false;
        for (auto & aSkin : aSkins) {
            if (aSkin.DetargetAnimation(animation, entitiesInAnimation)) {
                foundSkin = true;
                break;
            }
        }
        if (!foundSkin) {
            SPDLOG_ERROR("The animation {} targets multiple skins. This is not supported. Make "
                         "sure your animations only target one skin.",
                         animation->GetName());
            continue;
            //exit(EXIT_FAILURE);
        }

        animationPointers.push_back(animation);
    }

    // for each entity, if it has a skin, add an animator
    for (size_t i = 0; i < data->nodes_count; i++) {
        const auto &gltfNode = data->nodes[i];
        if (gltfNode.skin) {
            int skinIndex = static_cast<int>(gltfNode.skin - data->skins);
            auto *animator =
                new Animator(&aMeshManager.mContext, &aSkins[skinIndex]);
            aEntities[i]->SetAnimator(animator);
            animator->SetAnimations(animationPointers);
        }
    }

    //    if (aIsDebug) {
    aMeshManager.debugOuptutMeshes();
    aMaterialManager.DebugOutputMaterials();
    //    }

    cgltf_free(data);

    for (auto &&entity : aEntities) {
        if (entity->IsCharacter()) {
            // Try emplace the parent entity as a key and an empty vector of
            // children as the value. The key might already have been inserted
            // into the map if the children were found first
            aCharacterEntities.try_emplace(entity);
            // Mark as invalid
            entity = nullptr;
            continue;
        }

        Entity *parent = entity;
        while (parent) {
            if (parent->IsCharacter()) {
                aCharacterEntities[parent].push_back(entity);
                // Mark as invalid
                entity = nullptr;
                break;
            }

            parent = parent->GetParent();
        }
    }

    // Remove character entities from main entitites by partitioning nullptr
    // (invalid) entities and erasing them
    auto first = std::partition(aEntities.begin(), aEntities.end(),
                                [](Entity *entity) { return entity; });
    aEntities.erase(first, aEntities.end());

    return GLTF_LOAD_SUCCESS;
}

} // namespace ResourceLoader

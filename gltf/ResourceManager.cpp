//
// Created by thomas on 05/02/25.
//

#include "ResourceManager.hpp"
#define CGLTF_IMPLEMENTATION
#define CGLTF_WRITE_IMPLEMENTATION
#include "cgltf_write.h"
#include <iostream>

int LoadGLTF(std::string aFilepath, MeshManager &aMeshManager,
             bool aIsDebug) {
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

    if (aIsDebug) {
        aMeshManager.debugOuptutMeshes();
    }

    cgltf_free(data);
        return GLTF_LOAD_SUCCESS;
}

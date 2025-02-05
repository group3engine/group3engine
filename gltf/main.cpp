#include "cgltf.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

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

int main() {
    cgltf_options options = {};
    cgltf_data *data = nullptr;
    std::string path = "../Box.gltf";
    cgltf_result result = cgltf_parse_file(&options, path.c_str(), &data);
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

    result = cgltf_load_buffers(&options, data, path.c_str());
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

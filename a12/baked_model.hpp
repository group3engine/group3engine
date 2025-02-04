#ifndef BAKED_MODEL_HPP_7D7BFF3A_1743_43DF_8D4F_D67D80FD8282
#define BAKED_MODEL_HPP_7D7BFF3A_1743_43DF_8D4F_D67D80FD8282

#include <cstdint>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <stdexcept>
#include <string>
#include <vector>

#include "glm/ext/matrix_float4x4.hpp"
#include "glm/vec4.hpp"
#include <array>
#include <volk/volk.h>

/* Baked file format:
 *
 *  1. Header:
 *    - 16*char: file magic = "\0\0COMP5892Mmesh"
 *    - 16*char: variant = "default" (changes later)
 *
 *  2. Textures
 *    - 1*uint32_t: U = number of (unique) textures
 *    - repeat U times:
 *      - string: path to texture
 *      - 1*uint8_t: texture color space (see ETextureSpace)
 *      - 1*uint8_t: number of channels in texture
 *
 *  3. Material information
 *    - 1*uint32_t: M = number of materials
 *    - repeat M times:
 *      - uint32_t: base color texture index
 *      - uint32_t: roughness texture index
 *      - uint32_t: metalness texture index
 *      - uint32_t: alpha mask texture index; set to 0xffffffff if not available
 *      - uint32_t: normal map texture index; set to 0xffffffff if not available
 *      - uint32_t: emissive texture index;
 *
 *  4. Mesh data
 *    - 1*uint32_t: M = number of meshes
 *    - repeat M times:
 *      - uint32_t : material index
 *      - uint32_t : V = number of vertices
 *      - uint32_t : I = number of indices
 *      - repeat V times: vec3 position
 *      - repeat V times: vec3 normal
 *      - repeat V times: vec2 texture coordinate
 *      - repeat I times: uint32_t index
 *
 * Strings are stored as
 *   - 1*uint32_t: N = length of string in chars, including terminating \0
 *   - repeat N times: char in string
 *
 * See cw2-bake/main.cpp (specifically write_model_data_()) for additional
 * information.
 *
 *
 * My suggestion for loading the data into Vulkan is as follows:
 *
 * - Create and load textures. This gives a list of Images (which includes a
 *   VkImage + VmaAllocation) and VkImageViews. We only need to keep these
 *   around -- place them in a vector.
 *
 * - Create a Descriptor Set Layout for material information only. Initially,
 *   this would include three textures (base color, metalness, roughness).
 *
 * - Create a Descriptor Set for each material. You can easily get the
 *   VkImageViews from the list in the first step by the index in the
 *   BaseMaterialInfo. This also avoids loading duplicates of textures if they
 *   are reused across multiple materials.
 *
 * - Upload mesh data. In my reference solution, I created separate VkBuffers
 *   for each mesh (one for each attribute and one for the indices).
 */

struct Vertex {

    glm::vec3 pos;
    glm::vec2 tex;
    uint32_t compressedTBN;

    static std::array<VkVertexInputBindingDescription, 3> GetBindingDescription() {
        //VkVertexInputBindingDescription bindingDescrip{};
        //bindingDescrip.binding = 0;
        //bindingDescrip.stride = sizeof(Vertex);
        //bindingDescrip.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        //return bindingDescrip;

        std::array<VkVertexInputBindingDescription, 3> vertexInputs = {};
        // the iPosition
        vertexInputs[0].binding = 0;
        vertexInputs[0].stride = sizeof(float) * 3;
        vertexInputs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // the iTexCoord
        vertexInputs[1].binding = 1;
        vertexInputs[1].stride = sizeof(float) * 2;
        vertexInputs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        // the compressed tbn frame
        vertexInputs[2].binding = 2;
        vertexInputs[2].stride = sizeof(uint32_t);
        vertexInputs[2].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return vertexInputs;
    }

    static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributes = {};

        //attributes[0].binding = 0;
        //attributes[0].location = 0;
        //attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        //attributes[0].offset = offsetof(Vertex, pos);

        //attributes[1].binding = 0;
        //attributes[1].location = 1;
        //attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        //attributes[1].offset = offsetof(Vertex, tex);

        //attributes[2].binding = 0;
        //attributes[2].location = 2;
        //attributes[2].format = VK_FORMAT_R32_UINT;
        //attributes[2].offset = offsetof(Vertex, compressedTBN);

        attributes[0].binding = 0;
        attributes[0].location = 0;
        attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[0].offset = 0;
        // the colour
        attributes[1].binding = 1;
        attributes[1].location = 1;
        attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributes[1].offset = 0;
        // the compressed tbn frame
        attributes[2].binding = 2;
        attributes[2].location = 2;
        attributes[2].format = VK_FORMAT_R32_UINT;

        return attributes;
    }
};

enum class ETextureSpace : std::uint8_t
{
	unorm = 0,
	srgb = 1
};

struct BakedTextureInfo
{
	std::string path;
	ETextureSpace space;
	std::uint8_t channels;
};

struct BakedMaterialInfo
{
	std::uint32_t baseColorTextureId;
	std::uint32_t roughnessTextureId;
	std::uint32_t metalnessTextureId;
	std::uint32_t alphaMaskTextureId; // May be set to 0xffffffff if no alpha mask
	std::uint32_t normalMapTextureId; // May be set to 0xffffffff if no normal map
	std::uint32_t emissiveTextureId; // May be set to 0xffffffff if no emissive map


    [[nodiscard]] bool alpha() const
    {
        return alphaMaskTextureId != 0xffffffff;
    }

	// The emissive map can be ignored in Assignment 1.2. It is only required 
	// in parts of Assignment 2.2.

    // to allow us to iterate over the textures we implement the [] operator
    std::uint32_t operator[](std::uint32_t index) const
    {
        switch (index)
        {
        case 0:
            return baseColorTextureId;
        case 1:
            return roughnessTextureId;
        case 2:
            return metalnessTextureId;
        case 3:
            return alphaMaskTextureId;
        case 4:
            if(normalMapTextureId == 0xffffffff)
            {
                throw std::out_of_range("Material does not have a normal map");
            }
            return normalMapTextureId;
        case 5:
            return emissiveTextureId;
        default:
            throw std::out_of_range("Material texture index out of range");
        }
    }
};

struct BakedMeshData
{
	std::uint32_t materialId;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec2> texcoords;
    std::vector<std::uint32_t> compressedTBN;
    glm::mat4 modelMatrix;

	std::vector<std::uint32_t> indices;
    std::vector<Vertex> vertexData;

};

struct BakedModel
{
	std::vector<BakedTextureInfo> textures;
	std::vector<BakedMaterialInfo> materials;
	std::vector<BakedMeshData> meshes;
};

BakedModel load_baked_model( char const* aModelPath );

#endif // BAKED_MODEL_HPP_7D7BFF3A_1743_43DF_8D4F_D67D80FD8282


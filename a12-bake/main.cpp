#include <cstdlib>
#include <tgen.h>

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <system_error>
#include <typeinfo>
#include <unordered_map>
#include <vector>

#include "index_mesh.hpp"
#include "input_model.hpp"
#include "load_model_obj.hpp"


namespace {
// constants
/* File "magic". The first 16 bytes of our custom file are equal to this
 * magic value. This allows us to check whether a certain file is
 * (probably) of the right type. Having a file magic is relatively common
 * practice -- you can find a list of such magic sequences e.g. here:
 * https://en.wikipedia.org/wiki/List_of_file_signatures
 *
 * When picking a signature there are a few considerations. For example,
 * including non-printable characters (e.g. the \0) early keeps the file
 * from being misidentified as text.
 */
constexpr char kFileMagic[16] = "\0\0COMP5892Mmesh";

/* Note: change the file variant if you change the file format!
 *
 * Suggestion: use 'uid-tag'. For example, I would use "scsmbil-tan" to
 * indicate that this is a custom format by myself (=scsmbil) with
 * additional tangent space information.
 */
constexpr char kFileVariant[16] = "default-a12";

/* Fallback texture for RGBA 1111 and Grayscale 1
 */
constexpr char kTextureFallbackR1[] = "assets-src/a12/r1.png";
constexpr char kTextureFallbackRGBA1111[] = "assets-src/a12/rgba1111.png";
constexpr char kTextureFallbackRGB000[] = "assets-src/a12/rgb000.png";

// types
struct TextureInfo_ {
    std::uint32_t uniqueId;
    std::uint8_t space;
    std::uint8_t channels;
    std::string newPath;
};

// local functions:
void process_model_(
    char const *aOutput, char const *aInputOBJ,
    glm::mat4x4 const &aStaticTransform = glm::mat4x4(1.f)
);

void add_tangents(IndexedMesh &aMesh);
uint32_t quantize_component(float component);
uint32_t quaternion_compacter(glm::quat quaternion);

InputModel normalize_(InputModel);

void write_model_data_(FILE *, InputModel const &,
                       std::vector<IndexedMesh> const &,
                       std::unordered_map<std::string, TextureInfo_> const &);

std::vector<IndexedMesh> index_meshes_(InputModel const &,
                                       float aErrorTolerance = 1e-5f);

std::unordered_map<std::string, TextureInfo_> find_unique_textures_(
    InputModel const &);

std::unordered_map<std::string, TextureInfo_> new_paths_(
    std::unordered_map<std::string, TextureInfo_>,
    std::filesystem::path const &aTexDir);

}  // namespace

int main() {
#if !defined(NDEBUG)
    std::printf(
        "Suggest running this in release mode (it appears to be running in "
        "debug)\n");
    std::printf(
        "Especially under VisualStudio/MSVC, the debug build seems very "
        "slow.\n");
    /* A few notes:
     *
     * I have not profiled this at all. The following are based on previous
     * experience(s).
     *
     * - ZStd benefits immensely from compiler optimizations.
     *
     * - Under MSVC, std::unordered_set performs quite badly in debug mode. This
     *   may be further related to other debug-related options (e.g., extended
     *   iterator checking...).
     *
     *   Normally, I avoid unordered_set here, and instead rely on one of the
     * many high quality flat_set implementations. They tend to be faster from
     * the get go and perform more equally under different compilers.
     *
     * - NDEBUG is the standard macro to control the behaviour of assert(). When
     *   NDEBUG is defined, assert() will "do nothing" (they're expanded to an
     *   empty statement). This is typically desirable in a release build, but
     * not necessary or guaranteed. (Indeed, the premake sets NDEBUG explicitly
     * for this project -- this is why the check above works. But don't rely on
     * this blindly.)
     *
     * - The VisualStudio interactive debugger's heap profiler (the thing that
     *   shows you the memory usage graph) carries a measurable overhead as
     * well.
     *
     * The binary .comp5892mesh should be unchanged between debug and release
     * builds, so you can safely use the release build to create the file once,
     * even while debugging the main A12 program.
     */
#endif
    process_model_("assets/a12/suntemple.comp5892mesh",
                   "assets-src/a12/suntemple.obj-zstd");

    return 0;
}

namespace {

//    glm::vec4 decompressQuaternion(uint32_t compressed)
//    {
//        // extract the index of the largest component
//        uint32_t index = compressed >> 30;
//        // extract the 10 bits of each component
//        uint32_t x = (compressed >> 20) & 0x3FFu;
//        uint32_t y = (compressed >> 10) & 0x3FFu;
//        uint32_t z = compressed & 0x3FFu;
//        // convert the 10 bits to a float in the range [0, 1]
//        float fx = float(x) / 1023.0f;
//        float fy = float(y) / 1023.0f;
//        float fz = float(z) / 1023.0f;
//        // convert the float to the range [-1/sqrt(2), 1/sqrt(2)]
//        float oneOverSqrt2 = 1.0f / (float)sqrt(2.0);
//        fx = fx * 2.f * oneOverSqrt2 - oneOverSqrt2;
//        fy = fy * 2.f * oneOverSqrt2 - oneOverSqrt2;
//        fz = fz * 2.f * oneOverSqrt2 - oneOverSqrt2;
//        // calculate the largest component
//        auto fw = (float)sqrt(1.0 - fx * fx - fy * fy - fz * fz);
//        // create the quaternion
//        switch (index)
//        {
//            case 0:
//                return {fw, fx, fy, fz};
//            case 1:
//                return {fx, fw, fy, fz};
//            case 2:
//                return {fx, fy, fw, fz};
//            case 3:
//                return {fx, fy, fz, fw};
//            default:
//                throw lut::Error("Invalid index %u", index);
//        }
//    }
void add_tangents(IndexedMesh &aMesh) {
    // init empty corner tangents and bitangents
    std::vector<tgen::RealT> cTangents3D;
    std::vector<tgen::RealT> cBitangents3D;
    // init empty vertex tangents and bitangents
    std::vector<tgen::RealT> vTangents3D;
    std::vector<tgen::RealT> vBitangents3D;
    // init tangent 4D to zeros
    std::vector<tgen::RealT> tangents4D;
    // uv indices ( same as triangle indices )
    std::vector<tgen::VIndexT> triIndicesUV(aMesh.indices.begin(),
                                            aMesh.indices.end());
    // indices as a vector of tgen::VIndexT
    std::vector<tgen::VIndexT> triIndicesPos(aMesh.indices.begin(),
                                             aMesh.indices.end());
    // positions3D as a vector of tgen::RealT (doubles)
    std::vector<tgen::RealT> positions3D;
    for (auto const &v : aMesh.vert) {
        positions3D.push_back(v.x);
        positions3D.push_back(v.y);
        positions3D.push_back(v.z);
    }
    // texture coordinates as a vector of tgen::RealT (doubles)
    std::vector<tgen::RealT> uvs2D;
    for (auto const &t : aMesh.text) {
        uvs2D.push_back(t.x);
        uvs2D.push_back(t.y);
    }
    // normals as a vector of tgen::RealT (doubles)
    std::vector<tgen::RealT> normals3D;
    for (auto const &n : aMesh.norm) {
        normals3D.push_back(n.x);
        normals3D.push_back(n.y);
        normals3D.push_back(n.z);
    }
    tgen::computeCornerTSpace(triIndicesPos, triIndicesUV, positions3D, uvs2D,
                              cTangents3D, cBitangents3D);
    tgen::computeVertexTSpace(triIndicesUV, cTangents3D, cBitangents3D,
                              triIndicesUV.size(), vTangents3D, vBitangents3D);
    tgen::orthogonalizeTSpace(normals3D, vTangents3D, vBitangents3D);
    tgen::computeTangent4D(normals3D, vTangents3D, vBitangents3D, tangents4D);
    if (tangents4D.size() != aMesh.vert.size() * 4) {
        std::fprintf(stderr, 
            "Tangent generation failed: expected %zu tangents, got %zu",
            aMesh.vert.size(), tangents4D.size());
        std::exit(EXIT_FAILURE);
    }
    aMesh.tangent.resize(aMesh.vert.size());
    aMesh.compressedTBN.resize(aMesh.vert.size());
    for (std::size_t i = 0; i < tangents4D.size(); i += 4) {
        aMesh.tangent[i / 4] = glm::vec4(tangents4D[i], tangents4D[i + 1],
                                         tangents4D[i + 2], tangents4D[i + 3]);
    }
    // construct the tbn frames
    for (std::size_t i = 0; i < aMesh.vert.size(); i++) {
        auto const &n = glm::normalize(aMesh.norm[i]);
        glm::vec3 const t = glm::normalize(glm::vec3(aMesh.tangent[i]));
        auto const b = glm::normalize(glm::cross(n, t) * aMesh.tangent[i].w);
        // normalise the vectors as they are not guaranteed to be unit length
        glm::mat3 tbn = glm::mat3(t, b, n);
        glm::quat quaternion = glm::quat_cast(tbn);
        quaternion = glm::normalize(quaternion);
        // compact the quaternion into a uint32_t
        uint32_t compacted = quaternion_compacter(quaternion);
        aMesh.compressedTBN[i] = compacted;
    }
}
uint32_t quaternion_compacter(glm::quat quaternion) {
    uint32_t ret = 0;
    // work out the biggest component
    float max = std::max(std::max(quaternion.x, quaternion.y),
                         std::max(quaternion.z, quaternion.w));
    float threeComponents[3];
    // switch on the biggest component
    if (max == quaternion.x) {
        // mask the two most significant bits as x index (0)
        ret |= 0b0000'0000'0000'0000'0000'0000'0000'0000;
        threeComponents[0] = quaternion.y;
        threeComponents[1] = quaternion.z;
        threeComponents[2] = quaternion.w;
    } else if (max == quaternion.y) {
        // mask the two most significant bits as y index (1)
        ret |= 0b0100'0000'0000'0000'0000'0000'0000'0000;
        threeComponents[0] = quaternion.x;
        threeComponents[1] = quaternion.z;
        threeComponents[2] = quaternion.w;
    } else if (max == quaternion.z) {
        // mask the two most significant bits as z index (2)
        ret |= 0b1000'0000'0000'0000'0000'0000'0000'0000;
        threeComponents[0] = quaternion.x;
        threeComponents[1] = quaternion.y;
        threeComponents[2] = quaternion.w;
    } else {
        // mask the two most significant bits as w index (3)
        ret |= 0b1100'0000'0000'0000'0000'0000'0000'0000;
        threeComponents[0] = quaternion.x;
        threeComponents[1] = quaternion.y;
        threeComponents[2] = quaternion.z;
    }
    uint32_t x = quantize_component(threeComponents[0]);
    uint32_t y = quantize_component(threeComponents[1]);
    uint32_t z = quantize_component(threeComponents[2]);
    ret |= x << 20;
    ret |= y << 10;
    ret |= z;
    return ret;
}
uint32_t quantize_component(float component) {
    assert(component >= -1.f / glm::sqrt(2.f) &&
           component <= 1.f / glm::sqrt(2.f));
    // quantize component to 10 bits between -1/sqrt(2) and 1/sqrt(2)
    component += 1.f / glm::sqrt(2.f);
    component /= 2.f / glm::sqrt(2.f);
    return uint32_t(component * 1023.f);
}

void process_model_(char const *aOutput, char const *aInputOBJ,
                    glm::mat4x4 const &aStaticTransform) {
    static constexpr std::size_t vertexSize = sizeof(float) * (3 + 3 + 2 + 4);

    // Figure out output paths
    std::filesystem::path const outname(aOutput);
    std::filesystem::path const rootdir = outname.parent_path();
    std::filesystem::path const basename = outname.stem();
    std::filesystem::path const texdir = basename.string() + "-tex";

    // Load input model
    auto const model = normalize_(load_compressed_wavefront_obj(aInputOBJ));

    std::size_t inputVerts = 0;
    for (auto const &imesh : model.meshes) inputVerts += imesh.vertexCount;

    std::printf("%s: %zu meshes, %zu materials\n", aInputOBJ,
                model.meshes.size(), model.materials.size());
    std::printf(" - triangle soup vertices: %zu => %zu kB\n", inputVerts,
                inputVerts * vertexSize / 1024);

    // Index meshes
    auto indexed = index_meshes_(model);

    // add tangents to the indexed meshes
    for (auto &imesh : indexed) add_tangents(imesh);

    std::size_t outputVerts = 0, outputIndices = 0;
    for (auto const &mesh : indexed) {
        outputVerts += mesh.vert.size();
        outputIndices += mesh.indices.size();
    }

    std::printf(
        " - indexed vertices: %zu with %zu indices => %zu kB\n", outputVerts,
        outputIndices,
        (outputVerts * vertexSize + outputIndices * sizeof(std::uint32_t)) /
            1024);

    // Find list of unique textures
    auto const textures = new_paths_(find_unique_textures_(model), texdir);

    std::printf(" - unique textures: %zu\n", textures.size());

    // Ensure output directory exists
    std::filesystem::create_directories(rootdir);

    // Output mesh data
    auto mainpath = rootdir / basename;
    mainpath.replace_extension("comp5892mesh");

    FILE *fof = std::fopen(mainpath.string().c_str(), "wb");
    if (!fof) {
        std::fprintf(stderr, "Unable to open '%s' for writing",
                         mainpath.string().c_str());
        std::exit(EXIT_FAILURE);
    }

    write_model_data_(fof, model, indexed, textures);

    std::fclose(fof);

    // Copy textures
    std::filesystem::create_directories(rootdir / texdir);

    std::size_t errors = 0;
    for (auto const &entry : textures) {
        auto const dest = rootdir / entry.second.newPath;

        std::error_code ec;
        bool ret = std::filesystem::copy_file(
            entry.first, dest, std::filesystem::copy_options::none, ec);

        if (!ret) {
            ++errors;
            std::fprintf(stderr, "copy_file(): '%s' failed: %s (%s)\n",
                         dest.string().c_str(), ec.message().c_str(),
                         ec.category().name());
        }
    }

    auto const total = textures.size();
    std::printf("Copied %zu textures out of %zu.\n", total - errors, total);
    if (errors) {
        std::fprintf(stderr,
                     "Some copies reported an error. Currently, the code will "
                     "never overwrite existing files. The errors likely just "
                     "indicate that the file was copied previously. Remove old "
                     "files manually, if necessary.\n");
    }
}
}  // namespace

namespace {
InputModel normalize_(InputModel aModel) {
    for (auto &mat : aModel.materials) {
        if (mat.baseColorTexturePath.empty())
            mat.baseColorTexturePath = kTextureFallbackRGBA1111;
        if (mat.roughnessTexturePath.empty())
            mat.roughnessTexturePath = kTextureFallbackR1;
        if (mat.metalnessTexturePath.empty())
            mat.metalnessTexturePath = kTextureFallbackR1;
        if (mat.emissiveTexturePath.empty())
            mat.emissiveTexturePath = kTextureFallbackRGB000;
    }

    return aModel;  // This should use the move constructor implicitly.
}
}  // namespace

namespace {
void checked_write_(FILE *aOut, std::size_t aBytes, void const *aData) {
    auto const ret = std::fwrite(aData, 1, aBytes, aOut);

    if (ret != aBytes) {
        std::fprintf(stderr, "fwrite() failed: %zu instead of %zu", ret, aBytes);
        std::exit(EXIT_FAILURE);
    }
}

void write_string_(FILE *aOut, char const *aString) {
    // Write a string
    // Format:
    //  - uint32_t : N = length of string in bytes, including terminating '\0'
    //  - N x char : string
    auto const length = std::uint32_t(std::strlen(aString) + 1);
    checked_write_(aOut, sizeof(std::uint32_t), &length);

    checked_write_(aOut, length, aString);
}

void write_model_data_(
    FILE *aOut, InputModel const &aModel,
    std::vector<IndexedMesh> const &aIndexedMeshes,
    std::unordered_map<std::string, TextureInfo_> const &aTextures) {
    // Write header
    // Format:
    //   - char[16] : file magic
    //   - char[16] : file variant ID
    checked_write_(aOut, sizeof(char) * 16, kFileMagic);
    checked_write_(aOut, sizeof(char) * 16, kFileVariant);

    // Write list of unique textures
    // Format:
    //  - unit32_t : U = number of unique textures
    //  - repeat U times:
    //    - string : path to texture
    //    - uint8_t : texture color space (0 = unorm, 1 = srgb)
    //    - uint8_t : number of channels in texture
    std::vector<TextureInfo_ const *> orderedUnqiue(aTextures.size());
    for (auto const &tex : aTextures) {
        assert(!orderedUnqiue[tex.second.uniqueId]);
        orderedUnqiue[tex.second.uniqueId] = &tex.second;
    }

    auto const textureCount = std::uint32_t(orderedUnqiue.size());
    checked_write_(aOut, sizeof(textureCount), &textureCount);

    for (auto const &tex : orderedUnqiue) {
        assert(tex);
        write_string_(aOut, tex->newPath.c_str());

        std::uint8_t space = tex->space;
        checked_write_(aOut, sizeof(space), &space);

        std::uint8_t channels = tex->channels;
        checked_write_(aOut, sizeof(channels), &channels);
    }

    // Write material information
    // Format:
    //  - uint32_t : M = number of materials
    //  - repeat M times:
    //    - uin32_t : base color texture index
    //    - uin32_t : roughness texture index
    //    - uin32_t : metalness texture index
    //    - uin32_t : alphaMask texture index (or 0xffffffff if none)
    //    - uin32_t : normalMap texture index (or 0xffffffff if none)
    //    - uin32_t : emissive texture index
    auto const materialCount = std::uint32_t(aModel.materials.size());
    checked_write_(aOut, sizeof(materialCount), &materialCount);

    for (auto const &mat : aModel.materials) {
        auto const write_tex_ = [&](std::string const &aTexturePath) {
            if (aTexturePath.empty()) {
                static constexpr std::uint32_t sentinel = ~std::uint32_t(0);
                checked_write_(aOut, sizeof(std::uint32_t), &sentinel);
                return;
            }

            auto const it = aTextures.find(aTexturePath);
            assert(aTextures.end() != it);

            checked_write_(aOut, sizeof(std::uint32_t), &it->second.uniqueId);
        };

        write_tex_(mat.baseColorTexturePath);
        write_tex_(mat.roughnessTexturePath);
        write_tex_(mat.metalnessTexturePath);
        write_tex_(mat.alphaMaskTexturePath);
        write_tex_(mat.normalMapTexturePath);
        write_tex_(mat.emissiveTexturePath);
    }

    // Write mesh data
    // Format:
    //  - uint32_t : M = number of meshes
    //  - repeat M times:
    //    - uint32_t : material index
    //    - uint32_t : V = number of vertices
    //    - uint32_t : I = number of indices
    //    - repeat V times: vec3 position
    //    - repeat V times: vec3 normal
    //    - repeat V times: vec2 texture coordinate
    //    - repeat I times: uint32_t index
    auto const meshCount = std::uint32_t(aModel.meshes.size());
    checked_write_(aOut, sizeof(meshCount), &meshCount);

    assert(aModel.meshes.size() == aIndexedMeshes.size());
    for (std::size_t i = 0; i < aModel.meshes.size(); ++i) {
        auto const &mmesh = aModel.meshes[i];

        auto materialIndex = std::uint32_t(mmesh.materialIndex);
        checked_write_(aOut, sizeof(materialIndex), &materialIndex);

        auto const &imesh = aIndexedMeshes[i];

        auto vertexCount = std::uint32_t(imesh.vert.size());
        checked_write_(aOut, sizeof(vertexCount), &vertexCount);
        auto indexCount = std::uint32_t(imesh.indices.size());
        checked_write_(aOut, sizeof(indexCount), &indexCount);

        checked_write_(aOut, sizeof(glm::vec3) * vertexCount,
                       imesh.vert.data());
        checked_write_(aOut, sizeof(glm::vec2) * vertexCount,
                       imesh.text.data());
        checked_write_(aOut, sizeof(std::uint32_t) * vertexCount,
                       imesh.compressedTBN.data());

        checked_write_(aOut, sizeof(std::uint32_t) * indexCount,
                       imesh.indices.data());

        checked_write_(aOut, sizeof(glm::mat4x4), &imesh.modelMatrix);
    }
}
}  // namespace

namespace {
std::vector<IndexedMesh> index_meshes_(InputModel const &aModel,
                                       float aErrorTolerance) {
    std::vector<IndexedMesh> indexed;

    for (auto const &imesh : aModel.meshes) {
        auto const endIndex = imesh.vertexStartIndex + imesh.vertexCount;

        TriangleSoup soup;

        soup.vert.reserve(imesh.vertexCount);
        for (std::size_t i = imesh.vertexStartIndex; i < endIndex; ++i)
            soup.vert.emplace_back(aModel.positions[i]);

        soup.text.reserve(imesh.vertexCount);
        for (std::size_t i = imesh.vertexStartIndex; i < endIndex; ++i)
            soup.text.emplace_back(aModel.texcoords[i]);

        soup.norm.reserve(imesh.vertexCount);
        for (std::size_t i = imesh.vertexStartIndex; i < endIndex; ++i)
            soup.norm.emplace_back(aModel.normals[i]);

        indexed.emplace_back(make_indexed_mesh(soup, aErrorTolerance));
    }

    return indexed;
}
}  // namespace

namespace {
std::unordered_map<std::string, TextureInfo_> find_unique_textures_(
    InputModel const &aModel) {
    std::unordered_map<std::string, TextureInfo_> unique;

    std::uint32_t texid = 0;
    auto const add_unique_ = [&](std::string const &aPath, std::uint8_t aSpace,
                                 std::uint8_t aChannels) {
        if (aPath.empty()) return;

        TextureInfo_ info{};
        info.uniqueId = texid;
        info.space = aSpace;
        info.channels = aChannels;

        auto const [it, isNew] = unique.emplace(aPath, info);

        if (isNew) ++texid;
    };

    for (auto const &mat : aModel.materials) {
        add_unique_(mat.baseColorTexturePath, 1, 4);
        add_unique_(mat.roughnessTexturePath, 0, 1);
        add_unique_(mat.metalnessTexturePath, 0, 1);
        add_unique_(mat.alphaMaskTexturePath, 1, 4);  // assume == baseColor
        add_unique_(mat.normalMapTexturePath, 0, 3);  // xyz only
        add_unique_(mat.emissiveTexturePath, 1, 4);
    }

    return unique;
}

std::unordered_map<std::string, TextureInfo_> new_paths_(
    std::unordered_map<std::string, TextureInfo_> aTextures,
    std::filesystem::path const &aTexDir) {
    for (auto &entry : aTextures) {
        std::filesystem::path const originalPath(entry.first);
        auto const filename = originalPath.filename();
        auto const newpath = aTexDir / filename;

        auto &info = entry.second;
        info.newPath = newpath.string();
    }

    // Note: aTextures is still local to the function, so there is no need
    // to explicitly std::move() it. However, since it is passed in as an
    // argument, NRVO is unlikely to occur.
    return aTextures;
}
}  // namespace

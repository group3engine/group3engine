//
// Created by thomas on 06/02/25.
//

#include "TextureManager.hpp"

#include <algorithm>
#include <string>
#include <cctype>

#include <spdlog/spdlog.h>

#include "Utils.hpp"

namespace {
bool FindCaseInsensitive(const std::string &str, const std::string &subStr) {
    auto it = std::search(str.begin(), str.end(), subStr.begin(), subStr.end(),
                          [](auto ch1, auto ch2) {
                              return std::toupper(ch1) == std::toupper(ch2);
                          });
    return (it != str.end());
}
}

void TextureManager::addTexture(const std::filesystem::path &aTexturePath,
                                const std::string &aTextureName) {
    // check if the texture already exists, if it does, yay :)
    if (mTextureMap.find(aTextureName) != mTextureMap.end()) {
        return;
    }

    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

    bool isColor =
        FindCaseInsensitive(aTextureName, "color") || FindCaseInsensitive(aTextureName, "diffuse");

    bool isMetallic = FindCaseInsensitive(aTextureName, "metallic");
    bool isMetalness = FindCaseInsensitive(aTextureName, "metalness");
    bool isRoughness = FindCaseInsensitive(aTextureName, "roughness");
    bool isMetallicRoughness = isMetallic || isMetalness || isRoughness;

    bool isSpecular = FindCaseInsensitive(aTextureName, "specular");
    bool isGlossiness = FindCaseInsensitive(aTextureName, "glossiness");
    bool isSpecularGlossiness = isSpecular || isGlossiness;

    bool isNormal = FindCaseInsensitive(aTextureName, "normal");

    if (isColor) {
        format = VK_FORMAT_R8G8B8A8_SRGB;
    } else if (isMetallicRoughness || isSpecularGlossiness || isNormal) {
        format = VK_FORMAT_R8G8B8A8_UNORM;
    }
    // AND to check if at least one identifier exists
    else if ((isColor & isMetallicRoughness & isSpecularGlossiness & isNormal) == 0) {
        SPDLOG_ERROR(
            "Detected zero image identifiers in {}. Should contain either color, normal, "
            "metallic, metalness, roughness, specular, glossiness. Defaulting to sRGB color space.",
            aTexturePath.filename().string());
    }
    // XOR to make sure exactly one identifier exists
    else if ((isColor ^ isMetallicRoughness ^ isSpecularGlossiness ^ isNormal) == 0) {
        SPDLOG_ERROR(
            "Detected more than one image identifier in {}. Defaulting to sRGB color space.",
            aTexturePath.filename().string());
    } else {
        assert(false);
    }

    // load the texture
    Image textureImage = LoadTextureFromDisk(aTexturePath, mContext, format);
    Texture texture;
    texture.name = aTextureName;
    texture.image = std::move(textureImage);
    // add the texture to the map
    mTextureMap[aTextureName] = std::move(texture);
}

TextureManager::TextureManager(Context &aContext)
    : mContext(aContext) {
}

void TextureManager::Initialise() {
    // create a default (white) texture
    std::cout << assetsPath << std::endl;
    addTexture(assetsPath / "white_pixel.png", "white");
    // create a default normal texture
    addTexture(assetsPath / "default_normal.png", "normal");
    // create the command pool : TODO: do we need this?
    VkCommandPoolCreateInfo cmdPool{};
    cmdPool.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmdPool.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    cmdPool.queueFamilyIndex = mContext.graphicsFamilyIndex;

    mCommandPool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateCommandPool(mContext.device, &cmdPool, nullptr, &mCommandPool), "Failed to create command pool");
}

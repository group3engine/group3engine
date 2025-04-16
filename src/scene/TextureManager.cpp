//
// Created by thomas on 06/02/25.
//

#include "TextureManager.hpp"

#include "Utils.hpp"

void TextureManager::addTexture(const std::filesystem::path &aTexturePath,
                                const std::string &aTextureName) {
    // check if the texture already exists, if it does, yay :)
    if (mTextureMap.find(aTextureName) != mTextureMap.end()) {
        return;
    }
    // load the texture
    Image textureImage = LoadTextureFromDisk(
        aTexturePath, mContext,
        VK_FORMAT_R8G8B8A8_UNORM); // create the texture
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

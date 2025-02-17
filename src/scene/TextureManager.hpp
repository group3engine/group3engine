//
// Created by thomas on 06/02/25.
//

#ifndef VULKANTIME_TEXTUREMANAGER_HPP
#define VULKANTIME_TEXTUREMANAGER_HPP
#include <string>
#include <unordered_map>
#include <vector>

#include "GLTFImportStructs.hpp"
#include "../Context.hpp"
#include "../Image.hpp"

namespace vk {
class TextureManager {
   public:
    TextureManager(Context &aContext);
    ~TextureManager();

    void addTexture(const std::filesystem::path& aTexturePath, const std::string& aTextureName);

    Texture *GetTexture(std::string aName) { return &mTextureMap[aName]; }

   private:
    Context &mContext;
    VkCommandPool mCommandPool;

    // map of texture names to texture
    std::unordered_map<std::string, Texture> mTextureMap;
};

}  // namespace vk
#endif  // VULKANTIME_TEXTUREMANAGER_HPP

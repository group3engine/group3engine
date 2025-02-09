//
// Created by thomas on 06/02/25.
//

#ifndef VULKANTIME_TEXTUREMANAGER_HPP
#define VULKANTIME_TEXTUREMANAGER_HPP
#include "allocator.hpp"
#include "vkbuffer.hpp"
#include "vkobject.hpp"
#include "vkutil.hpp"
#include "vulkan_context.hpp"
#include "GLTFImportStructs.hpp"

#include <unordered_map>
#include <string>

#include <vector>

namespace lut = labutils;

class TextureManager {
  public:
    TextureManager(lut::VulkanContext const &aContext, lut::Allocator const &aAllocator)
        : mContext(aContext), mAllocator(aAllocator),
          mCommandPool(lut::create_command_pool(aContext, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT)) {}

    void addTexture(Image aCPUImage);

    Texture* GetTexture(std::string aName) {
        return &mTextureMap[aName];
    }

  private:
    lut::VulkanContext const &mContext;
    lut::Allocator const &mAllocator;
    lut::CommandPool mCommandPool;

    // map of texture names to texture
    std::unordered_map<std::string, Texture> mTextureMap;

};

#endif // VULKANTIME_TEXTUREMANAGER_HPP

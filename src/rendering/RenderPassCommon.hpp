#ifndef GROUP3ENGINE_RENDERPASSCOMMON_HPP
#define GROUP3ENGINE_RENDERPASSCOMMON_HPP

#include <array>
#include <vector>

#include "Volk.hpp"

#include "Config.hpp"

inline VkViewport CalcViewport(VkExtent2D extent, size_t playerCount, size_t playerId) {
    float width = 0.0f;
    float height = 0.0f;

    if (playerCount == 1) {
        width = static_cast<float>(extent.width);
        height = static_cast<float>(extent.height);
    } else if (playerCount == 2) {
        width = static_cast<float>(extent.width) / playerCount;
        height = static_cast<float>(extent.height);
    }

    float offsetX = 0.0f;
    float offsetY = 0.0f;
    if (playerId == 0) {
        offsetX = 0.0f;
        offsetY = 0.0f;
    } else if (playerId == 1) {
        offsetX = width;
    }

    VkViewport viewport{};
    viewport.x = offsetX;
    viewport.y = offsetY;
    viewport.width = width;
    viewport.height = height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    return viewport;
}

#endif // GROUP3ENGINE_RENDERPASSCOMMON_HPP

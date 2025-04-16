#ifndef GROUP3ENGINE_RENDERPASSCOMMON_HPP
#define GROUP3ENGINE_RENDERPASSCOMMON_HPP

#include <array>
#include <vector>

#include "Volk.hpp"

#include "Config.hpp"

struct ViewportSize {
    float width = 0.0f;
    float height = 0.0f;
};

inline ViewportSize CalcViewportSize(VkExtent2D extent, size_t playerCount, size_t playerId) {
    float width = 0.0f;
    float height = 0.0f;

    if (playerCount == 1) {
        width = static_cast<float>(extent.width);
        height = static_cast<float>(extent.height);
    } else if (playerCount == 2) {
        width = static_cast<float>(extent.width) / playerCount;
        height = static_cast<float>(extent.height);
    } else if (playerCount == 3) {
        if (playerId == 0) {
            width = static_cast<float>(extent.width);
            height = static_cast<float>(extent.height) / 2.0f;
        } else if (playerId == 1 || playerId == 2) {
            width = static_cast<float>(extent.width) / 2.0f;
            height = static_cast<float>(extent.height) / 2.0f;
        }
    } else if (playerCount == 4) {
        width = static_cast<float>(extent.width) / 2.0f;
        height = static_cast<float>(extent.height) / 2.0f;
    }

    return {width, height};
}

inline VkViewport CalcViewport(VkExtent2D extent, size_t playerCount, size_t playerId) {
    ViewportSize viewportSize = CalcViewportSize(extent, playerCount, playerId);

    float width = viewportSize.width;
    float height = viewportSize.height;

    float offsetX = 0.0f;
    float offsetY = 0.0f;

    if (playerCount == 2) {
        if (playerId == 0) {
            offsetX = 0.0f;
            offsetY = 0.0f;
        } else if (playerId == 1) {
            offsetX = width;
        }
    } else if (playerCount == 3) {
        if (playerId == 0) {
            offsetX = 0.0f;
            offsetY = 0.0f;
        } else if (playerId == 1) {
            offsetX = 0;
            offsetY = height;
        } else if (playerId == 2) {
            offsetX = width;
            offsetY = height;
        }
    } else if (playerCount == 4) {
        if (playerId == 0) {
            offsetX = 0.0f;
            offsetY = 0.0f;
        } else if (playerId == 1) {
            offsetX = width;
            offsetY = 0;
        } else if (playerId == 2) {
            offsetX = 0;
            offsetY = height;
        } else if (playerId == 3) {
            offsetX = width;
            offsetY = height;
        }
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

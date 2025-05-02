#ifndef GROUP3ENGINE_CONFIG_HPP
#define GROUP3ENGINE_CONFIG_HPP

#include <cstddef>

namespace GlobalConfig {

// Global toggles
#ifdef JPH_DEBUG_RENDERER
extern bool enablePhysicsDebugRenderer;
#endif // JPH_DEBUG_RENDERER

constexpr size_t maxPlayers = 4;
static_assert(maxPlayers >= 1 && maxPlayers <= 4);


} // namespace GlobalConfig
#define NUM_DRAW_THREADS 1

#endif // GROUP3ENGINE_CONFIG_HPP

#ifndef GROUP3ENGINE_CONFIG_HPP
#define GROUP3ENGINE_CONFIG_HPP

#include <cstddef>

namespace GlobalConfig {

// Global toggles
#ifdef JPH_DEBUG_RENDERER
extern bool enablePhysicsDebugRenderer;
#endif // JPH_DEBUG_RENDERER

constexpr size_t maxPlayers = 2;

} // namespace GlobalConfig

#endif // GROUP3ENGINE_CONFIG_HPP

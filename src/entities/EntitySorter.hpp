//
// Created by thomas on 10/03/25.
//

#ifndef GROUP3ENGINE_ENTITYSORTER_HPP
#define GROUP3ENGINE_ENTITYSORTER_HPP
#include "Bouncepad.hpp"
#include "Entity.hpp"
#include "MovingEntity.hpp"
#include "CharacterEntity.hpp"
#include "RotatingPlatform.hpp"
#include "RotateOnX.hpp"
#include "SampleSecondaryEntity.hpp"
#include "ParticleCube.hpp"
#include "NetworkCharacterManager.hpp"
#include "NetworkedLocalCharacter.hpp"
#include "NetworkedCharacterRemote.hpp"
#include "Arrow.hpp"
#include "TileBox.hpp"
#include "TileManager.hpp"
#include "Sinking.hpp"
#include "SinkingChild.hpp"
#include "BoulderSpawner.hpp"
#include "SpikeTrap.hpp"
#include "SwingAxe.hpp"
#include "Lever.hpp"
#include "Trapdoor.hpp"
#include "Idol.hpp"
#include "SpawnPortal.hpp"
// Add more includes here

// an enum of all the different entity types
enum class EntityType {
    DEFAULT,
    CHARACTER,
    MOVING,
    ROTATING,
    SPINNINGONX,
    SECONDCHARACTER,
    CAMERA,
    PARTICLES,
    BOUNCEPAD,
    NETWORKEDLOCALCHARACTER,
    NETWORKEDCHARACTERREMOTE,
    NETWORKCHARACTERMANAGER,
    ARROW,
    TILEBOX,
    TILEMANAGER,
    SINKING,
    SINKINGCHILD,
    SWINGAXE,
    LEVER,
    TRAPDOOR,
    BOULDERSPAWNER,
    SPIKETRAP,
    IDOL,
    SPAWNPORTAL
    // Add more entity types here
};
// a map of strings to entity types
static const std::unordered_map<std::string, EntityType> entityTypeMap = {
    {"default", EntityType::DEFAULT},
    {"character", EntityType::CHARACTER},
    {"movingTest", EntityType::MOVING},
    {"rotatingPlatform", EntityType::ROTATING},
    {"SpinningOnX", EntityType::SPINNINGONX},
    {"second", EntityType::SECONDCHARACTER},
    {"bouncepad", EntityType::BOUNCEPAD},
    {"camera", EntityType::CAMERA},
    {"particles", EntityType::PARTICLES},
    {"networkedlocal", EntityType::NETWORKEDLOCALCHARACTER},
    {"networkedremote", EntityType::NETWORKEDCHARACTERREMOTE},
    {"networkmanager", EntityType::NETWORKCHARACTERMANAGER},
    {"arrow",  EntityType::ARROW},
    {"tileBox", EntityType::TILEBOX},
    {"tileManager", EntityType::TILEMANAGER},
    {"sinking", EntityType::SINKING},
    {"sinkingChild", EntityType::SINKINGCHILD},
    {"swing_axe", EntityType::SWINGAXE},
    {"lever", EntityType::LEVER},
    {"trapdoor", EntityType::TRAPDOOR},
    {"boulderSpawner", EntityType::BOULDERSPAWNER},
    {"spikeTrap", EntityType::SPIKETRAP},
    {"idol", EntityType::IDOL},
    {"spawn_portal", EntityType::SPAWNPORTAL}
};
// a function to convert a string to an entity type
inline EntityType GetEntityTypeFromString(const std::string& aTypeName) {
    auto it = entityTypeMap.find(aTypeName);
    if (it != entityTypeMap.end()) {
        return it->second;
    }
    return EntityType::DEFAULT; // Default if type not found
}

// this function should return an entity pointer, selected from the different entity types by the string given
inline Entity* CreateNewEntity(const std::string& aEntityType)
{
    EntityType entityType = GetEntityTypeFromString(aEntityType);

    switch(entityType) {
    case EntityType::DEFAULT:
        return new Entity();
    case EntityType::CHARACTER:
        return new CharacterEntity();
    case EntityType::MOVING:
        return new MovingEntity();
    case EntityType::ROTATING:
        return new RotatingPlatform(0.3f);
    case EntityType::SPINNINGONX:
        return new RotateOnX(10.f);
    case EntityType::SECONDCHARACTER:
        return new SampleSecondaryEntity();
    case EntityType::CAMERA:
        SPDLOG_ERROR("Cannot create Camera entity using CreateNewEntity.");
        exit(EXIT_FAILURE);
    case EntityType::PARTICLES:
        return new ParticleCube();
    case EntityType::BOUNCEPAD:
        return new Bouncepad();
    case EntityType::NETWORKEDLOCALCHARACTER:
        return new NetworkedLocalCharacter();
    case EntityType::NETWORKEDCHARACTERREMOTE:
        return new NetworkedCharacterRemote();
    case EntityType::NETWORKCHARACTERMANAGER:
        return new NetworkCharacterManager();
    case EntityType::ARROW:
        return new Arrow();
    case EntityType::TILEBOX:
        return new TileBox();
    case EntityType::TILEMANAGER:
        return new TileManager();
    case EntityType::SINKING:
        return new Sinking();
    case EntityType::SINKINGCHILD:
        return new SinkingChild();
    case EntityType::SWINGAXE:
        return new SwingAxe();
    case EntityType::LEVER:
        return new Lever();
    case EntityType::TRAPDOOR:
        return new Trapdoor();
    case EntityType::BOULDERSPAWNER:
        return new BoulderSpawner();
    case EntityType::SPIKETRAP:
        return new SpikeTrap();
    case EntityType::IDOL:
        return new Idol();
    case EntityType::SPAWNPORTAL:
        return new SpawnPortal();
    // Add more cases here
    default:
        assert(false);
        return new Entity();
    }
}


#endif // GROUP3ENGINE_ENTITYSORTER_HPP

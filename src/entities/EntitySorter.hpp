//
// Created by thomas on 10/03/25.
//

#ifndef GROUP3ENGINE_ENTITYSORTER_HPP
#define GROUP3ENGINE_ENTITYSORTER_HPP
#include "Entity.hpp"
#include "MovingEntity.hpp"
#include "CharacterEntity.hpp"
// Add more includes here

// an enum of all the different entity types
enum class EntityType {
    DEFAULT,
    CHARACTER,
    MOVING
};
// a map of strings to entity types
static const std::unordered_map<std::string, EntityType> entityTypeMap = {
    {"default", EntityType::DEFAULT},
    {"character", EntityType::CHARACTER},
    {"movingTest", EntityType::MOVING}
};
// a function to convert a string to an entity type
const EntityType GetEntityTypeFromString(const std::string& aTypeName) {
    auto it = entityTypeMap.find(aTypeName);
    if (it != entityTypeMap.end()) {
        return it->second;
    }
    return EntityType::DEFAULT; // Default if type not found
}

// this function should return an entity pointer, selected from the different entity types by the string given
Entity* CreateNewEntity(const std::string& aEntityType)
{
    EntityType entityType = GetEntityTypeFromString(aEntityType);

    switch(entityType) {
        case EntityType::DEFAULT:
            return new Entity();
        case EntityType::CHARACTER:
            return new CharacterEntity();
        case EntityType::MOVING:
            return new MovingEntity();
        default:
            return new Entity();
    }
}


#endif // GROUP3ENGINE_ENTITYSORTER_HPP

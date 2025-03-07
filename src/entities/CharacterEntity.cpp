//
// Created by thomas on 07/03/25.
//

#include "CharacterEntity.hpp"
void CharacterEntity::SetCharacterVirtual(unique_ptr<CharacterVirtualTest> &&uniquePtr) {
    mCharacterVirtual = std::move(uniquePtr);
}
CharacterEntity::~CharacterEntity() {
    // call the destructor of the parent class
    Entity::~Entity();
}

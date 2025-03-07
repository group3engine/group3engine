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
void CharacterEntity::Update(double deltaTime) {
    Entity::Update(deltaTime);
    // for each child, if there is an animator, call set animation
        for (auto &child : mChildren) {
                if (child->HasAnimator()) {
                    child->GetAnimator().SetActiveAnimation("running", 0.1f);
                }
        }
}

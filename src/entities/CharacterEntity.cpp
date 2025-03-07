//
// Created by thomas on 07/03/25.
//

#include "CharacterEntity.hpp"
#include <spdlog/spdlog.h>

void CharacterEntity::SetCharacterVirtual(unique_ptr<CharacterVirtualTest> &&uniquePtr) {
    mCharacterVirtual = std::move(uniquePtr);

}


CharacterEntity::~CharacterEntity() {
    // call the destructor of the parent class
    Entity::~Entity();
}
void CharacterEntity::Update(double deltaTime) {
    if(!mHasFirstFrameHappened) {
        mInitialTransform = GetTransform();
        mHasFirstFrameHappened = true;
    }

    Entity::Update(deltaTime);
    // get the character state
    // calculate the delta velocity
    glm::vec3 deltaVelocity = -1.f * mDeltaPosition / deltaTime;
    // set the character to face the direction of the velocity without the y component
    deltaVelocity.y = 0;
    if (glm::length(deltaVelocity) > 0.1f) {
        // set the transform rotation to the direction of the velocity, on top of the initial transform rotation
        Transform newTransform = GetTransform();
        newTransform.rotation = glm::quatLookAt(glm::normalize(deltaVelocity), glm::vec3(0, 1, 0)) * mInitialTransform.rotation;
        SetTransform(newTransform);
    }
    // work out the active animation, and the time scale
    float timeScale = 1.0f;
    std::string activeAnimation = "idle";
    float blend = 0.1f;
    bool playWholeAnimation = false;
    if(glm::length(deltaVelocity) > 0.1f) {
        activeAnimation = "running";
        timeScale = min(glm::length(deltaVelocity) / 5.5f, 2.f);
    }
    // spdlog the current jump state
    switch (mCharacterVirtual->GetJumpState()) {
    case EJumpState::Start:
        activeAnimation = "jump up";
        playWholeAnimation = false;
        timeScale = 1.0f;
        break;
    case EJumpState::Falling:
        activeAnimation = "falling";
        timeScale = 1.0f;
        blend = 0.5f;
        playWholeAnimation = false;
        break;
    case EJumpState::End:
        break;
    case EJumpState::None:
        break;
    }
    // for each child, if there is an animator, call set animation
    for (auto &child : mChildren) {
            if (child->HasAnimator()) {
                child->GetAnimator().SetActiveAnimation(activeAnimation, blend, playWholeAnimation);
                child->GetAnimator().SetTimeScale(timeScale);
            }
    }
}
CharacterEntity::CharacterEntity() {
    mHasCharacter = true;
}

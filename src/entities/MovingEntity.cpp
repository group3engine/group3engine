//
// Created by thomas on 10/03/25.
//

#include "MovingEntity.hpp"
void MovingEntity::Update(double deltaTime) {
    if(!mHasFirstFrameHappened)
    {
        start_position = GetTransform().translation;
        end_position = start_position + glm::vec3(0, 0, 10);
        mHasFirstFrameHappened = true;
    }
    Entity::Update(deltaTime);
    timeElapsed += deltaTime;
    timeElapsed = fmod(timeElapsed, timeToMove);
    float t = timeElapsed / timeToMove;
    // update the local transform
    Transform newTransform = GetTransform();
    newTransform.translation = glm::mix(start_position, end_position, t);
    SetTransform(newTransform);
}

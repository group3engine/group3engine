//
// Created by thomas on 26/04/25.
//

#include "DisappearingPlatform.hpp"

void DisappearingPlatform::Awake()
{
    // get the time to step
    if (auto it = mFloatProperties.find("timeToStep"); it != mFloatProperties.end()) {
        timeToStep = it->second;
    } else {
        SPDLOG_ERROR("Disappearing platform does not have a timeToStep property.");
        exit(EXIT_FAILURE);
    }
    // get the time to reset
    if (auto it = mFloatProperties.find("timeToReset"); it != mFloatProperties.end()) {
        timeToReset = it->second;
    } else {
        SPDLOG_ERROR("Disappearing platform does not have a timeToReset property.");
        exit(EXIT_FAILURE);
    }
    // get the time to shrink
    if (auto it = mFloatProperties.find("timeToShrink"); it != mFloatProperties.end()) {
        timeToShrink = it->second;
    } else {
        SPDLOG_ERROR("Disappearing platform does not have a timeToShrink property.");
        exit(EXIT_FAILURE);
    }
    // get the start scale
    startScale = GetWorldTransformComponents().scale;


}

void DisappearingPlatform::Update(double deltaTime)
{
    float scaleMultiplier = 1.f;
    switch (mState) {
        case (DisappearingPlatformState::IDLE):
            timer += deltaTime;
            scaleMultiplier = std::min(timer / timeToShrink, 1.f);
            break;
            // if the state is stepping, update the timer
        case (DisappearingPlatformState::STEPPING):
            timer += deltaTime;
            if (timer >= timeToStep) {
                mState = DisappearingPlatformState::DISAPPEARING;
                timer = 0.f;
            }
            break;
        case (DisappearingPlatformState::DISAPPEARING):
            timer += deltaTime;
            scaleMultiplier = std::max(1.f - (timer / timeToShrink), 0.f);
            if (timer >= timeToStep) {
                mState = DisappearingPlatformState::RESETTING;
                timer = 0.f;
            }
            break;
        case (DisappearingPlatformState::RESETTING):
            timer += deltaTime;
            scaleMultiplier = 0.f;
            if (timer >= timeToReset) {
                mState = DisappearingPlatformState::IDLE;
                timer = 0.f;
            }
            break;
    }
    // set the scale of the entity
    glm::vec3 scale = startScale * scaleMultiplier;
    Transform transform = GetLocalTransform();
    transform.scale = scale;
    SetTransform(transform);
    // if the scale isn't 1, turn off the rigid body
    if (scaleMultiplier < 1.f) {
        GetRigidBody().SetActive(false);
    } else {
        GetRigidBody().SetActive(true);
    }


}

void DisappearingPlatform::OnCollisionStart(Entity *other)
{
    // check that the other entity is a player
    if (other->IsCharacter())
    {
        // set the state to stepping
        mState = DisappearingPlatformState::STEPPING;
        // set the timer to 0
        timer = 0.f;
    }
}

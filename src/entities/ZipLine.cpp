//
// Created by thomas on 26/04/25.
//

#include "ZipLine.hpp"

#include "Camera.hpp"


void ZipLine::Awake()
{
    // get the float values - mMaxZipSpeed, mAcceleration, mProximityPromptRadius
    if (auto it = mFloatProperties.find("maxZipSpeed"); it != mFloatProperties.end()) {
        mMaxZipSpeed = it->second;
    } else {
        SPDLOG_ERROR("ZipLine does not have a max zip speed property.");
        exit(EXIT_FAILURE);
    }
    if (auto it = mFloatProperties.find("acceleration"); it != mFloatProperties.end()) {
        mAcceleration = it->second;
    } else {
        SPDLOG_ERROR("ZipLine does not have an acceleration property.");
        exit(EXIT_FAILURE);
    }
    auto proximityPrompt = mFloatProperties.find("proximity_prompt");
    if (proximityPrompt != mFloatProperties.end()) {
        mProximityPromptRadius = proximityPrompt->second;
    } else {
        SPDLOG_ERROR("Idol missing required property.");
        exit(EXIT_FAILURE);
    }


    // get the start and end positions
    // find the child with the tag "zipline_start"
    for (auto *child : GetChildren()) {

        if (child->CompareTag("zipline_start")) {
            mStartPosition = child->GetWorldTransformComponents().translation;
        } else if (child->CompareTag("zipline_end")) {
            mEndPosition = child->GetWorldTransformComponents().translation;
        }
    }
    mDirection = mEndPosition - mStartPosition;
    mDistance = glm::length(mDirection);

    // Init proximity prompt sensor
    // TODO: Factor this out into a class so we can programatically create proximity sensors given a
    // proximity_prompt float property
    glm::vec3 translation = mStartPosition;
    BodyCreationSettings sensorSettings(new SphereShape(mProximityPromptRadius),
                                        Vec3(translation.x, translation.y, translation.z),
                                        Quat::sIdentity(), EMotionType::Static, Layers::MOVING);
    SPDLOG_INFO("Idol position {}", glm::to_string(translation));
    sensorSettings.mIsSensor = true;

    mSensor = std::make_unique<RigidBody>(sensorSettings);

    mSensor->Init(PhysicsManager::get(), false);
    PhysicsManager::get().RegisterEntity(this, mSensor->mBodyId);
}

void ZipLine::OnInteract(Entity *other, ENetworkLocality networkLocality)
{
    // if the other entity is a character, start zipping
    if (other->CompareType("character") || other->CompareType("NetworkedLocalCharacter")) {
        mCharacter = static_cast<CharacterEntity *>(other);
        mIsZipping = true;
        mCharacter->SetPosition(mStartPosition);
        mCharacter->SetHanging(true);
        mCharacter->GetCamera()->SetNewZoomLevel(sZiplineCameraZoomLevel);

        mCurrentPosition = mStartPosition;
    }
}

void ZipLine::LateUpdate(double deltaTime)
{
    // if we are zipping, zip
    if(mIsZipping)
    {
        // accelerate
        if (mCurrentSpeed < mMaxZipSpeed) {
            mCurrentSpeed += mAcceleration * deltaTime;
        } else {
            mCurrentSpeed = mMaxZipSpeed;
        }
        // move the character in the direction of the zipline
        mCurrentPosition += mDirection * static_cast<float>(mCurrentSpeed * deltaTime);
        mCharacter->SetPosition(mCurrentPosition);
        // if we are at the end of the zipline, stop zipping
        if (glm::distance(mCurrentPosition, mStartPosition) > mDistance) {
            mIsZipping = false;
            mCharacter->SetHanging(false);
            mCharacter->GetCamera()->ResetZoomLevel();

            mCurrentSpeed = 0.f;
            mCharacter->SetPosition(mEndPosition);
        }
    }
}

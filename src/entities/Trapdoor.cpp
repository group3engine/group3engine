#include "Trapdoor.hpp"

Trapdoor::Trapdoor() {
    mType = "trapdoor";
}

void Trapdoor::Awake() {
    // Trapdoor should be the child of something that controls it

    // Find the angle of the trapdoor when it is activated
    auto activationAngle = mFloatProperties.find("activation_angle");
    auto animationTime = mFloatProperties.find("animation_time");
    if (activationAngle != mFloatProperties.end() && animationTime != mFloatProperties.end()) {
        mActivationAngle = JPH::DegreesToRadians(activationAngle->second);
        mAnimationTime = animationTime->second;
    } else {
        SPDLOG_ERROR("Trapdoor handle missing required property.");
        exit(EXIT_FAILURE);
    }

    // Scoped lock
    {
        JPH::BodyLockRead lock(mLockInterface, GetRigidBody().mBodyId);

        const JPH::Body &trapdoorBody = lock.GetBody();

        mAxisX = trapdoorBody.GetWorldTransform().GetAxisX().Normalized();
        mAxisY = trapdoorBody.GetWorldTransform().GetAxisY().Normalized();
        mAxisZ = trapdoorBody.GetWorldTransform().GetAxisZ().Normalized();

        mInitialRotation = trapdoorBody.GetRotation();
        mFinalRotation = JPH::Quat::sRotation(mAxisZ, mActivationAngle) * trapdoorBody.GetRotation();
    }
}

void Trapdoor::Update(double deltaTime) {
    if (mCurrentAnimationTime >= mAnimationTime) {
        // TODO: Refactor this code duplication? If the current animation time has gone past the
        // total animation time then the last part of the animation needs to finish
        float fraction = JPH::Clamp(mCurrentAnimationTime / mAnimationTime, 0.0f, 1.0f);
        JPH::Quat rotation = mInitialRotation.SLERP(mFinalRotation, fraction);
        GetRigidBody().SetRotationJolt(rotation.Normalized());

        mIsActivated = false;
        mCurrentAnimationTime = 0.0f;
        // Swap between initial and final rotation so the door can be opened and closed
        std::swap(mInitialRotation, mFinalRotation);
    }

    if (mIsActivated) {
        float fraction = JPH::Clamp(mCurrentAnimationTime / mAnimationTime, 0.0f, 1.0f);
        JPH::Quat rotation = mInitialRotation.SLERP(mFinalRotation, fraction);
        GetRigidBody().SetRotationJolt(rotation.Normalized());

        mCurrentAnimationTime += deltaTime;
    }
}

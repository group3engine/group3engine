//
// Created by thomas on 10/03/25.
//

#include "MovingEntity.hpp"

#include "Scene.hpp"

void MovingEntity::Update(double deltaTime) {
    Entity::Update(deltaTime);

    mCurrentTime += deltaTime;

    if (mCurrentTime > mTime) {
        mVelocity = -mVelocity;
        // std::swap(mInitialPosition, mFinalPosition);
        mCurrentTime = 0.0f;
    }

    GetRigidBody().SetLinearVelocity({mVelocity.GetX(), mVelocity.GetY(), mVelocity.GetZ()});

#ifdef JPH_DEBUG_RENDERER
    // Draw initial position
    GetScene()->GetDebugRenderer()->DrawSphere(mInitialPosition, 0.1f, Color::sOrange);
    // Draw final position
    GetScene()->GetDebugRenderer()->DrawSphere(mFinalPosition, 0.1f, Color::sCyan);
#endif // JPH_DEBUG_RENDERER
}

void MovingEntity::Awake() {
    Vec3 displacement;
    // Find float properties
    auto time = mFloatProperties.find("time");
    auto moveX = mFloatProperties.find("moveX");
    auto moveY = mFloatProperties.find("moveY");
    auto moveZ = mFloatProperties.find("moveZ");
    if (time != mFloatProperties.end() &&
        moveX != mFloatProperties.end() &&
        moveY != mFloatProperties.end() &&
        moveZ != mFloatProperties.end()) {
        mTime = time->second;
        displacement = {moveX->second, moveY->second, moveZ->second};
    } else {
        SPDLOG_ERROR("Moving entity missing required property.");
        exit(EXIT_FAILURE);
    }

    // Scoped lock
    {
        JPH::BodyLockRead lock(mLockInterface, GetRigidBody().mBodyId);

        const JPH::Body &body = lock.GetBody();

        mAxisX = body.GetWorldTransform().GetAxisX().Normalized();
        mAxisY = body.GetWorldTransform().GetAxisY().Normalized();
        mAxisZ = body.GetWorldTransform().GetAxisZ().Normalized();

        mInitialPosition = body.GetPosition();

        Vec4 C1 = {mAxisX.GetX(), mAxisX.GetY(), mAxisX.GetZ(), 0};
        Vec4 C2 = {mAxisY.GetX(), mAxisY.GetY(), mAxisY.GetZ(), 0};
        Vec4 C3 = {mAxisZ.GetX(), mAxisZ.GetY(), mAxisZ.GetZ(), 0};
        Vec4 C4 = {0, 0, 0, 1};
        RMat44 changeOfBasis = {C1, C2, C3, C4};

        mDisplacement = changeOfBasis * displacement;
        mVelocity = mDisplacement / mTime;

        mFinalPosition = mInitialPosition + mDisplacement;
    }
}

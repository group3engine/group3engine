#include "SwingAxe.hpp"

SwingAxe::SwingAxe() {
    mType = "swing_axe";
}

void SwingAxe::Awake() {
    // Control how fast the pendulum swings
    if (auto it = mFloatProperties.find("pendulum_length"); it != mFloatProperties.end()) {
        mPendulumLength = it->second;
    } else {
        SPDLOG_ERROR("Swing axe does not have a pendulum length property.");
        exit(EXIT_FAILURE);
    }

    // Scoped lock
    {
        JPH::BodyLockRead lock(mLockInterface, GetRigidBody().mBodyId);

        const JPH::Body &axeBody = lock.GetBody();

        // Swing on axis of swing axe body
        mAxisX = axeBody.GetWorldTransform().GetAxisX().Normalized();
        mAxisY = axeBody.GetWorldTransform().GetAxisY().Normalized();
        mAxisZ = axeBody.GetWorldTransform().GetAxisZ().Normalized();

        initialRotation = axeBody.GetRotation();
    }
}

void SwingAxe::Update(double deltaTime) {
    mTime += deltaTime;

    JPH::Quat rotation = Quat::sRotation(mAxisX, JPH_PI / 2.0f * std::cos(mPendulumLength * mTime));
    JPH::Quat newRotation = rotation * initialRotation;
    GetRigidBody().SetRotationJolt(newRotation.Normalized());
}

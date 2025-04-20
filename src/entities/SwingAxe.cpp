#include "SwingAxe.hpp"

#include <Jolt/Physics/Body/BodyLockMulti.h>

void SwingAxe::InitPhysics() {
    GetRigidBody().Init(PhysicsManager::get(), true);

    PhysicsManager::get().RegisterEntity(this, GetRigidBody().mBodyId);

    mAngleStart = M_PI / 2;
    // Control how fast the pendulum swings
    if (mFloatProperties.contains("pendulum_length")) {
        mPendulumLength = mFloatProperties["pendulum_length"];
    }
    // Pendulum mass does not matter in this simulation, mass cancels out
    mPendulumMass = 1.0f;
    mInertia = mPendulumMass * mPendulumLength*mPendulumLength;
    mAngularSpeed = 0.0f;

    // Scoped lock
    {
        JPH::BodyLockWrite lock(mLockInterface, GetRigidBody().mBodyId);

        JPH::Body &axeBody = lock.GetBody();

        // Swing on axis of swing axe body
        mAxisX = axeBody.GetWorldTransform().GetAxisX().Normalized();
        mAxisY = axeBody.GetWorldTransform().GetAxisY().Normalized();
        mAxisZ = axeBody.GetWorldTransform().GetAxisZ().Normalized();

        JPH::Quat rotation = Quat::sRotation(mAxisX, mAngleStart);
        JPH::Quat newRotation = rotation * axeBody.GetRotation();
        GetRigidBody().SetRotationJolt(newRotation.Normalized());
    }
}

void SwingAxe::Update(double deltaTime) {
    JPH::Vec3 gravity = mPhysicsSystem.GetGravity();

    JPH::Vec3 pendulumUp;
    {
        JPH::BodyLockRead lock(mLockInterface, GetRigidBody().mBodyId);
        pendulumUp = (lock.GetBody().GetWorldTransform().GetAxisY()).Normalized();
    }

    JPH::Vec3 torque = pendulumUp.Cross(mPendulumMass * -gravity) * deltaTime;

    mAngularSpeed += torque.GetX() / mInertia * deltaTime;

    JPH::Quat rotation = Quat::sRotation(mAxisX, mAngularSpeed);
    JPH::Quat newRotation = rotation * GetRigidBody().GetRotation();
    GetRigidBody().SetRotationJolt(newRotation.Normalized());
}

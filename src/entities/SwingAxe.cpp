#include "SwingAxe.hpp"

#include <Jolt/Physics/Body/BodyLockMulti.h>

#include "Scene.hpp"
#include "SwingAxeHinge.hpp"

void SwingAxe::InitPhysics() {
    GetRigidBody().Init(PhysicsManager::get(), true);

    PhysicsManager::get().RegisterEntity(this, GetRigidBody().mBodyId);

    // Get body of swing axe hinge
    SwingAxeHinge *swingAxeHinge = nullptr;

    // See Jolt pendulum example:
    // https://github.com/wedesoft/jolttest/blob/main/pendulum.cc#L278

    // Find swing axe hinge entity
    auto &entities = GetScene()->GetEntities();
    for (auto *entity : entities) {
        if (entity->CompareType("swing_axe_hinge")) {
            SPDLOG_INFO("Found swing axe hinge");
            swingAxeHinge = static_cast<SwingAxeHinge*>(entity);
        }
    }

    JPH::PhysicsSystem &physicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &lockInterface = physicsSystem.GetBodyLockInterface();

    JPH::BodyID hingeBodyId = swingAxeHinge->GetRigidBody().mBodyId;
    JPH::BodyID axeBodyId = GetRigidBody().mBodyId;
    std::vector<JPH::BodyID> bodyIds = {hingeBodyId, axeBodyId};

    // Scoped lock
    {
        JPH::BodyLockMultiWrite lock(lockInterface, bodyIds.data(), bodyIds.size());

        JPH::Body *hingeBody = lock.GetBody(0);
        JPH::Body *axeBody = lock.GetBody(1);

        // Swing on axis of swing axe body
        Vec3 axisX = (axeBody->GetWorldTransform().GetAxisX()).Normalized();
        Vec3 axisY = (axeBody->GetWorldTransform().GetAxisY()).Normalized();
        Vec3 axisZ = (axeBody->GetWorldTransform().GetAxisZ()).Normalized();

        JPH::HingeConstraintSettings hinge;
        hinge.mSpace = EConstraintSpace::WorldSpace;
        hinge.mPoint1 = hinge.mPoint2 = hingeBody->GetPosition();
        hinge.mHingeAxis1 = hinge.mHingeAxis2 = axisY;
        hinge.mNormalAxis1 = hinge.mNormalAxis2 = axisX;

        mConstraint = static_cast<JPH::HingeConstraint*>(hinge.Create(*hingeBody, *axeBody));
        mConstraint->SetMotorState(EMotorState::Velocity);
        mConstraint->SetTargetAngularVelocity(DegreesToRadians(100));

        physicsSystem.AddConstraint(mConstraint);
    }
}

void SwingAxe::Update(double deltaTime) {
    float angle = mConstraint->GetCurrentAngle();
    SPDLOG_INFO("angle {}", angle);
    if (angle <= -JPH_PI / 4.0f) {
        mConstraint->SetTargetAngularVelocity(-mConstraint->GetTargetAngularVelocity());
        SPDLOG_INFO("Reverse");
    }
    swingTime += deltaTime;
}

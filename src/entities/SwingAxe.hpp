#ifndef GROUP3ENGINE_SWINGAXE_HPP
#define GROUP3ENGINE_SWINGAXE_HPP

#include "Entity.hpp"

#include <Jolt/Physics/Constraints/HingeConstraint.h>

class SwingAxe : public Entity {
  public:
    void InitPhysics() override;

    // void Awake() override;

    void Update(double deltaTime) override;

    PhysicsSystem &mPhysicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &mLockInterface = mPhysicsSystem.GetBodyLockInterface();

    Vec3 mAxisX;
    Vec3 mAxisY;
    Vec3 mAxisZ;

    float mAngleStart = 0.0f;

    float mPendulumLength = 0.0f;
    float mPendulumMass = 0.0f;
    float mInertia = 0.0f;
    float mAngularSpeed = 0.0f;
};
#endif // GROUP3ENGINE_SWINGAXE_HPP

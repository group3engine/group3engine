#ifndef GROUP3ENGINE_SWINGAXE_HPP
#define GROUP3ENGINE_SWINGAXE_HPP

#include "Entity.hpp"

class SwingAxe : public Entity {
  public:
    SwingAxe();

    void Awake() override;

    void Update(double deltaTime) override;

    PhysicsSystem &mPhysicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &mLockInterface = mPhysicsSystem.GetBodyLockInterface();

    float mPendulumLength = 0.0f;
    float mAngle = 0.0f;

    Vec3 mAxisX;
    Vec3 mAxisY;
    Vec3 mAxisZ;

    JPH::Quat initialRotation;

    float mTime = 0.0f;
};
#endif // GROUP3ENGINE_SWINGAXE_HPP

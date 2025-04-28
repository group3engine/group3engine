//
// Created by thomas on 10/03/25.
//

#ifndef GROUP3ENGINE_MOVINGENTITY_HPP
#define GROUP3ENGINE_MOVINGENTITY_HPP

#include "Entity.hpp"

class MovingEntity : public Entity{
  public:
    MovingEntity() = default;

    void Awake() override;

    void Update(double deltaTime) override;

    PhysicsSystem &mPhysicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &mLockInterface = mPhysicsSystem.GetBodyLockInterface();

  private:
    float mTime = 0.0f;
    Vec3 mDisplacement;
    Vec3 mVelocity;

    JPH::Vec3 mAxisX;
    JPH::Vec3 mAxisY;
    JPH::Vec3 mAxisZ;

    Vec3 mInitialPosition;
    Vec3 mFinalPosition;

    float mCurrentTime = 0.0f;
};

#endif // GROUP3ENGINE_MOVINGENTITY_HPP

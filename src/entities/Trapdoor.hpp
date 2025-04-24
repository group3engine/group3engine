#ifndef GROUP3ENGINE_TRAPDOOR_HPP
#define GROUP3ENGINE_TRAPDOOR_HPP

#include "Entity.hpp"

class Trapdoor : public Entity {
  public:
    Trapdoor();

    void Awake() override;

    void Update(double deltaTime) override;

    bool IsActivated() const { return mIsActivated; }
    void Activate() { mIsActivated = true; };

  public:
    PhysicsSystem &mPhysicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &mLockInterface = mPhysicsSystem.GetBodyLockInterface();

    float mActivationAngle = 0.0f;
    float mAnimationTime = 0.0f;

    JPH::Vec3 mAxisX;
    JPH::Vec3 mAxisY;
    JPH::Vec3 mAxisZ;

    JPH::Quat mInitialRotation;
    JPH::Quat mFinalRotation;

    float mCurrentAnimationTime = 0.0f;

    bool mIsActivated = false;
};
#endif // GROUP3ENGINE_TRAPDOOR_HPP

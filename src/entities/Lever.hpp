#ifndef GROUP3ENGINE_LEVER_HPP
#define GROUP3ENGINE_LEVER_HPP

#include "Entity.hpp"

#include "Trapdoor.hpp"

class Lever : public Entity {
  public:
    Lever();

    void Awake() override;

    void Update(double deltaTime) override;

    PhysicsSystem &mPhysicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &mLockInterface = mPhysicsSystem.GetBodyLockInterface();

    Entity *mLeverHandle = nullptr;
    std::vector<Trapdoor *> mTrapdoors;

    // Float properties
    float mMinAngle = 0.0f;
    float mMaxAngle = 0.0f;
    float mAnimationTime = 0.0f;

    JPH::Vec3 mAxisX;
    JPH::Vec3 mAxisY;
    JPH::Vec3 mAxisZ;

    JPH::Quat mInitialRotation;
    JPH::Quat mFinalRotation;

    float mCurrentAnimationTime = 0.0f;

    bool mIsPulled = false;
};
#endif // GROUP3ENGINE_LEVER_HPP

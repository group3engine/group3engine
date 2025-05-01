#ifndef GROUP3ENGINE_LEVER_HPP
#define GROUP3ENGINE_LEVER_HPP

#include "Entity.hpp"

#include "NetworkSignals.hpp"
#include "Trapdoor.hpp"

class Lever : public Entity {
  public:
    Lever();

    void Awake() override;

    void OnInteract(Entity *other, ENetworkLocality networkLocality) override;

    void OnNetworkInteract(NetworkLeverSignal *signal);

    void Update(double deltaTime) override;

    EInteractable IsInteractable() const override {
        return mIsPulled ? EInteractable::NotInteractable : EInteractable::Interactable;
    }

    PhysicsSystem &mPhysicsSystem = PhysicsManager::get().mPhysicsSystem;
    const JPH::BodyLockInterface &mLockInterface = mPhysicsSystem.GetBodyLockInterface();

    Entity *mLeverHandle = nullptr;
    std::vector<Trapdoor *> mTrapdoors;

    std::unique_ptr<RigidBody> mSensor;

    // Lever handle float properties
    float mMinAngle = 0.0f;
    float mMaxAngle = 0.0f;
    float mAnimationTime = 0.0f;

    // Lever base float properties
    float mProximityPromptRadius = 0.0f;

    JPH::Vec3 mAxisX;
    JPH::Vec3 mAxisY;
    JPH::Vec3 mAxisZ;

    JPH::Quat mInitialRotation;
    JPH::Quat mFinalRotation;

    float mCurrentAnimationTime = 0.0f;

    bool mIsPulled = false;

    float fpsMinSpec = 30.0f;
    float mNetworkRepeatTime = 0.25f * 1.0f / fpsMinSpec;
    float mCurrentNetworkRepeatTime = 0.25f * 1.0f / fpsMinSpec;
    ENetworkLocality mPulledNetworkLocality = ENetworkLocality::None;
};
#endif // GROUP3ENGINE_LEVER_HPP

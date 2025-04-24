#ifndef GROUP3ENGINE_IDOL_HPP
#define GROUP3ENGINE_IDOL_HPP

#include "Entity.hpp"

class Idol : public Entity {
  public:
    Idol();

    void Awake() override;

    void OnInteract(Entity *other) override;

    // void Update(double deltaTime) override;

    std::unique_ptr<RigidBody> mSensor;

    // Float properties
    float mProximityPromptRadius = 0.0f;

    bool mIsCollected = false;
};
#endif // GROUP3ENGINE_IDOL_HPP

//
// Created by thomas on 26/04/25.
//

#ifndef GROUP3ENGINE_ZIPLINE_HPP
#define GROUP3ENGINE_ZIPLINE_HPP
#include "Entity.hpp"
#include "CharacterEntity.hpp"

class ZipLine : public Entity{
public:
    ZipLine() = default;
    ~ZipLine() = default;
    void Awake() override;
    void LateUpdate(double deltaTime) override;

    void OnInteract(Entity *other) override;
    EInteractable IsInteractable() const override { return EInteractable::Interactable; }

private:
    glm::vec3 mStartPosition{};
    glm::vec3 mEndPosition{};
    glm::vec3 mDirection{};
    float mDistance{};
    float mMaxZipSpeed{};
    float mAcceleration{};
    CharacterEntity* mCharacter = nullptr;
    bool mIsZipping = false;
    double mCurrentSpeed = 0.0f;
    glm::vec3 mCurrentPosition{};

    std::unique_ptr<RigidBody> mSensor;

    // Float properties
    float mProximityPromptRadius = 0.0f;

};


#endif //GROUP3ENGINE_ZIPLINE_HPP

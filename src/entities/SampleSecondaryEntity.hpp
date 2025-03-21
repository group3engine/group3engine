//
// Created by thomas on 20/03/25.
//

#ifndef SAMPLENPCENTITY_HPP
#define SAMPLENPCENTITY_HPP



#include "Entity.hpp"
#include "SampleJoltCharacter.h"



class SampleSecondaryEntity : public Entity {
  public:
    SampleSecondaryEntity();
    ~SampleSecondaryEntity() override;

    void ProcessInput();
    void PrePhysicsUpdate();

    Vec3 GetCharacterPosition() {
        return mSampleJoltCharacter->GetCharacterPosition();
    }

    // update override
    void Update(double deltaTime) override;


    void CreateJoltCharacter();

    void Awake() override ;

    void OnCollisionStart(Entity *aOther) override ;

    void OnCollisionStay(Entity *aOther) override {
//        SPDLOG_INFO("I am {} and I am colliding with {}", mName, aOther->mName);
    }


    [[nodiscard]] glm::vec3 GetCharacterPositionOffset() const { return mCharacterPositionOffset; }
    void SetCharacterPositionOffset(glm::vec3 aPosition) { mCharacterPositionOffset = aPosition; }
    void SetCharacterPositionOffset(float x, float y, float z) {
        mCharacterPositionOffset = glm::vec3(x, y, z);
    }

    void MoveToSpawn();

  private:
    Transform mInitialTransform = {};
    std::unique_ptr<SampleJoltCharacter> mSampleJoltCharacter;

};

#endif //SAMPLENPCENTITY_HPP

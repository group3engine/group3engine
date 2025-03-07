//
// Created by thomas on 07/03/25.
//

#ifndef GROUP3ENGINE_CHARACTERENTITY_HPP
#define GROUP3ENGINE_CHARACTERENTITY_HPP
#include "Entity.hpp"
#include "CharacterVirtualTest.h"


class CharacterEntity : public Entity {
  public:
    CharacterEntity();
    ~CharacterEntity() override;

    void SetCharacterVirtual(unique_ptr<CharacterVirtualTest> &&uniquePtr);
    void ProcessInput(const ProcessInputParams &inParams) {
        mCharacterVirtual->ProcessInput(inParams);
    }
    void PrePhysicsUpdate(const PreUpdateParams &inParams) {
        mCharacterVirtual->PrePhysicsUpdate(inParams);
    }

    Vec3 GetCharacterPosition() {
        return mCharacterVirtual->GetCharacterPosition();
    }

    // update override
    void Update(double deltaTime) override;

    void OnCollisionStart(Entity *aOther) override ;

    void OnCollisionStay(Entity *aOther) override {
//        SPDLOG_INFO("I am {} and I am colliding with {}", mName, aOther->mName);
    }

    // set the checkpoint
    void SetCheckpoint(glm::vec3 checkpoint) { mLastCheckpoint = checkpoint; }
    // reset the character to the last checkpoint
    void Reset() {
        mCharacterVirtual->SetCharacterPosition(RVec3(mLastCheckpoint.x,
                                                mLastCheckpoint.y,
                                                mLastCheckpoint.z));
    }

  private:
    bool mHasFirstFrameHappened = false;
    Transform mInitialTransform = {};
    std::unique_ptr<CharacterVirtualTest> mCharacterVirtual;

    glm::vec3 mLastCheckpoint = {};
};

#endif // GROUP3ENGINE_CHARACTERENTITY_HPP

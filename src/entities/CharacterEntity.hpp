//
// Created by thomas on 07/03/25.
//

#ifndef GROUP3ENGINE_CHARACTERENTITY_HPP
#define GROUP3ENGINE_CHARACTERENTITY_HPP

#include <stack>

#include "Entity.hpp"
#include "CharacterVirtualTest.h"

#include "ImGuiRenderer.hpp"

enum class InternalEvent {
    eDeath,
    eCount
};

enum class InternalUiEvent {
    eDeathPopup,
    eFinishPopup,
    eCount
};

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

    void UpdateUi(double deltaTime) override;

    void Awake() override ;

    void OnCollisionStart(Entity *aOther) override ;

    void OnCollisionStay(Entity *aOther) override {
//        SPDLOG_INFO("I am {} and I am colliding with {}", mName, aOther->mName);
    }

    // set the checkpoint
    void SetCheckpoint(glm::vec3 checkpoint) { mLastCheckpoint = checkpoint; Save();}


    // reset the character to the last checkpoint
    void Reset() {
        mCharacterVirtual->SetCharacterPosition(RVec3(mLastCheckpoint.x,
                                                mLastCheckpoint.y,
                                                mLastCheckpoint.z));
    }

    [[nodiscard]] glm::vec3 GetCharacterPositionOffset() const { return mCharacterPositionOffset; }
    void SetCharacterPositionOffset(glm::vec3 aPosition) { mCharacterPositionOffset = aPosition; }
    void SetCharacterPositionOffset(float x, float y, float z) {
        mCharacterPositionOffset = glm::vec3(x, y, z);
    }

    void SetScene(Scene* input_scene){mScene = input_scene;}
    void MoveToSpawn();

  private:
    void Save();
    void Load();

  private:
    Transform mInitialTransform = {};
    std::unique_ptr<CharacterVirtualTest> mCharacterVirtual;

    glm::vec3 mLastCheckpoint = glm::vec3(0, 10.0f, 0);
    float ragdollTime = -10000.0f;
    float totalRagdollTime = 2.0f;

    size_t mDeathCount = 0;
    float mDeathVisibleTimer = 0.0f;
    float mFinishVisibleTimer = 0.0f;

    gui::DeathCounterData mGuiDeathCounterData{};
    gui::DeathPopupData mGuiDeathPopupData{};
    gui::FinishPopupData mGuiFinishPopupData{};

    std::stack<InternalEvent> mInternalEvents;
    std::stack<InternalUiEvent> mInternalUiEvents;
    bool m_has_save = false;
    Scene* mScene;
};

#endif // GROUP3ENGINE_CHARACTERENTITY_HPP

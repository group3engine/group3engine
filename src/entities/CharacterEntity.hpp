//
// Created by thomas on 07/03/25.
//

#ifndef GROUP3ENGINE_CHARACTERENTITY_HPP
#define GROUP3ENGINE_CHARACTERENTITY_HPP

#include <stack>

#include "Entity.hpp"
#include "SampleJoltCharacter.h"

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

    void PrePhysicsUpdate();

    virtual void ProcessInput();

    virtual Vec3 GetCharacterPosition() const {
        return mSampleJoltCharacter->GetCharacterPosition();
    }

    virtual void CreateJoltCharacter();

    void PreUpdate(double deltaTime) override;

    // update override
    void Update(double deltaTime) override;

    void UpdateUi(double deltaTime) override;

    void Awake() override;
    void UnscaledUpdate(double deltaTime) override;

    void OnCollisionStart(Entity *aOther) override;

    void OnCollisionStay(Entity *aOther) override {
//        SPDLOG_INFO("I am {} and I am colliding with {}", mName, aOther->mName);
    }

    // set the checkpoint
    void SetCheckpoint(glm::vec3 checkpoint) { mLastCheckpoint = checkpoint; Save();}


    // reset the character to the last checkpoint
    void Reset() {
        mSampleJoltCharacter->SetCharacterPosition(RVec3(mLastCheckpoint.x,
                                                mLastCheckpoint.y,
                                                mLastCheckpoint.z));
    }

    [[nodiscard]] glm::vec3 GetCharacterPositionOffset() const { return mCharacterPositionOffset; }
    void SetCharacterPositionOffset(glm::vec3 aPosition) { mCharacterPositionOffset = aPosition; }
    void SetCharacterPositionOffset(float x, float y, float z) {
        mCharacterPositionOffset = glm::vec3(x, y, z);
    }

    void AddImpulse(glm::vec3 glm_impulse) {
        Vec3 impulse(glm_impulse.x, glm_impulse.y, glm_impulse.z);
        mSampleJoltCharacter->AddImpulse(impulse);
    }

    void MoveToSpawn();

    void TeleportCallback(glm::vec3 aPosition) {
        SetCheckpoint(aPosition);
        Reset();
    }

    [[nodiscard]] Camera* GetCamera() const{return mCamera;}

    size_t GetPlayerId() const { return mPlayerId; }

  private:
    void Save();
    void Load();

  protected:
    Camera *mCamera = nullptr;
    std::unique_ptr<SampleJoltCharacter> mSampleJoltCharacter;


    size_t mPlayerId = 0;

    gui::TimerData mGuiTimerData{};

    size_t mDeathCount = 0;
    float mDeathVisibleTimer = 0.0f;
    float mFinishVisibleTimer = 0.0f;

    gui::DeathCounterData mGuiDeathCounterData{};
    gui::DeathPopupData mGuiDeathPopupData{};
    gui::FinishPopupData mGuiFinishPopupData{};

    Transform mInitialTransform = {};

    glm::vec3 mLastCheckpoint = glm::vec3(0, 10.0f, 0);

    std::stack<InternalEvent> mInternalEvents;
    std::stack<InternalUiEvent> mInternalUiEvents;

  private:
    bool m_has_save = false;
};


#endif // GROUP3ENGINE_CHARACTERENTITY_HPP

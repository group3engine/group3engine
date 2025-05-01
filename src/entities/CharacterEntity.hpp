//
// Created by thomas on 07/03/25.
//

#ifndef GROUP3ENGINE_CHARACTERENTITY_HPP
#define GROUP3ENGINE_CHARACTERENTITY_HPP

#include <stack>

#include "Entity.hpp"
#include "SampleJoltCharacter.h"

#include "ImGuiRenderer.hpp"
#include "InputMapping.hpp"

struct WinSignal;

enum class InternalEvent {
    eDeath,
    eCount
};

enum class DeathState{
    eLiving,
    eDying,
    eDead,
};

enum class InternalUiEvent {
    eDeathPopup,
    eFinishPopup,
    eWinPopup,
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

    void LateUpdate(double deltaTime) override;

    void UpdateUi(double deltaTime) override;

    void Awake() override;
    void UnscaledUpdate(double deltaTime) override;

    void OnCollisionStart(Entity *aOther) override;

    void OnCollisionStay(Entity *aOther) override;

    void OnCollisionEnd(Entity *aOther) override;

    void OnWin(WinSignal *signal);

    // set the checkpoint
    void SetCheckpoint(glm::vec3 checkpoint) { mLastCheckpoint = checkpoint; Save();}

    void Die();
    // reset the character to the last checkpoint
    void Reset() {
        mSampleJoltCharacter->SetCharacterPosition(RVec3(mLastCheckpoint.x,
                                                mLastCheckpoint.y,
                                                mLastCheckpoint.z));
        mDeathState = DeathState::eLiving;
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

    void SetHanging(bool isHanging) { mHangingAbout = isHanging; }

    void SetPosition(glm::vec3 position) {
        mSampleJoltCharacter->SetCharacterPosition(RVec3(position.x, position.y, position.z));
        // set the velocity to zero
        mSampleJoltCharacter->SetCharacterVelocity(Vec3(0.f, 0.f, 0.f));
    }

  private:
    void Save();
    void Load();

    glm::vec3 CalcClimbDirection(Entity *climbEntity);

    void RegisterControls();

    bool WouldJumpHitCeiling(ECrouchState crouchState) const;

    bool WouldUncrouchHitCeiling() const;

  public:
    inline static float sJumpTimeScale = 0.1f;
    inline static float sFallTimeScale = 0.3f;

    inline static float sFallBlend = 1.0f;
    inline static float sHangingBlend = 0.25f;
    inline static float sClimbTimeScale = 0.45f;

  protected:
    Camera *mCamera = nullptr;
    std::unique_ptr<SampleJoltCharacter> mSampleJoltCharacter;


    size_t mPlayerId = 0;

    bool mIsTiming = false;
    gui::TimerData mGuiTimerData{};

    size_t mDeathCount = 0;
    float mDeathVisibleTimer = 0.0f;
    float mFinishVisibleTimer = 0.0f;

    DeathState mDeathState = DeathState::eLiving;
    double mDeathTimer = 0.0;
    const double mDeathTime = 1.0;

    bool mHasWon = false;

    float mWinVisibleTimer = 0.0f;

    gui::DeathCounterData mGuiDeathCounterData{};
    gui::DeathPopupData mGuiDeathPopupData{};
    gui::FinishPopupData mGuiFinishPopupData{};

    Transform mInitialTransform = {};

    glm::vec3 mLastCheckpoint = glm::vec3(0, 10.0f, 0);

    std::stack<InternalEvent> mInternalEvents;
    std::stack<InternalUiEvent> mInternalUiEvents;
    bool mInClimb = false;
    bool mIsCrouching = false;
    bool mIsEmoting = false;
    bool mHangingAbout = false;

    InputMapping mInputMapping{};

  private:
    bool m_has_save = false;

    bool mLeftClimb = false;
    bool mEnterClimb = false;

    glm::vec3 mClimbDirection = glm::vec3(0.f, 0.f, 0.f);

    std::vector<Entity *> mInteractables;
};


#endif // GROUP3ENGINE_CHARACTERENTITY_HPP

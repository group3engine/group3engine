//
// Created by thomas on 07/03/25.
//

#include "CharacterEntity.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#include <Jolt/Math/Vec3.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>

#include "AudioManager.hpp"
#include "Camera.hpp"
#include "Engine.hpp"
#include "imgui.h"
#include "GLFW.hpp"
#include "RenderPassCommon.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"
#include "InputMapping.hpp"
#include "Input.hpp"
#include "Saving.hpp"

#include "Signals.hpp"

namespace {
    std::filesystem::path BuildSaveFilename() {
        std::filesystem::path saveFilename = "save_";
        saveFilename += Scene::get().GetActiveScene()->GetSceneFilename();
        saveFilename += ".txt";
        return saveFilename;
    }
}

CharacterEntity::~CharacterEntity() {
}

bool CharacterEntity::WouldUncrouchHitCeiling() const {
    // Get feet position by offseting COM with half cylinder height
    // Offset to head by adding cylinder height + capsule height + some offset
    Ref<CharacterVirtual> characterVirtual = mSampleJoltCharacter->GetCharacter();
    RVec3 characterCOM = mSampleJoltCharacter->GetCharacterCenterOfMassPosition();

    float crouchingHalfCapsuleHeight =
        mSampleJoltCharacter->GetCapsuleHalfHeight(ECrouchState::Crouching);

    RVec3 capsuleBottomPosition = characterCOM - Vec3(0, crouchingHalfCapsuleHeight, 0);

    float standingHalfCapsuleHeight =
        mSampleJoltCharacter->GetCapsuleHalfHeight(ECrouchState::Standing);
    RVec3 standingCapsuleTopPosition =
        capsuleBottomPosition + 2.0f * RVec3(0, standingHalfCapsuleHeight, 0);

    float characterPadding = characterVirtual->GetCharacterPadding();

    // Add some padding for some breathing room
    RVec3 ceilingPosition = standingCapsuleTopPosition + 2.0f * RVec3(0, characterPadding, 0);

#ifdef JPH_DEBUG_RENDERER
    GetScene()->GetDebugRenderer()->DrawSphere(ceilingPosition, 0.1f, Color::sPurple);
#endif // JPH_DEBUG_RENDERER

    RRayCast ray;
    ray.mOrigin = characterCOM;
    ray.mDirection = ceilingPosition - characterCOM;

    JPH::RayCastResult result;
    JPH::SpecifiedObjectLayerFilter objectLayerFilter{Layers::NON_MOVING};
    bool hit = PhysicsManager::get().mPhysicsSystem.GetNarrowPhaseQuery().CastRay(
        ray, result, {}, objectLayerFilter, {});

    bool hitCeiling = hit && result.mFraction >= 0 && result.mFraction < 1.0f;

    return hitCeiling;
}

bool CharacterEntity::WouldJumpHitCeiling(ECrouchState crouchState) const {
    // Get feet position by offseting COM with half cylinder height
    // Offset to head by adding cylinder height + capsule height + some offset
    Ref<CharacterVirtual> characterVirtual = mSampleJoltCharacter->GetCharacter();
    RVec3 characterCOM = mSampleJoltCharacter->GetCharacterCenterOfMassPosition();

    RVec3 jumpPeakPosition;
    if (crouchState == ECrouchState::Crouching) {
        float crouchingHalfCapsuleHeight =
            mSampleJoltCharacter->GetCapsuleHalfHeight(ECrouchState::Crouching);

        RVec3 capsuleTopPosition = characterCOM + Vec3(0, crouchingHalfCapsuleHeight, 0);

        float characterPadding = characterVirtual->GetCharacterPadding();

        // Add some padding for some breathing room
        RVec3 jumpHeight = RVec3(0, mSampleJoltCharacter->GetJumpHeight(), 0);
        jumpPeakPosition = capsuleTopPosition + jumpHeight + 2.0f * RVec3(0, characterPadding, 0);
    } else {
        float standingHalfCapsuleHeight =
            mSampleJoltCharacter->GetCapsuleHalfHeight(ECrouchState::Standing);

        RVec3 capsuleTopPosition = characterCOM + Vec3(0, standingHalfCapsuleHeight, 0);

        float characterPadding = characterVirtual->GetCharacterPadding();

        // Add some padding for some breathing room
        RVec3 jumpHeight = RVec3(0, mSampleJoltCharacter->GetJumpHeight(), 0);
        jumpPeakPosition = capsuleTopPosition + jumpHeight + 2.0f * RVec3(0, characterPadding, 0);
    }

#ifdef JPH_DEBUG_RENDERER
    GetScene()->GetDebugRenderer()->DrawSphere(jumpPeakPosition, 0.1f, Color::sOrange);
#endif // JPH_DEBUG_RENDERER

    RRayCast ray;
    ray.mOrigin = characterCOM;
    ray.mDirection = jumpPeakPosition - characterCOM;

    JPH::RayCastResult result;
    JPH::SpecifiedObjectLayerFilter objectLayerFilter{Layers::NON_MOVING};
    bool hit = PhysicsManager::get().mPhysicsSystem.GetNarrowPhaseQuery().CastRay(
        ray, result, {}, objectLayerFilter, {});

    bool hitCeiling = hit && result.mFraction >= 0 && result.mFraction < 1.0f;

    return hitCeiling;
}

void CharacterEntity::ProcessInput(){
#ifdef JPH_DEBUG_RENDERER
    // Extra unneeded call to debug render the test position for the raycasts in these functions
    if (mIsCrouching) {
        WouldUncrouchHitCeiling();
    }
    WouldJumpHitCeiling(mIsCrouching ? ECrouchState::Crouching : ECrouchState::Standing);
#endif // JPH_DEBUG_RENDERER

    if (IsKeyDown(KEY::eLEFT_SHIFT) && IsMouseButtonPressed(MOUSE_BUTTON::eLEFT)) {
        auto &flag = mCamera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = !flag;

        if (flag) {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }
    if (IsMouseButtonPressed(MOUSE_BUTTON::eRIGHT)) {
        auto &flag = mCamera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = false;
    }
#ifdef PLATINUM
    // don't process input if we aren't mousing
    if (!mCamera->inputMap[std::size_t(EInputState::MOUSING)]) {
        return;
    }
#endif
    mCamera->SetInput(EInputState::FORWARD, IsKeyDown(KEY::eW));
    mCamera->SetInput(EInputState::BACKWARD, IsKeyDown(KEY::eS));
    mCamera->SetInput(EInputState::LEFT, IsKeyDown(KEY::eA));
    mCamera->SetInput(EInputState::RIGHT, IsKeyDown(KEY::eD));

    mCamera->SetInput(EInputState::DOWN, IsKeyDown(KEY::eQ));
    mCamera->SetInput(EInputState::UP, IsKeyDown(KEY::eE));

    mCamera->SetInput(EInputState::FAST, IsKeyDown(KEY::eLEFT_SHIFT));
    mCamera->SetInput(EInputState::SLOW, IsKeyDown(KEY::eLEFT_CONTROL));

    mCamera->SetInput(EInputState::SWITCHVIEW, IsKeyPressed(KEY::eV));

    mCamera->SetInput(EInputState::TELEPORT, IsKeyPressed(KEY::eT));

    mCamera->SetInput(EInputState::ZOOM_IN, IsKeyPressed(KEY::eY));
    mCamera->SetInput(EInputState::ZOOM_OUT, IsKeyPressed(KEY::eU));



    glm::vec3 controlInput = glm::vec3(0.0f);
    bool jump = false;
    if(mDeathState == DeathState::eLiving) {
        if (mCamera->isInFollowCharacterMode()) {
            // Determine controller input
            if (mInputMapping.GetActionDown("LEFT") > 0)
                controlInput.z = -1;
            if (mInputMapping.GetActionDown("RIGHT") > 0)
                controlInput.z = 1;
            if (mInputMapping.GetActionDown("FORWARD") > 0)
                controlInput.x = 1;
            if (mInputMapping.GetActionDown("BACKWARD") > 0)
                controlInput.x = -1;
            // make sure the magnitude of controlInput.xz is either 0 or 1
            NormaliseDPad(controlInput.x, controlInput.z);
            if (controlInput != glm::vec3(0.f))
                controlInput = glm::normalize(controlInput);
            float gamepadAxisForwards = mInputMapping.GetActionDown("FORWARD_BACKWARD");
            if (abs(gamepadAxisForwards) > abs(controlInput.x))
                controlInput.x = -gamepadAxisForwards;
            float gamepadAxisLeftRight = mInputMapping.GetActionDown("LEFT_RIGHT");
            if (abs(gamepadAxisLeftRight) > abs(controlInput.z))
                controlInput.z = gamepadAxisLeftRight;

            // Rotate controls to align with the camera
            auto cameraForward = mCamera->GetDirection();
            // if we are in climb, we want to align with the player instead
            if(mInClimb)
            {
                cameraForward = GetLocalTransform().rotation * glm::vec3(0.f, 0.f, -1.f);
            }
            cameraForward.y = 0.0f;
            cameraForward = glm::normalize(cameraForward);
            glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), cameraForward);

            if(mInClimb)
            {
                // if we are grounded, then only do this if x is forward
                if(!mSampleJoltCharacter->IsGrounded() || controlInput.x > 0.1f) {
                    controlInput.y = controlInput.x;
                    controlInput.x = -1.f;
                    controlInput.z = 0.f;
                }
            }
            controlInput = rotation * controlInput;

            // Check actions
            if(mInClimb)
            {
                if (mInputMapping.GetActionPressed("JUMP") > 0) {
                    jump = true;
                }
            }
            else if (mInputMapping.GetActionDown("JUMP") > 0) {
                jump = !WouldJumpHitCeiling(mIsCrouching ? ECrouchState::Crouching
                                                         : ECrouchState::Standing);
            }

            if ((mInputMapping.GetActionPressed("EMOTE") > 0) && !mInClimb) {
                mIsEmoting = true;
            }
            if ((mInputMapping.GetActionPressed("CROUCH") > 0) && !mInClimb) {
                // If we are crouching but we are somewhere we cannot uncrouch (a tunnel)
                // then do not uncrouch
                if (mIsCrouching) {
                    if (!WouldUncrouchHitCeiling()) {
                        mIsCrouching = !mIsCrouching;
                    }
                } else {
                    mIsCrouching = !mIsCrouching;
                }
            }

            if (mInputMapping.GetActionPressed("INTERACT") > 0) {
                // NOTE: Having more than one interactable in an area will allow the user to
                // interact with them all at once
                for (auto &interactable : mInteractables) {
                    interactable->OnInteract(this, ENetworkLocality::Local);
                }
            }
        }
    }
    if(mIsCrouching)
    {
        controlInput *= 0.25f;
    }
    mSampleJoltCharacter->ProcessInput(controlInput, jump, mInClimb, mIsCrouching);
}

void CharacterEntity::PrePhysicsUpdate() {
    PreUpdateParams preUpdateParams{};
    preUpdateParams.mDeltaTime = GlobalUtil::deltaTime;
    mSampleJoltCharacter->PrePhysicsUpdate(preUpdateParams);
}

void CharacterEntity::PreUpdate(double deltaTime) {
    // process the input
    ProcessInput();
}

void CharacterEntity::Update(double deltaTime) {
    // update the death timer
    if(mDeathState == DeathState::eDying) {
        mDeathTimer -= deltaTime;
        if(mDeathTimer <= 0.0) {
            mDeathState = DeathState::eDead;
            mInternalEvents.push(InternalEvent::eDeath);
            Reset();
        }
    }
    // pre physics update
    PrePhysicsUpdate();
    if(mLeftClimb)
    {
        mInClimb = false;
        mLeftClimb = false;
    }
    if(mEnterClimb)
    {
        mInClimb = true;
        mEnterClimb = false;
    }
    if(mInClimb)
    {
        mLastClimbTime = GetTotalTime();
    }
    bool recentlyClimbing = GetTotalTime() - mLastClimbTime < 0.1;
    // update the character position offset
    auto characterPhysicsPos = mSampleJoltCharacter->GetCharacterPosition();
    SetCharacterPositionOffset(characterPhysicsPos.GetX(), characterPhysicsPos.GetY(), characterPhysicsPos.GetZ());


    if(mInputMapping.GetActionPressed("PAUSE") > 0)
    {
        // Engine::get().Quit();
        Engine::get().SetTimeScale(0.f);
        // free the mouse
        auto &flag = mCamera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = false;
        glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    }
#ifndef PLATINUM
    if(IsKeyPressed(KEY::eESCAPE))
    {
    // quit the game
    Engine::get().Quit();
    }
#endif




    while (!mInternalEvents.empty()) {
        auto &event = mInternalEvents.top();
        mInternalEvents.pop();

        switch (event) {
        case InternalEvent::eDeath:
            ++mDeathCount;
            mInternalUiEvents.push(InternalUiEvent::eDeathPopup);
            break;
        default:
            SPDLOG_ERROR("Unaccounted for switch case.");
            exit(EXIT_FAILURE);
            break;
        }
    }



    // get the character state
    // calculate the delta velocity
    Vec3 characterVelocityJolt = mSampleJoltCharacter->GetCharacterVelocity();
    glm::vec3 characterVelocity = glm::vec3(characterVelocityJolt.GetX(), characterVelocityJolt.GetY(), characterVelocityJolt.GetZ());
    // set the character to face the direction of the velocity without the y component
    float characterYSpeed = characterVelocity.y;
    characterVelocity.y = 0;

    Vec3 intendedVelocity = mSampleJoltCharacter->GetIntendedVelocity();
    glm::vec3 intendedVelocityGlm = {intendedVelocity.GetX(), 0, intendedVelocity.GetZ()};

    // if we are in climb, we want to fce
    if(recentlyClimbing)
    {
        characterVelocity = mClimbDirection;
        intendedVelocityGlm = mClimbDirection;
    }

    if (glm::length(intendedVelocityGlm) > 0.1f) {
        // set the transform rotation to the direction of the velocity, on top of the initial transform rotation
        Transform newTransform = GetLocalTransform();
        newTransform.rotation = glm::quatLookAt(glm::normalize(intendedVelocityGlm * -1.f), glm::vec3(0, 1, 0)) * mInitialTransform.rotation;
        SetTransform(newTransform);
    }

    bool isLooping = true;
    bool resetAnimation = false;
    std::string resetAnimationName{};

    // work out the active animation, and the time scale
    float timeScale = 1.0f;
    std::string activeAnimation = "idle";
    float blend = 0.1f;
    bool playWholeAnimation = false;
    if(glm::length(characterVelocity) > 0.4f) {
        activeAnimation = "running";
        timeScale = min(glm::length(characterVelocity) / 10.5f, 2.f);
    }
    // spdlog the current jump state
    switch (mSampleJoltCharacter->GetJumpState()) {
    case EJumpState::Start:
        activeAnimation = "jump up";
        timeScale = sJumpTimeScale;
        playWholeAnimation = false;
        isLooping = false;
        resetAnimation = true;
        resetAnimationName = "jump up";
        break;
    case EJumpState::Falling:
        SPDLOG_INFO("EJumpState::Falling");
        activeAnimation = "falling";
        timeScale = sFallTimeScale;
        blend = sFallBlend;
        playWholeAnimation = false;
        break;
    case EJumpState::End:
        SPDLOG_INFO("EJumpState::End");
        break;
    case EJumpState::None:
        break;
    }
    // if the character is dying, set the animation to dying
    if(mDeathState == DeathState::eDying) {
        activeAnimation = "death";
        timeScale = 1.0f;
        blend = 0.5f;
        playWholeAnimation = false;
        isLooping = false;
    }
    // if the character is climbing, set the animation to climb
    if(recentlyClimbing)
    {
        activeAnimation = "climb";
        timeScale = characterYSpeed * sClimbTimeScale;
        blend = 0.1f;
        playWholeAnimation = false;
        // we can't crouch if we are climbing
        mIsCrouching = false;
    }

    // if we aren't idling, then we can't be crouching or emoting
    if(activeAnimation != "idle")
    {
        mIsEmoting = false;
    }
    // if we are emoting, set the animation to emote
    if(mIsEmoting)
    {
        activeAnimation = "dance";
        timeScale = 1.0f;
        blend = 0.5f;
        playWholeAnimation = false;
    }
    // if we are crouching, set the animation to crouch
    if(mIsCrouching) {
        if(activeAnimation == "running" || activeAnimation == "idle")
            activeAnimation = activeAnimation + "_crouch";
        if(activeAnimation == "running_crouch")
        {
            timeScale = sRunningCrouchTimeScale;
        }
    }
    // if we are hanging, set the animation to hanging
    if(mHangingAbout)
    {
        activeAnimation = "hanging";
        timeScale = 1.f;
        blend = sHangingBlend;
        playWholeAnimation = false;
        isLooping = true;
    }

    // for each child, if there is an animator, call set animation
    for (auto *child : GetChildren()) {
            if (child->HasAnimator()) {
                child->GetAnimator().SetActiveAnimation(activeAnimation, blend, playWholeAnimation, isLooping);
                child->GetAnimator().SetTimeScale(timeScale);
                if(resetAnimation)
                {   
                    child->GetAnimator().ResetActiveAnimation(resetAnimationName);
                }
            }
    }

    mCamera->UpdateCameraRotation(deltaTime);
    ECrouchState crouchState = mIsCrouching ? ECrouchState::Crouching : ECrouchState::Standing;
    mCamera->UpdateCameraMovement(mSampleJoltCharacter->GetCharacterCenterOfMassPosition(), crouchState);
    // if the camera is too close to us, set invisible
    SPDLOG_INFO("cam dist {}", glm::distance(mCamera->GetPosition(), GetCharacterPositionOffset()));
    if(glm::distance(mCamera->GetPosition(), GetCharacterPositionOffset()) < 1.5f)
    {
        for (auto *child : GetChildren()) {
            child->SetAsInvisible();
        }
    }
    else {
        for (auto *child : GetChildren()) {
            child->SetAsVisible();
        }
    }
}

void CharacterEntity::UnscaledUpdate(double deltaTime)
{
    if (Engine::get().GetTimeScale() == 0.f)
    {
        if(mInputMapping.GetActionPressed("PAUSE") > 0)
        {
            Engine::get().SetTimeScale(1.f);
        }
    }
}
void CharacterEntity::UpdateUi(double deltaTime) {
    ImGuiRenderer::NewCharacterInfo(GetName(),
                                    GetCamera()->GetPosition().x,
                                    GetCamera()->GetPosition().y,
                                    GetCamera()->GetPosition().z);

    while (!mInternalUiEvents.empty()) {
        auto &event = mInternalUiEvents.top();
        mInternalUiEvents.pop();

        switch (event) {
        case InternalUiEvent::eDeathPopup:
            // Reset death popup timer
            mDeathVisibleTimer = 1.0f;
            break;
        case InternalUiEvent::eFinishPopup:
            // Reset death popup timer
            mFinishVisibleTimer = 1.0f;
            break;
        case InternalUiEvent::eWinPopup:
            mWinVisibleTimer = 1.0f;
            break;
        default:
            SPDLOG_ERROR("Unaccounted for switch case.");
            exit(EXIT_FAILURE);
            break;
        }
    }

    size_t activePlayerCount = GetScene()->GetActivePlayerCount();

    // New timer window
    if (mIsTiming) {
        mGuiTimerData.time += deltaTime;
    }
    ImGuiRenderer::NewTimer(mGuiTimerData, activePlayerCount, mPlayerId);

    // NOTE: If copying the data into a struct gets annoying, we can just use
    // simple parameters to the gui functions. But using structs might help
    // bundle things more nicely in some cases. This is just an example.
    mGuiDeathCounterData.deathCount = mDeathCount;
    ImGuiRenderer::NewDeathCounter(mGuiDeathCounterData, activePlayerCount, mPlayerId);

    mGuiCoinCounterData.coinCount = mCoinCount;
    ImGuiRenderer::NewCoinCounter(mGuiCoinCounterData, activePlayerCount, mPlayerId);

    mDeathVisibleTimer = std::max(0.0f, mDeathVisibleTimer - static_cast<float>(deltaTime));
    mGuiDeathPopupData.visibleTimer = mDeathVisibleTimer;
    ImGuiRenderer::NewDeathPopup(mGuiDeathPopupData, activePlayerCount, mPlayerId);

    mFinishVisibleTimer = std::max(0.0f, mFinishVisibleTimer - static_cast<float>(deltaTime));
    mGuiFinishPopupData.visibleTimer = mFinishVisibleTimer;

    ImGuiRenderer::NewFinishPopup(mGuiFinishPopupData, activePlayerCount, mPlayerId);

    mWinVisibleTimer = std::max(0.0f, mWinVisibleTimer - static_cast<float>(deltaTime));
    if (mWinVisibleTimer) {
        ImGuiRenderer::Text("You Win!", ImVec2(0.5f, 0.75f), Fonts::TextFont, activePlayerCount, mPlayerId);
    }

    if (!mInteractables.empty()) {
        ImGuiRenderer::Text("Press E to Interact", ImVec2(0.5f, 0.5f), Fonts::TextFont, activePlayerCount, mPlayerId);
    }
}

void CharacterEntity::CreateJoltCharacter()
{
    mSampleJoltCharacter = std::make_unique<SampleJoltCharacter>();
    mSampleJoltCharacter->SetPhysicsSystem(&PhysicsManager::get().mPhysicsSystem);
    mSampleJoltCharacter->SetJobSystem(PhysicsManager::get().mJobSystem.get());
    mSampleJoltCharacter->SetTempAllocator(PhysicsManager::get().mTempAllocator.get());
    mSampleJoltCharacter->SetCustomContactListener(&PhysicsManager::get().mContactListener);
    mSampleJoltCharacter->Initialize();
    PhysicsManager::get().RegisterEntity(this, mSampleJoltCharacter->GetCharacter()->GetInnerBodyID());

}

CharacterEntity::CharacterEntity() {
    SetAsCharacter();
    Load();
    mType = "character";
    RegisterControls();
}
void CharacterEntity::OnCollisionStart(Entity *aOther) {

    if(aOther->CompareTag("deathzone")) {
        SPDLOG_INFO("I am {} and I collided with a death zone", GetName());
        mInternalEvents.push(InternalEvent::eDeath);
        Die();
    }

    // if its a checkpoint, set the checkpoint
    if(aOther->CompareTag("checkpoint")) {
        std::string entityName = aOther->GetName();
        assert(entityName.find("Checkpoint") != std::string::npos);

        int checkpointID = std::stoi(entityName.substr(10));

        if (checkpointID > mLastCheckpointID) {
            // set the checkpoint to the position of the checkpoint, plus a bit in the y direction
            glm::vec3 checkpointPosition =
                aOther->GetWorldTransformComponents().translation + glm::vec3(0, 2.5f, 0);
            SetCheckpoint(checkpointPosition);

            mLastCheckpointID = checkpointID;
        }
    }

    if(aOther->CompareTag("finishzone"))
    {
        // do finish zone things
        mInternalUiEvents.push(InternalUiEvent::eFinishPopup);
    }

    if(aOther->CompareTag("climbable"))
    {
        mEnterClimb = true;

        mClimbDirection = CalcClimbDirection(aOther);
    }

    if (aOther->CompareType("spawn_portal")) {
        if (mHasWon) {
            MoveToSpawn();
        }
    }

    // SPDLOG_INFO("I am {} and I collided with {}", GetName(), aOther->GetName());

}
void CharacterEntity::Save() {
    Saving::get().Save("LastCheckpoint", mLastCheckpoint);
    m_has_save = true;
}

void CharacterEntity::Load() {
    if(Saving::get().HasKey("LastCheckpoint"))
    {
        mLastCheckpoint = Saving::get().Get<glm::vec3>("LastCheckpoint");
        m_has_save = true;
        return;
    }
    else
    {
        m_has_save = false;
    }
}

void CharacterEntity::Awake() {
    mPlayerId = GetScene()->PostIncrementPlayerCount();

    mInitialTransform = GetLocalTransform();
    // create the jolt character
    CreateJoltCharacter();

    JPH::Vec3 joltPos = GetCharacterPosition();
    glm::vec3 pos = glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());
    glm::vec3 dir = glm::vec3(1.0f, 1.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0);
    mCamera = new Camera(pos, dir, up);

    mCamera->SetIsActive(true);

    Scene::get().GetActiveScene()->AddCamera(mCamera);

    // register the teleport callback
    mCamera->SetTeleportCallbackFunction(std::bind(&CharacterEntity::TeleportCallback, this, std::placeholders::_1));

    // if there is no save
    if(!m_has_save)
    {
        MoveToSpawn();
    }
    else
    {
        Reset();
    }

    mIsTiming = true;

    // TODO: What you could do is maybe add and remove these receivers dynamically so the number of
    // receivers doesn't grow too large. I.e., player gets to 90% of level, start receiving the
    // on win signal. Then when the player resets to the start of the level remove the receiver
    GetScene()->mSignalSystem.AddReceiver<CharacterEntity, WinSignal>(this, &CharacterEntity::OnWin);
    GetScene()->mSignalSystem.AddReceiver<CharacterEntity, CoinSignal>(this, &CharacterEntity::AddCoin);
}

void CharacterEntity::OnWin(WinSignal *signal) {
    mInternalUiEvents.push(InternalUiEvent::eWinPopup);
    mHasWon = true;
    mIsTiming = false;
}

void CharacterEntity::MoveToSpawn()
{
    for(auto &entity: Scene::get().GetActiveScene()->GetEntities())
    {
        if(entity->CompareTag("spawnpoint"))
        {
            SetCheckpoint(entity->GetWorldTransformComponents().translation + glm::vec3(0.f, 2.5f, 0.f));
            Reset();
        }
    }

    mIsTiming = true;
    mHasWon = false;

    ResetToSpawnSignal resetToSpawnSignal{};
    resetToSpawnSignal.transmitter = this;
    GetScene()->mSignalSystem.EmitSignal(&resetToSpawnSignal);
}

void CharacterEntity::Die()
{
    if(mDeathState == DeathState::eLiving) {
        // set the death state to dying
        mDeathState = DeathState::eDying;
        // set the death timer to death time
        mDeathTimer = mDeathTime;
    }

}

void CharacterEntity::AddCoin(CoinSignal *signal)
{
    mCoinCount++;
}

void CharacterEntity::OnCollisionEnd(Entity *aOther)
{
    if (aOther->CompareTag("climbablezone"))
    {
        SPDLOG_INFO("Left climb");
        mLeftClimb = true;
    }

    if (aOther->CompareTag("proximity_prompt")) {
        std::erase_if(mInteractables, [aOther](auto *entity) { return entity == aOther; });
    }
}

void CharacterEntity::OnCollisionStay(Entity *aOther)
{
    if (aOther->CompareTag("climbable"))
    {
        mEnterClimb = true;

        mClimbDirection = CalcClimbDirection(aOther);
    }

    if (aOther->CompareTag("proximity_prompt") &&
        aOther->IsInteractable() == EInteractable::Interactable) {
        mInteractables.push_back(aOther);
    }
}

glm::vec3 CharacterEntity::CalcClimbDirection(Entity *climbEntity) {
    // Get the z axis of the climbable entity (assume its a flat wall/plane)
    // Make sure the origin/transform of the object in the editor is correct.
    // If it is then the z axis should be parallel to the plane normal
    glm::vec4 worldTransform = climbEntity->GetWorldTransform()[2];
    worldTransform.y = 0;
    worldTransform = glm::normalize(worldTransform);
    // Negate the z axis to make the character's velocity go into the wall and look at it
    worldTransform = -worldTransform;
    return glm::vec3(worldTransform);
}

void CharacterEntity::RegisterControls()
{
    // Register controls for the player
    mInputMapping.AddBinding("FORWARD", KEY::eW);
    mInputMapping.AddBinding("BACKWARD", KEY::eS);
    mInputMapping.AddBinding("FORWARD_BACKWARD", SDL_INPUT::GamepadAxis::GAMEPAD_AXIS_LEFT_Y, 0);
    mInputMapping.AddBinding("LEFT", KEY::eA);
    mInputMapping.AddBinding("RIGHT", KEY::eD);
    mInputMapping.AddBinding("LEFT_RIGHT", SDL_INPUT::GamepadAxis::GAMEPAD_AXIS_LEFT_X, 0);
    mInputMapping.AddBinding("JUMP", KEY::eSPACE);
    mInputMapping.AddBinding("JUMP", SDL_INPUT::GamepadButton::GAMEPAD_BUTTON_A, 0);
    mInputMapping.AddBinding("CROUCH", KEY::eC);
    mInputMapping.AddBinding("CROUCH", KEY::eLEFT_CONTROL);
    mInputMapping.AddBinding("CROUCH", SDL_INPUT::GamepadButton::GAMEPAD_BUTTON_B, 0);
    mInputMapping.AddBinding("CROUCH", SDL_INPUT::GamepadButton::GAMEPAD_BUTTON_RIGHT_THUMB, 0);
    mInputMapping.AddBinding("EMOTE", KEY::eF);
    mInputMapping.AddBinding("EMOTE", SDL_INPUT::GamepadButton::GAMEPAD_BUTTON_LEFT_FACE_DOWN, 0);
    mInputMapping.AddBinding("INTERACT", KEY::eE);
    mInputMapping.AddBinding("INTERACT", SDL_INPUT::GamepadButton::GAMEPAD_BUTTON_X, 0);
    mInputMapping.AddBinding("PAUSE", SDL_INPUT::GamepadButton::GAMEPAD_BUTTON_MIDDLE_RIGHT, 0);
#ifndef PLATINUM
    mInputMapping.AddBinding("PAUSE", KEY::eP);
#else
    mInputMapping.AddBinding("PAUSE", KEY::eESCAPE);
#endif

    // TODO: Interact gamepad binding?


}

void CharacterEntity::LateUpdate(double deltaTime)
{


    // if we are beginning a jump
    if(mSampleJoltCharacter->GetJumpState() == EJumpState::Start)
    {
        // we are not in climb
        mInClimb = false;
        // we should vibrate the controller
        SDL_INPUT::SetGamepadVibration(0, 0.5f, 0.5f, 0.1f);
        // play the jump sound
        glm::vec3 pos = GetWorldTransformComponents().translation;
        AudioManager::get().Play3D("jump", pos.x, pos.y, pos.z);
    }
    // if we have just landed
    if(mSampleJoltCharacter->GetJumpState() == EJumpState::End)
    {
        // we should vibrate the controller
        SDL_INPUT::SetGamepadVibration(0, 0.1f, 0.1f, 0.1f);
        // play the land sound
        glm::vec3 pos = GetWorldTransformComponents().translation;
        AudioManager::get().Play3D("land", pos.x, pos.y, pos.z);
    }

    // Clear the interactables after every update, their conditions need to be checked next frame
    // NOTE: It is too difficult to work out how to add and remove interactables in OnCollisionStart
    // and OnCollision given additional conditions, such as an interactable having been used and no
    // longer being interactable
    mInteractables.clear();

}

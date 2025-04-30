//
// Created by thomas on 03/04/25.
//

#include "NetworkedCharacterRemote.hpp"


#include "Engine.hpp"
#include "GLFW.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"
#include "CharacterEntity.hpp"



void NetworkedCharacterRemote::ProcessInput(){
    mSampleJoltCharacter->ProcessInput(glm::vec3(0), false, mInClimb, mIsCrouching);
}

void NetworkedCharacterRemote::PrePhysicsUpdate() {
    if(!mHasEverBeenGivenState)
        return;
    PreUpdateParams preUpdateParams{};
    preUpdateParams.mDeltaTime = GlobalUtil::deltaTime;
    mSampleJoltCharacter->PrePhysicsUpdate(preUpdateParams);
}

void NetworkedCharacterRemote::LateUpdate(double deltaTime) {
    if(!mHasEverBeenGivenState)
        return;
    // process the input
    ProcessInput();
    // pre physics update
    PrePhysicsUpdate();
    // update the character position offset
    auto characterPhysicsPos = mSampleJoltCharacter->GetCharacterPosition();
    SetCharacterPositionOffset(characterPhysicsPos.GetX(), characterPhysicsPos.GetY(), characterPhysicsPos.GetZ());


     // get the character state
    // calculate the delta velocity
    Vec3 characterVelocityJolt = mSampleJoltCharacter->GetCharacterVelocity();
    glm::vec3 characterVelocity = glm::vec3(characterVelocityJolt.GetX(), characterVelocityJolt.GetY(), characterVelocityJolt.GetZ());
    // set the character to face the direction of the velocity without the y component
    float characterYSpeed = characterVelocity.y;
    characterVelocity.y = 0;

    Vec3 intendedVelocity = mSampleJoltCharacter->GetIntendedVelocity();
    glm::vec3 intendedVelocityGlm = {intendedVelocity.GetX(), 0, intendedVelocity.GetZ()};

    //TODO: network climb direction
    // // if we are in climb, we want to fce
    // if(mInClimb)
    // {
    //     characterVelocity = mClimbDirection;
    //     intendedVelocityGlm = mClimbDirection;
    // }

    if (glm::length(intendedVelocityGlm) > 0.1f) {
        // set the transform rotation to the direction of the velocity, on top of the initial transform rotation
        Transform newTransform = GetLocalTransform();
        newTransform.rotation = glm::quatLookAt(glm::normalize(intendedVelocityGlm * -1.f), glm::vec3(0, 1, 0));
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
    if(mSampleJoltCharacter->GetCharacterVelocity().GetY() > 0.6 && mSampleJoltCharacter->GetJumpState() == EJumpState::None)
    {
        mSampleJoltCharacter->SetJumpState(EJumpState::Start);
    }
    switch (mSampleJoltCharacter->GetJumpState()) {
    case EJumpState::Start:
        activeAnimation = "jump up";
        timeScale = CharacterEntity::sJumpTimeScale;
        playWholeAnimation = false;
        isLooping = false;
        resetAnimation = true;
        resetAnimationName = "jump up";
        break;
    case EJumpState::Falling:
        SPDLOG_INFO("EJumpState::Falling");
        activeAnimation = "falling";
        timeScale = CharacterEntity::sFallTimeScale;
        blend = CharacterEntity::sFallBlend;
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
    if(mInClimb)
    {
        activeAnimation = "climb";
        timeScale = characterYSpeed;
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
            timeScale = 10.0f * timeScale;
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
    for (auto &child : GetChildren()) {
            if (child->HasAnimator()) {
                child->GetAnimator().SetActiveAnimation(activeAnimation, blend, playWholeAnimation, isLooping);
                child->GetAnimator().SetTimeScale(timeScale);
                if(resetAnimation)
                {   
                    child->GetAnimator().ResetActiveAnimation(resetAnimationName);
                }
            }
    }

    // update the transform
    Transform newTransform = GetLocalTransform();
    SetTransform(newTransform);



}
void NetworkedCharacterRemote::CreateJoltCharacter()
{
    mSampleJoltCharacter = std::make_unique<SampleJoltCharacter>();
    mSampleJoltCharacter->SetPhysicsSystem(&PhysicsManager::get().mPhysicsSystem);
    mSampleJoltCharacter->SetJobSystem(PhysicsManager::get().mJobSystem.get());
    mSampleJoltCharacter->SetTempAllocator(PhysicsManager::get().mTempAllocator.get());
    mSampleJoltCharacter->SetCustomContactListener(&PhysicsManager::get().mContactListener);
    mSampleJoltCharacter->Initialize();
    mSampleJoltCharacter->SetManualVelocityMode(true);
    PhysicsManager::get().RegisterEntity(this, mSampleJoltCharacter->GetCharacter()->GetInnerBodyID());

}

NetworkedCharacterRemote::NetworkedCharacterRemote() {
    mType = "NetworkedCharacterRemote";
    SetHasOffset();
}


void NetworkedCharacterRemote::Awake() {
    // create the jolt character
    CreateJoltCharacter();
    SetCharacterPositionOffset({0, -10000, 0});
    Transform newTransform = GetLocalTransform();
    SetTransform(newTransform);
}

void NetworkedCharacterRemote::UpdateState(State state)
{
    mSampleJoltCharacter->SetCharacterPosition(RVec3(state.position.x, state.position.y, state.position.z));
    Transform newTransform = GetLocalTransform();
    newTransform.rotation = glm::normalize(state.rotation);
    SetTransform(newTransform);
    mSampleJoltCharacter->SetCharacterVelocity(RVec3(state.velocity.x, state.velocity.y, state.velocity.z));
    mIsCrouching = state.isCrouching;
    mIsEmoting = state.isEmoting;
    mInClimb = state.isInClimb;
    mDeathState = state.deathState;
    mHangingAbout = state.isHanging;
    mHasEverBeenGivenState = true;
}



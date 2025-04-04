//
// Created by thomas on 03/04/25.
//

#include "NetworkedCharacterRemote.hpp"


#include "Engine.hpp"
#include "GLFW.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"



void NetworkedCharacterRemote::ProcessInput(){
    mSampleJoltCharacter->ProcessInput(glm::vec3(0), false);
}

void NetworkedCharacterRemote::PrePhysicsUpdate() {
    PreUpdateParams preUpdateParams{};
    preUpdateParams.mDeltaTime = GlobalUtil::deltaTime;
    mSampleJoltCharacter->PrePhysicsUpdate(preUpdateParams);
}

void NetworkedCharacterRemote::LateUpdate(double deltaTime) {
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
    characterVelocity.y = 0;
    // work out the active animation, and the time scale
    float timeScale = 1.0f;
    std::string activeAnimation = "idle";
    float blend = 0.1f;
    bool playWholeAnimation = false;
    if(glm::length(characterVelocity) > 0.4f) {
        activeAnimation = "running";
        timeScale = min(glm::length(characterVelocity) / 5.5f, 2.f);
    }
    // spdlog the current jump state
    switch (mSampleJoltCharacter->GetJumpState()) {
        case EJumpState::Start:
            activeAnimation = "jump up";
            playWholeAnimation = false;
            timeScale = 1.0f;
            break;
        case EJumpState::Falling:
            activeAnimation = "falling";
            timeScale = 1.0f;
            blend = 0.5f;
            playWholeAnimation = false;
            break;
        case EJumpState::End:
            break;
        case EJumpState::None:
            break;
    }
    // for each child, if there is an animator, call set animation
    for (auto &child : GetChildren()) {
        if (child->HasAnimator()) {
            child->GetAnimator().SetActiveAnimation(activeAnimation, blend, playWholeAnimation);
            child->GetAnimator().SetTimeScale(timeScale);
        }
    }

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
    mSampleJoltCharacter->SetCharacterPosition({0, -10000, 0});
}

void NetworkedCharacterRemote::UpdateState(State state)
{
    mSampleJoltCharacter->SetCharacterPosition(RVec3(state.position.x, state.position.y, state.position.z));
    Transform newTransform = GetLocalTransform();
    newTransform.rotation = glm::normalize(state.rotation);
    SetTransform(newTransform);
    mSampleJoltCharacter->SetCharacterVelocity(RVec3(state.velocity.x, state.velocity.y, state.velocity.z));
}



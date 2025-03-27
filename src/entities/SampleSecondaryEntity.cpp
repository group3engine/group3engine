//
// Created by thomas on 20/03/25.
//

#include "SampleSecondaryEntity.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#include "Camera.hpp"
#include "GLFW.hpp"
#include "Scene.hpp"



SampleSecondaryEntity::~SampleSecondaryEntity() {
}

void SampleSecondaryEntity::ProcessInput(){
    mCamera->SetInput(EInputState::FORWARD, IsKeyDown(KEY::eUP));
    mCamera->SetInput(EInputState::BACKWARD, IsKeyDown(KEY::eDOWN));
    mCamera->SetInput(EInputState::LEFT, IsKeyDown(KEY::eLEFT));
    mCamera->SetInput(EInputState::RIGHT, IsKeyDown(KEY::eRIGHT));

    mCamera->SetInput(EInputState::DOWN, IsKeyDown(KEY::eQ));
    mCamera->SetInput(EInputState::UP, IsKeyDown(KEY::eE));

    mCamera->SetInput(EInputState::FAST, IsKeyDown(KEY::eLEFT_SHIFT));
    mCamera->SetInput(EInputState::SLOW, IsKeyDown(KEY::eLEFT_CONTROL));

    mCamera->SetInput(EInputState::SWITCHVIEW, IsKeyPressed(KEY::eV));

    mCamera->SetInput(EInputState::TELEPORT, IsKeyPressed(KEY::eT));

    mCamera->SetInput(EInputState::ZOOM_IN, IsKeyPressed(KEY::eY));
    mCamera->SetInput(EInputState::ZOOM_OUT, IsKeyPressed(KEY::eU));

    if (IsMouseButtonPressed(MOUSE_BUTTON::eRIGHT)) {
        auto &flag = mCamera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = !flag;

        if (flag) {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        } else {
            glfwSetInputMode(Platform::get().window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }
    }

    glm::vec3 controlInput = glm::vec3(0.0f);
    bool jump = false;
    if(mCamera->isInFollowCharacterMode()) {
        // Determine controller input
        if (IsKeyDown(KEY::eLEFT))
            controlInput.z = -1;
        if (IsKeyDown(KEY::eRIGHT))
            controlInput.z = 1;
        if (IsKeyDown(KEY::eUP))
            controlInput.x = 1;
        if (IsKeyDown(KEY::eDOWN))
            controlInput.x = -1;
        if (controlInput != glm::vec3(0.f))
            controlInput = glm::normalize(controlInput);

        // Rotate controls to align with the camera
        auto cameraForward = mCamera->GetDirection();
        cameraForward.y = 0.0f;
        cameraForward = glm::normalize(cameraForward);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), cameraForward);
        controlInput = rotation * controlInput;

        // Check actions
        jump = IsKeyPressed(KEY::eRIGHT_SHIFT);
    }
    mSampleJoltCharacter->ProcessInput(controlInput, jump);
}

void SampleSecondaryEntity::PrePhysicsUpdate() {
    PreUpdateParams preUpdateParams{};
    preUpdateParams.mDeltaTime = GlobalUtil::deltaTime;
    mSampleJoltCharacter->PrePhysicsUpdate(preUpdateParams);
}

void SampleSecondaryEntity::Update(double deltaTime) {
    // process the input
    ProcessInput();
    // pre physics update
    PrePhysicsUpdate();
    // update the character position offset
    auto characterPhysicsPos = mSampleJoltCharacter->GetCharacterPosition();
    SetCharacterPositionOffset(characterPhysicsPos.GetX(), characterPhysicsPos.GetY(), characterPhysicsPos.GetZ());




    Entity::Update(deltaTime);


    // get the character state
    // calculate the delta velocity
    Vec3 characterVelocityJolt = mSampleJoltCharacter->GetCharacterVelocity();
    glm::vec3 characterVelocity = glm::vec3(characterVelocityJolt.GetX(), characterVelocityJolt.GetY(), characterVelocityJolt.GetZ());
    // set the character to face the direction of the velocity without the y component
    characterVelocity.y = 0;
    if (glm::length(characterVelocity) > 0.1f) {
        // set the transform rotation to the direction of the velocity, on top of the initial transform rotation
        Transform newTransform = GetLocalTransform();
        newTransform.rotation = glm::quatLookAt(glm::normalize(characterVelocity * -1.f), glm::vec3(0, 1, 0)) * mInitialTransform.rotation;
        SetTransform(newTransform);
    }
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

    mCamera->UpdateCameraRotation(deltaTime);
    mCamera->UpdateCameraMovement(GetWorldTransformComponents());
}

void SampleSecondaryEntity::CreateJoltCharacter()
{
    mSampleJoltCharacter = std::make_unique<SampleJoltCharacter>();
    mSampleJoltCharacter->SetPhysicsSystem(&PhysicsManager::get().mPhysicsSystem);
    mSampleJoltCharacter->SetJobSystem(PhysicsManager::get().mJobSystem.get());
    mSampleJoltCharacter->SetTempAllocator(PhysicsManager::get().mTempAllocator.get());
    mSampleJoltCharacter->SetCustomContactListener(&PhysicsManager::get().mContactListener);
    mSampleJoltCharacter->Initialize();
    PhysicsManager::get().RegisterEntity(this, mSampleJoltCharacter->GetCharacter()->GetInnerBodyID());

}

SampleSecondaryEntity::SampleSecondaryEntity() {
    SetAsCharacter();
}
void SampleSecondaryEntity::OnCollisionStart(Entity *aOther) {

}

void SampleSecondaryEntity::Awake() {
    mInitialTransform = GetLocalTransform();
    // create the jolt character
    CreateJoltCharacter();

    JPH::Vec3 joltPos = GetCharacterPosition();
    glm::vec3 pos = glm::vec3(joltPos.GetX(), joltPos.GetY(), joltPos.GetZ());
    glm::vec3 dir = glm::vec3(1.0f, 1.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0);
    mCamera = new Camera(pos, dir, up);

    Scene::get().GetActiveScene()->AddCamera(mCamera);


    // move to spawn
    MoveToSpawn();
}

void SampleSecondaryEntity::MoveToSpawn()
{
    for(auto &entity: Scene::get().GetActiveScene()->GetEntities())
    {
        if(entity->CompareTag("spawnpoint"))
        {
            mSampleJoltCharacter->SetCharacterPosition(RVec3(entity->GetWorldTransformComponents().translation.x,
                                                    entity->GetWorldTransformComponents().translation.y + 2.5,
                                                    entity->GetWorldTransformComponents().translation.z));
        }
    }
}



//
// Created by thomas on 07/03/25.
//

#include "CharacterEntity.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>

#include "Camera.hpp"
#include "Engine.hpp"
#include "imgui.h"
#include "GLFW.hpp"
#include "RenderPassCommon.hpp"
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"

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

void CharacterEntity::ProcessInput(){
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
            if (IsKeyDown(KEY::eA))
                controlInput.z = -1;
            if (IsKeyDown(KEY::eD))
                controlInput.z = 1;
            if (IsKeyDown(KEY::eW))
                controlInput.x = 1;
            if (IsKeyDown(KEY::eS))
                controlInput.x = -1;
            if (controlInput != glm::vec3(0.f))
                controlInput = glm::normalize(controlInput);
            if (abs(GetGamepadAxis(GAMEPAD_AXIS::eLEFT_Y)) > 0.1f)
                controlInput.z = -GetGamepadAxis(GAMEPAD_AXIS::eLEFT_Y);
            if (abs(GetGamepadAxis(GAMEPAD_AXIS::eLEFT_X)) > 0.1f)
                controlInput.z = GetGamepadAxis(GAMEPAD_AXIS::eLEFT_X);

            // Rotate controls to align with the camera
            auto cameraForward = mCamera->GetDirection();
            cameraForward.y = 0.0f;
            cameraForward = glm::normalize(cameraForward);
            glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), cameraForward);
            controlInput = rotation * controlInput;

            // Check actions
            jump = IsKeyPressed(KEY::eSPACE) || IsGamepadButtonPressed(GAMEPAD_BUTTON::eA);
            if (IsKeyPressed(KEY::eF)) {
                // for each child, if there is an animator, call set animation
                for (auto &child: GetChildren()) {
                    if (child->HasAnimator()) {
                        child->GetAnimator().SetActiveAnimation("dance", 0.1, true, true);
                        child->GetAnimator().SetTimeScale(1.f);
                    }
                }
            }
        }
    }
    mSampleJoltCharacter->ProcessInput(controlInput, jump, mInClimb);
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
    // update the character position offset
    auto characterPhysicsPos = mSampleJoltCharacter->GetCharacterPosition();
    SetCharacterPositionOffset(characterPhysicsPos.GetX(), characterPhysicsPos.GetY(), characterPhysicsPos.GetZ());

#ifdef PLATINUM
    if (IsKeyPressed(KEY::eESCAPE))
#else
    if (IsKeyPressed(KEY::eP))
#endif
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
    // if the character is dying, set the animation to dying
    if(mDeathState == DeathState::eDying) {
        activeAnimation = "death";
        timeScale = 1.0f;
        blend = 0.5f;
        playWholeAnimation = false;
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


void CharacterEntity::UnscaledUpdate(double deltaTime)
{
    if (Engine::get().GetTimeScale() == 0.f)
    {
#ifdef PLATINUM
        if (IsKeyPressed(KEY::eESCAPE))
#else
        if (IsKeyPressed(KEY::eP))
#endif
        {
            Engine::get().SetTimeScale(1.f);
        }
    }
}
void CharacterEntity::UpdateUi(double deltaTime) {
    ImGuiRenderer::NewCharacterInfo(this);

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
        default:
            SPDLOG_ERROR("Unaccounted for switch case.");
            exit(EXIT_FAILURE);
            break;
        }
    }

    size_t activePlayerCount = GetScene()->GetActivePlayerCount();

    // New timer window
    mGuiTimerData.time += deltaTime;
    ImGuiRenderer::NewTimer(mGuiTimerData, activePlayerCount, mPlayerId);

    // NOTE: If copying the data into a struct gets annoying, we can just use
    // simple parameters to the gui functions. But using structs might help
    // bundle things more nicely in some cases. This is just an example.
    mGuiDeathCounterData.deathCount = mDeathCount;
    ImGuiRenderer::NewDeathCounter(mGuiDeathCounterData, activePlayerCount, mPlayerId);

    mDeathVisibleTimer = std::max(0.0f, mDeathVisibleTimer - static_cast<float>(deltaTime));
    mGuiDeathPopupData.visibleTimer = mDeathVisibleTimer;
    ImGuiRenderer::NewDeathPopup(mGuiDeathPopupData, activePlayerCount, mPlayerId);

    mFinishVisibleTimer = std::max(0.0f, mFinishVisibleTimer - static_cast<float>(deltaTime));
    mGuiFinishPopupData.visibleTimer = mFinishVisibleTimer;

    ImGuiRenderer::NewFinishPopup(mGuiFinishPopupData, activePlayerCount, mPlayerId);
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
}
void CharacterEntity::OnCollisionStart(Entity *aOther) {

    if(aOther->CompareTag("deathzone")) {
        SPDLOG_INFO("I am {} and I collided with a death zone", GetName());
        mInternalEvents.push(InternalEvent::eDeath);
        Die();
    }

    // if its a checkpoint, set the checkpoint
    if(aOther->CompareTag("checkpoint")) {
        // set the checkpoint to the position of the checkpoint, plus a bit in the y direction
        glm::vec3 checkpointPosition =
            aOther->GetWorldTransformComponents().translation + glm::vec3(0, 2.5f, 0);
        SetCheckpoint(checkpointPosition);

    }

    if(aOther->CompareTag("finishzone"))
    {
        // do finish zone things
        mInternalUiEvents.push(InternalUiEvent::eFinishPopup);
    }

    if(aOther->CompareTag("climbable"))
    {
        mInClimb = true;
    }

    SPDLOG_INFO("I am {} and I collided with {}", GetName(), aOther->GetName());

}
void CharacterEntity::Save() {
    // Get the user's home directory
    std::filesystem::path homePath;

#ifdef _WIN32
    homePath = std::getenv("USERPROFILE");
#else
    homePath = std::getenv("HOME");
#endif

    // Create path to documents folder/group3engine
    std::filesystem::path savePath = homePath / "Documents" / "group3enginesaves";

    // Create directories if they don't exist
    std::error_code ec;
    if (!std::filesystem::exists(savePath)) {
        std::filesystem::create_directories(savePath, ec);
        if (ec) {
            SPDLOG_ERROR("Failed to create save directory: {}", ec.message());
            return;
        }
    }

    // Create the full file path
    std::filesystem::path saveFile = savePath / BuildSaveFilename();

    // Open file for writing
    std::ofstream file(saveFile);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open save file for writing: {}", saveFile.string());
        return;
    }

    // Write character data
    file << "LastCheckpoint=" << mLastCheckpoint.x << "," << mLastCheckpoint.y << "," << mLastCheckpoint.z << std::endl;

    SPDLOG_INFO("Game saved to {}", saveFile.string());
    m_has_save = true;
}

void CharacterEntity::Load() {
    // Get the user's home directory
    std::filesystem::path homePath;

#ifdef _WIN32
    homePath = std::getenv("USERPROFILE");
#else
    homePath = std::getenv("HOME");
#endif

    // Path to save file
    std::filesystem::path savePath = homePath / "Documents" / "group3enginesaves";
    std::filesystem::path saveFile = savePath / BuildSaveFilename();

    // Check if file exists
    if (!std::filesystem::exists(saveFile)) {
        SPDLOG_INFO("No save file found at {}, using default checkpoint", saveFile.string());
        return;
    }

    // Open file for reading
    std::ifstream file(saveFile);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open save file for reading: {}", saveFile.string());
        return;
    }

    // Read and parse the save data
    std::string line;
    while (std::getline(file, line)) {
        if (line.find("LastCheckpoint=") == 0) {
            std::string values = line.substr(std::string("LastCheckpoint=").length());

            // Parse the comma-separated values
            std::stringstream ss(values);
            std::string xStr, yStr, zStr;

            if (std::getline(ss, xStr, ',') &&
                std::getline(ss, yStr, ',') &&
                std::getline(ss, zStr, ',')) {

                try {
                    float x = std::stof(xStr);
                    float y = std::stof(yStr);
                    float z = std::stof(zStr);

                    mLastCheckpoint = glm::vec3(x, y, z);
                    SPDLOG_INFO("Loaded checkpoint: ({}, {}, {})", x, y, z);
                    m_has_save = true;
                } catch (const std::exception& e) {
                    SPDLOG_ERROR("Failed to parse checkpoint coordinates: {}", e.what());
                }
            }
            break;
        }
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
}

void CharacterEntity::Die()
{
    // set the death state to dying
    mDeathState = DeathState::eDying;
    // set the death timer to death time
    mDeathTimer = mDeathTime;

}

void CharacterEntity::OnCollisionEnd(Entity *aOther)
{
    if (aOther->CompareTag("climbable"))
    {
        mInClimb = false;
    }
}

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
#include "SampleGLTFFilePaths.hpp"
#include "Scene.hpp"

namespace {
    std::filesystem::path BuildSaveFilename() {
        std::filesystem::path saveFilename = "save_";
        saveFilename += Scene::GetActiveScene()->GetSceneFilename();
        saveFilename += ".txt";
        return saveFilename;
    }
}

CharacterEntity::~CharacterEntity() {
}

void CharacterEntity::ProcessInput(){
    glm::vec3 controlInput = glm::vec3(0.0f);
    bool jump = false;
    if(Camera::GetMainCamera()->isInFollowCharacterMode()) {
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
        auto cameraForward = Camera::GetMainCamera()->GetDirection();
        cameraForward.y = 0.0f;
        cameraForward = glm::normalize(cameraForward);
        glm::quat rotation = glm::rotation(glm::vec3(1.0f, 0.0f, 0.0f), cameraForward);
        controlInput = rotation * controlInput;

        // Check actions
        jump = IsKeyPressed(KEY::eSPACE) || IsGamepadButtonPressed(GAMEPAD_BUTTON::eA);
    }
    mSampleJoltCharacter->ProcessInput(controlInput, jump);
}

void CharacterEntity::PrePhysicsUpdate() {
    PreUpdateParams preUpdateParams{};
    preUpdateParams.mDeltaTime = GlobalUtil::deltaTime;
    mSampleJoltCharacter->PrePhysicsUpdate(preUpdateParams);
}

void CharacterEntity::Update(double deltaTime) {
    // process the input
    ProcessInput();
    // pre physics update
    PrePhysicsUpdate();
    // update the character position offset
    auto characterPhysicsPos = mSampleJoltCharacter->GetCharacterPosition();
    SetCharacterPositionOffset(characterPhysicsPos.GetX(), characterPhysicsPos.GetY(), characterPhysicsPos.GetZ());

    if (IsKeyPressed(KEY::eR))
    {
        Engine::get().ChangeScene(Sample::SampleObby);
    }



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

    // set the ragdoll mode to false if the ragdoll time has passed
    if(GetTotalTime() - ragdollTime > totalRagdollTime) {
        mSampleJoltCharacter->SetRagdollMode(false);
    }
    else if (mSampleJoltCharacter->GetRagdollMode())
    {
        // set animation to hit
        for (auto &child : GetChildren()) {
            if (child->HasAnimator()) {
                child->GetAnimator().SetActiveAnimation("hit", 0.1f, false);
            }
        }
        return;
    }


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


}

void CharacterEntity::UpdateUi(double deltaTime) {
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

    // NOTE: If copying the data into a struct gets annoying, we can just use
    // simple parameters to the gui functions. But using structs might help
    // bundle things more nicely in some cases. This is just an example.
    mGuiDeathCounterData.deathCount = mDeathCount;
    ImGuiRenderer::NewDeathCounter(mGuiDeathCounterData);

    mDeathVisibleTimer = std::max(0.0f, mDeathVisibleTimer - static_cast<float>(deltaTime));
    mGuiDeathPopupData.visibleTimer = mDeathVisibleTimer;
    ImGuiRenderer::NewDeathPopup(mGuiDeathPopupData);

    mFinishVisibleTimer = std::max(0.0f, mFinishVisibleTimer - static_cast<float>(deltaTime));
    mGuiFinishPopupData.visibleTimer = mFinishVisibleTimer;

    ImGuiRenderer::NewFinishPopup(mGuiFinishPopupData);
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
}
void CharacterEntity::OnCollisionStart(Entity *aOther) {

    if(aOther->CompareTag("deathzone")) {
        SPDLOG_INFO("I am {} and I collided with a death zone", GetName());
        mInternalEvents.push(InternalEvent::eDeath);
        Reset();
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

    SPDLOG_INFO("I am {} and I collided with {}", GetName(), aOther->GetName());

    // if its a high impact object, set the charactervirtual to ragdoll
    if(aOther->CompareTag("highimpact")) {
            mSampleJoltCharacter->SetRagdollMode(true);
            ragdollTime = GetTotalTime();
    }

    // if its a bounce object, set the velocity to 2.f magnitude in the direction between the character and the object
    if(aOther->CompareTag("bounce")) {
            glm::vec3 characterPosition = GetWorldTransformComponents().translation;
            glm::vec3  otherPosition = aOther->GetWorldTransformComponents().translation;
            glm::vec3  direction = characterPosition - otherPosition;
            direction = glm::normalize(direction);
            direction *= 70.f;
            mSampleJoltCharacter->SetCharacterImpulse(Vec3(direction.x, direction.y, direction.z));
    }

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
    mInitialTransform = GetLocalTransform();
    // create the jolt character
    CreateJoltCharacter();
    // register the character with the scene
    Scene::GetActiveScene()->SetMainCharacter(this);
    // register the teleport callback
    Camera::GetMainCamera()->SetTeleportCallbackFunction(std::bind(&CharacterEntity::TeleportCallback, this, std::placeholders::_1));

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
    for(auto &entity: Scene::GetActiveScene()->GetEntities())
    {
        if(entity->CompareTag("spawnpoint"))
        {
            SetCheckpoint(entity->GetWorldTransformComponents().translation + glm::vec3(0.f, 2.5f, 0.f));
            Reset();
        }
    }
}

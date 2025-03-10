//
// Created by thomas on 07/03/25.
//

#include "CharacterEntity.hpp"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <fstream>
#include <cstdlib>

void CharacterEntity::SetCharacterVirtual(unique_ptr<CharacterVirtualTest> &&uniquePtr) {
    mCharacterVirtual = std::move(uniquePtr);

}


CharacterEntity::~CharacterEntity() {
}
void CharacterEntity::Update(double deltaTime) {
    if(!mHasFirstFrameHappened) {
        mInitialTransform = GetTransform();
        mHasFirstFrameHappened = true;
    }

    Entity::Update(deltaTime);


    // get the character state
    // calculate the delta velocity
    Vec3 characterVelocityJolt = mCharacterVirtual->GetCharacterVelocity();
    glm::vec3 characterVelocity = glm::vec3(characterVelocityJolt.GetX(), characterVelocityJolt.GetY(), characterVelocityJolt.GetZ());
    // set the character to face the direction of the velocity without the y component
    characterVelocity.y = 0;
    if (glm::length(characterVelocity) > 0.1f) {
        // set the transform rotation to the direction of the velocity, on top of the initial transform rotation
        Transform newTransform = GetTransform();
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
    switch (mCharacterVirtual->GetJumpState()) {
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
    for (auto &child : mChildren) {
            if (child->HasAnimator()) {
                child->GetAnimator().SetActiveAnimation(activeAnimation, blend, playWholeAnimation);
                child->GetAnimator().SetTimeScale(timeScale);
            }
    }
}
CharacterEntity::CharacterEntity() {
    mHasCharacter = true;
    Load();
}
void CharacterEntity::OnCollisionStart(Entity *aOther) {

    //        SPDLOG_INFO("I am {} and I collided with {}", mName, aOther->mName);
    if(aOther->CompareTag("deathzone")) {
        SPDLOG_INFO("I am {} and I collided with a death zone", mName);
        Reset();
    }
    // if its a checkpoint, set the checkpoint
    if(aOther->CompareTag("checkpoint")) {
        // set the checkpoint to the position of the checkpoint, plus a bit in the y direction
        glm::vec3 checkpointPosition = aOther->getWorldTransformComponents().translation + glm::vec3(0, 2.5f, 0);
        SetCheckpoint(checkpointPosition);

    }
    SPDLOG_INFO("I am {} and I collided with {}", mName, aOther->mName);

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
    std::filesystem::path saveFile = savePath / "save.txt";

    // Open file for writing
    std::ofstream file(saveFile);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open save file for writing: {}", saveFile.string());
        return;
    }

    // Write character data
    file << "LastCheckpoint=" << mLastCheckpoint.x << "," << mLastCheckpoint.y << "," << mLastCheckpoint.z << std::endl;

    SPDLOG_INFO("Game saved to {}", saveFile.string());
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
    std::filesystem::path saveFile = savePath / "save.txt";

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
                } catch (const std::exception& e) {
                    SPDLOG_ERROR("Failed to parse checkpoint coordinates: {}", e.what());
                }
            }
            break;
        }
    }
}

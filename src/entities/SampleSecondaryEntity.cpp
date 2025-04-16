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
    if (IsKeyDown(KEY::eLEFT_SHIFT) && IsMouseButtonPressed(MOUSE_BUTTON::eLEFT)) {
        auto &flag = mCamera->inputMap[std::size_t(EInputState::MOUSING)];
        flag = false;
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
    mSampleJoltCharacter->ProcessInput(controlInput, jump, false);
}

SampleSecondaryEntity::SampleSecondaryEntity() {
    SetAsCharacter();
}

void SampleSecondaryEntity::Awake() {
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


    // move to spawn
    MoveToSpawn();
}

void SampleSecondaryEntity::PreUpdate(double deltaTime) {
    ProcessInput();
}

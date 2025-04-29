#include "Camera.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/NarrowPhaseQuery.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include "PhysicsManager.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"
#include "Scene.hpp"

#include "Input.hpp"
#include "glm/fwd.hpp"
#include "SDL.hpp"
#include "AudioManager.hpp"
Camera::Camera(const glm::vec3 position, glm::vec3 direction, glm::vec3 up)
    :
    m_position{position}, m_direction{direction}, m_up{up} {
    m_mouseSensitivity = 0.1f;
    m_controllerSensitivity = 100.f;
    m_increaseSpeed = 0.0f;
    m_cameraSpeed = defaultSpeed;
}

void Camera::UpdateCameraMovement(const Transform &character_transform) {
    glm::vec3 character_position = character_transform.translation;

    if(inputMap[std::size_t(EInputState::SWITCHVIEW)] == true)
    {
        if(m_inputType == InputType::FreeCamera){
            m_inputType = InputType::FollowCharacter;
        }
        else {
            m_inputType = InputType::FreeCamera;
        }
    }
    if(m_inputType == InputType::FollowCharacter) {
        glm::vec3 forward = glm::normalize(m_direction);
        glm::vec3 rightVector = glm::normalize(glm::cross(m_direction, m_up));

        if (inputMap[std::size_t(EInputState::ZOOM_IN)]) {
            zoom_level -= 0.1f;
        }
        if (inputMap[std::size_t(EInputState::ZOOM_OUT)]) {
            zoom_level += 0.1f;
        }

        glm::vec3 third_person_camera_offset =
            ((-2.f * forward) + (1.0f * m_up) + (0.25f * rightVector)) * zoom_level;

        RRayCast ray;
        ray.mOrigin = Vec3(character_position.x, character_position.y, character_position.z);
        ray.mDirection = Vec3(third_person_camera_offset.x, third_person_camera_offset.y,
                              third_person_camera_offset.z);

        JPH::RayCastResult result;
        JPH::SpecifiedObjectLayerFilter objectLayerFilter{Layers::NON_MOVING};
        bool hit = m_physics_reference->get().mPhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, result, {}, objectLayerFilter, {});

        if (hit) {
            m_position = character_position + third_person_camera_offset * result.mFraction;
        } else {
            m_position = character_position + third_person_camera_offset;
        }
    }
    else if(m_inputType == InputType::FreeCamera) {
        glm::vec3 forward = glm::normalize(m_direction);
        glm::vec3 rightVector = glm::normalize(glm::cross(m_direction, m_up));

        float camSpeed = m_cameraSpeed * (inputMap[std::size_t(EInputState::FAST)] ? 5.0f : 1.0f);

        if (inputMap[std::size_t(EInputState::FORWARD)]) {
            m_position += camSpeed * GlobalUtil::deltaTime * forward;
        }
        if (inputMap[std::size_t(EInputState::BACKWARD)]) {
            m_position -= camSpeed * GlobalUtil::deltaTime * forward;
        }
        if (inputMap[std::size_t(EInputState::LEFT)]) {
            m_position -= camSpeed * GlobalUtil::deltaTime * rightVector;
        }
        if (inputMap[std::size_t(EInputState::RIGHT)]) {
            m_position += camSpeed * GlobalUtil::deltaTime * rightVector;
        }
        if (inputMap[std::size_t(EInputState::UP)]) {
            m_position += camSpeed * GlobalUtil::deltaTime * m_up;
        }
        if (inputMap[std::size_t(EInputState::DOWN)]) {
            m_position -= camSpeed * GlobalUtil::deltaTime * m_up;
        }
        if (inputMap[std::size_t(EInputState::TELEPORT)]) {
            // call the teleport callback
            if (m_teleportCallback) {
                m_teleportCallback(m_position);
            }
        }
    }
    // update the audio listener position
    AudioManager::get().SetListenerPosition(m_position.x, m_position.y, m_position.z,
                                            m_direction.x, m_direction.y, m_direction.z,
                                            m_up.x, m_up.y, m_up.z);
}

void Camera::UpdateCameraRotation(double deltaTime) {
    // If we're using the mouse
        // check if this is the first time mouse is being used, if so skip updating
        // skip next frame so we have the correct lastx and lasty position for cursor
        glm::vec2 delta {};
        if (wasMousing) {
            delta = m_mouseSensitivity * GetMouseDelta();
            delta.y = -delta.y; // Prevent inverted y
        }
        // Get the right joystick input
        delta.x += GetGamepadAxis(SDL_INPUT::GamepadAxis::GAMEPAD_AXIS_RIGHT_X, 0) * m_controllerSensitivity * deltaTime;
        delta.y -= GetGamepadAxis(SDL_INPUT::GamepadAxis::GAMEPAD_AXIS_RIGHT_Y, 0) * m_controllerSensitivity * deltaTime;

        // Update the camera angles based on the mouse and controller movement
        UpdateCameraAngles(delta);
        UpdateCameraDirection();
    if (inputMap[std::size_t(EInputState::MOUSING)]) {
        wasMousing = true;
    } else {
        wasMousing = false;
    }
}

// Handling mouse movement based on learnings from
/* Joey De Vries (2020). Learn OpenGL: Learn modern OpenGL graphics programming in a step-by-step fashion. Kendall & Welling. */
void Camera::UpdateCameraAngles(const glm::vec2 &offset) {
    yaw += offset.x;
    pitch += offset.y;
    pitch = std::clamp(pitch, -89.0, 89.0);
}

// Compute the camera direction based on the cameras updated rotation
void Camera::UpdateCameraDirection() {
    glm::vec3 direction = {
        static_cast<float>(cos(glm::radians(yaw)) * cos(glm::radians(pitch))),
        static_cast<float>(sin(glm::radians(pitch))),
        static_cast<float>(sin(glm::radians(yaw)) * cos(glm::radians(pitch)))};

    m_direction = glm::normalize(direction);
}

void Camera::LogPosition() const {
    std::cout << "Position: " << m_position.x << ", " << m_position.y << ", " << m_position.z << " Speed: " << m_cameraSpeed << std::endl;
}

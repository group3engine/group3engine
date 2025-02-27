#include "Camera.hpp"

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Context.hpp"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/NarrowPhaseQuery.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "PhysicsManager.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"

#include "Input.hpp"
#include "glm/fwd.hpp"

Camera::Camera(Context &context, const glm::vec3 position, glm::vec3 direction, glm::vec3 up, float aspect)
    : context{context}, m_position{position}, m_direction{direction}, m_up{up} {
    m_mouseSensitivity = 0.1f;
    m_increaseSpeed = 0.0f;
    m_transform.view = glm::lookAt(position, position + direction, up);
    m_transform.projection = glm::perspective(m_transform.fov, aspect, m_transform.nearPlane, m_transform.farPlane);
    m_transform.projection[1][1] *= -1;
    m_transform.cameraPosition = glm::vec4(m_position.x, m_position.y, m_position.z, 1.0);
    m_transform.viewportSize = glm::vec2(context.extent.width, context.extent.height);
    m_transform.nearPlane = 0.1f;
    m_transform.farPlane = 100.0f;
    m_transform.fov = 45.0f;
    m_cameraSpeed = defaultSpeed;

    m_cameraUBO.resize(vkutil::MAX_FRAMES_IN_FLIGHT);
    for (auto &buffer : m_cameraUBO) {
        buffer = CreateBuffer("cameraUBO", context, sizeof(CameraTransform), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    }
}

Camera::~Camera() {
    for (auto &buffer : m_cameraUBO) {
        buffer.Destroy();
    }
}

void Camera::Update(uint32_t width, uint32_t height, [[maybe_unused]] double deltaTime, glm::vec3 character_position) {
    UpdateTransforms(width, height);

    // Write new data to the buffer to update uniform
    VkDeviceSize size = sizeof(CameraTransform);
    m_cameraUBO[vkutil::currentFrame].WriteToBuffer(m_transform, size);

    UpdateCameraRotation();
    UpdateCameraMovement(character_position);
}

void Camera::UpdateTransforms(uint32_t width, uint32_t height) {
    m_transform.model = glm::mat4(1.0f);
    m_transform.view = glm::lookAt(m_position, m_position + m_direction, m_up);
    m_transform.projection = glm::perspective(m_transform.fov, width / (float)height, m_transform.nearPlane, m_transform.farPlane);
    m_transform.projection[1][1] *= -1;
    m_transform.cameraPosition = glm::vec4(m_position.x, m_position.y, m_position.z, 1.0);
    m_transform.viewportSize = glm::vec2(width, height);
    m_transform.nearPlane = m_transform.nearPlane;
    m_transform.farPlane = m_transform.farPlane;
    m_transform.fov = m_transform.fov;
}

void Camera::UpdateCameraMovement(glm::vec3 input_position) {
    glm::vec3 forward = glm::normalize(m_direction);
    glm::vec3 rightVector = glm::normalize(glm::cross(m_direction, m_up));

    glm::vec3 third_person_camera_offset = (-2.f * forward) + (0.5f * m_up) + (0.5f * rightVector);

    RRayCast ray;
    ray.mOrigin = Vec3(input_position.x, input_position.y, input_position.z);
    ray.mDirection = Vec3(third_person_camera_offset.x, third_person_camera_offset.y, third_person_camera_offset.z);

    JPH::RayCastResult result;
    bool hit = m_physics_reference->get().mPhysicsSystem.GetNarrowPhaseQuery().CastRay(ray, result);

    if(hit)
    {
        m_position = input_position + third_person_camera_offset * result.mFraction;
    }
    else 
    {
        m_position = input_position + third_person_camera_offset;
    }



    //m_position = input_position + third_person_camera_offset;
}

void Camera::UpdateCameraRotation() {
    // If we're using the mouse
    if (inputMap[std::size_t(EInputState::MOUSING)]) {
        // check if this is the first time mouse is being used, if so skip updating
        // skip next frame so we have the correct lastx and lasty position for cursor
        if (wasMousing) {
            glm::vec2 delta = m_mouseSensitivity * GetMouseDelta();
            delta.y = -delta.y; // Prevent inverted y
            UpdateCameraAngles(delta);
            UpdateCameraDirection();
        }

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

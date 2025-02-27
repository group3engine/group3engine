#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Context.hpp"
#include "PhysicsManager.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"

constexpr float m_speedIncreaseAmount = 15.0f;
constexpr float m_speedDecreaseAmount = 8.0f;
constexpr float maxSpeed = 100.0;
constexpr float minSpeed = 1.0f;
constexpr float defaultSpeed = 3.0f;

struct CameraTransform {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec4 cameraPosition;
    alignas(8) glm::vec2 viewportSize;
    alignas(4) float fov;
    alignas(4) float nearPlane;
    alignas(4) float farPlane;
};

enum class EInputState {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    FAST,
    SLOW,
    MOUSING,
    MAX
};

class Camera {
  public:
    Camera(Context &context, const glm::vec3 position, glm::vec3 direction, glm::vec3 up, float aspect);
    ~Camera();

    void SetSpeed(float speed) { m_cameraSpeed = speed; }
    void SetPosition(glm::vec3 newpos) { m_position = newpos; }
    void SetDirection(glm::vec3 newdir) { m_direction = newdir; }
    void SetFoV(float fov) { m_transform.fov = fov; }
    void SetNearPlane(float nearPlane) { m_transform.nearPlane = nearPlane; }
    void SetFarPlane(float farPlane) { m_transform.farPlane = farPlane; }
    void SetPhysics(PhysicsManager* input_physics_reference) {m_physics_reference = input_physics_reference; }

    const CameraTransform &GetCameraTransform() const { return m_transform; }
    std::vector<Buffer> &GetBuffers() { return m_cameraUBO; }

    void Update(uint32_t width, uint32_t height, double deltaTime, glm::vec3 character_postion);
    void UpdateTransforms(uint32_t width, uint32_t height);
    void UpdateCameraMovement(glm::vec3 position);
    void UpdateCameraRotation();
    void UpdateCameraAngles(const glm::vec2 &offset);

    // Compute the camera direction based on the cameras updated rotation
    void UpdateCameraDirection();

    // Debug position log for the camera
    void LogPosition() const;

    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetDirection() const { return m_direction; }
    glm::vec3 GetUp() const { return m_up; }

    float GetFoV() { return m_transform.fov; }

    void SetInput(EInputState inputState, bool state) {
        inputMap[static_cast<size_t>(inputState)] = state;
    }

    bool inputMap[std::size_t(EInputState::MAX)] = {};
    bool wasMousing = false;

    

  private:
    Context &context;
    CameraTransform m_transform;
    std::vector<Buffer> m_cameraUBO;

    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_up;

    float m_cameraSpeed;
    float m_increaseSpeed;
    float m_mouseSensitivity;
    double yaw = 90.0f;
    double pitch = 0.0f;
    
    const PhysicsManager* m_physics_reference;
};
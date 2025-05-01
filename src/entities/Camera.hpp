#pragma once

#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "Entity.hpp"
#include "PhysicsManager.hpp"
#include "Utils.hpp"
#include "Buffer.hpp"

class Scene;

constexpr float m_speedIncreaseAmount = 15.0f;
constexpr float m_speedDecreaseAmount = 8.0f;
constexpr float maxSpeed = 100.0;
constexpr float minSpeed = 1.0f;
constexpr float defaultSpeed = 3.0f;

enum class ECrouchState;

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
    SWITCHVIEW,
    TELEPORT,
    ZOOM_IN,
    ZOOM_OUT,
    MAX
};
enum class InputType {
    FollowCharacter,
    FreeCamera
};
class Camera : public Entity {
  public:
    Camera(const glm::vec3 position, glm::vec3 direction, glm::vec3 up);

    bool IsActive() const { return m_isActive; }
    void SetIsActive(bool isActive) { m_isActive = isActive; }

    void SetSpeed(float speed) { m_cameraSpeed = speed; }
    void SetPosition(glm::vec3 newpos) { m_position = newpos; }
    void SetDirection(glm::vec3 newdir) { m_direction = newdir; }
    void SetPhysics(PhysicsManager* input_physics_reference) {m_physics_reference = input_physics_reference; }
    void SetScene(Scene* input_scene_pointer) {m_scene_pointer = input_scene_pointer; }

    void UpdateCameraMovement(const RVec3 &characterCOM, ECrouchState crouchState);
    void UpdateCameraRotation(double deltaTime);
    void UpdateCameraAngles(const glm::vec2 &offset);


    void SetTeleportCallbackFunction(const std::function<void(glm::vec3)> &callback) { m_teleportCallback = callback; }

    // Compute the camera direction based on the cameras updated rotation
    void UpdateCameraDirection();

    // Debug position log for the camera
    void LogPosition() const;

    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetDirection() const { return m_direction; }
    glm::vec3 GetUp() const { return m_up; }

    void SetInput(EInputState inputState, bool state) {
        inputMap[static_cast<size_t>(inputState)] = state;
    }
    void SetInput(EInputState inputState, float state)
    {
        if (state > 0.2f)
        {
            SetInput(inputState, true);
        }
        else
        {
            SetInput(inputState, false);
        }

    }

    bool inputMap[std::size_t(EInputState::MAX)] = {};
    float inputMapFloat[std::size_t(EInputState::MAX)] = {};
    bool wasMousing = false;

    void SetInputType(InputType inputType) { m_inputType = inputType; }

    [[nodiscard]] bool isInFreeCameraMode() const { return m_inputType == InputType::FreeCamera; }
    [[nodiscard]] bool isInFollowCharacterMode() const { return m_inputType == InputType::FollowCharacter; }

  public:
    inline static float sZoomLevel = 1.8f;

    inline static float sCameraUpOffset = 0.75f;
    inline static float sCameraCrouchingUpOffset = 0.3f;
    inline static float sCameraRightOffset = 0.0f;

  private:
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_up;

    float m_cameraSpeed;
    float m_increaseSpeed;
    float m_mouseSensitivity;
    float m_controllerSensitivity;
    double yaw = 90.0f;
    double pitch = 0.0f;
    function<void(glm::vec3)> m_teleportCallback = nullptr;

    const PhysicsManager* m_physics_reference;
    Scene* m_scene_pointer;


    InputType m_inputType = InputType::FollowCharacter;

    bool m_isActive = false;
};
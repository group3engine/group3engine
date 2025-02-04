//
// Created by thomas on 05/12/24.
//

#ifndef VEX4_USERSTATE_HPP
#define VEX4_USERSTATE_HPP

// for printf
#include <cstdio>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>


namespace labutils
{

    enum class EInputState
    {
        forward,
        backward,
        strafeLeft,
        strafeRight,
        levitate,
        sink,
        fast,
        slow,
        mousing,
        max
    };

    enum class DebugPipelineState
    {
        none,
        mipmaps,
        depth,
        depthPartialDerivatives,
        overdraw,
        overshade,
        deferred,
        meshDensity,
    };

    enum class PostProcessPipelineState
    {
        none,
        mosaic,
        bloom,
    };

    struct UserState
    {
        bool inputMap[std::size_t(EInputState::max)] = {};

        float mouseX = 0.f, mouseY = 0.f;
        float previousX = 0.f, previousY = 0.f;

        float phi = 0.f, theta = 0.f;
        glm::vec3 cameraPos = glm::vec3(0.f, 0.f, 0.f);

        bool wasMousing = true;

        glm::mat4 camera2world = glm::identity<glm::mat4>();

        DebugPipelineState debugPipelineState = DebugPipelineState::none;

        PostProcessPipelineState postProcessPipelineState = PostProcessPipelineState::none;
    };

    struct CameraSettings
    {
        constexpr static float kCameraBaseSpeed = 4.f;
        constexpr static float kCameraFastMultiplier = 5.f;
        constexpr static float kCameraSlowMultiplier = 0.05f;

        constexpr static float kCameraMouseSensitivity = 0.01f; // radians per pixel
    };

    void update_user_state(UserState &, float aElapsedTime, CameraSettings const &cfg);

    // GLFW callbacks
    void user_state_key_press(GLFWwindow *, int, int, int, int);
    void user_state_button(GLFWwindow *, int, int, int);
    void user_state_motion(GLFWwindow *, double, double);
}


#endif //VEX4_USERSTATE_HPP

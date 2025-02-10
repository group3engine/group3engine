//
// Created by thomas on 05/12/24.
//

#include "UserState.hpp"

namespace labutils
{

    // function that handles the mouse moving
    void user_state_motion(GLFWwindow *aWindow, double aX, double aY)
    {
        auto state = static_cast<UserState *>(glfwGetWindowUserPointer(aWindow));
        assert(state);
        state->mouseX = float(aX);
        state->mouseY = float(aY);
    }

    // function that handles key presses that will change the state of the user
    void user_state_key_press(GLFWwindow *aWindow, int aKey, int /*aScanCode*/, int aAction, int /*aModifierFlags*/)
    {
        if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction)
        {
            glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
            return;
        }

        auto state = static_cast<UserState *>(glfwGetWindowUserPointer(aWindow));
        assert(state);

        bool const isReleased = (GLFW_RELEASE == aAction);

        switch (aKey)
        {
            case GLFW_KEY_W:
                state->inputMap[std::size_t(EInputState::forward)] = !isReleased;
                break;
            case GLFW_KEY_S:
                state->inputMap[std::size_t(EInputState::backward)] = !isReleased;
                break;
            case GLFW_KEY_A:
                state->inputMap[std::size_t(EInputState::strafeLeft)] = !isReleased;
                break;
            case GLFW_KEY_D:
                state->inputMap[std::size_t(EInputState::strafeRight)] = !isReleased;
                break;
            case GLFW_KEY_E:
                state->inputMap[std::size_t(EInputState::levitate)] = !isReleased;
                break;
            case GLFW_KEY_Q:
                state->inputMap[std::size_t(EInputState::sink)] = !isReleased;
                break;
            case GLFW_KEY_LEFT_SHIFT:
                [[fallthrough]];
            case GLFW_KEY_RIGHT_SHIFT:
                state->inputMap[std::size_t(EInputState::fast)] = !isReleased;
                break;
            case GLFW_KEY_LEFT_CONTROL:
                [[fallthrough]];
            case GLFW_KEY_RIGHT_CONTROL:
                state->inputMap[std::size_t(EInputState::slow)] = !isReleased;
                break;
            default:
                break;
        }

        // switch for debug pipeline state
        if(aAction == GLFW_RELEASE)
        {
            switch (aKey)
            {
                // no debug pipeline
                case GLFW_KEY_1:
                    state->debugPipelineState = DebugPipelineState::none;
                    break;
                // mipmaps
                case GLFW_KEY_2:
                    state->debugPipelineState = DebugPipelineState::mipmaps;
                    break;
                // depth
                case GLFW_KEY_3:
                    state->debugPipelineState = DebugPipelineState::depth;
                    break;
                // depth partial derivatives
                case GLFW_KEY_4:
                    state->debugPipelineState = DebugPipelineState::depthPartialDerivatives;
                    break;
                // overdraw
                case GLFW_KEY_5:
                    state->debugPipelineState = DebugPipelineState::overdraw;
                    break;
                // overshade
                case GLFW_KEY_6:
                    state->debugPipelineState = DebugPipelineState::overshade;
                    break;
                // deferred
                case GLFW_KEY_7:
                    state->debugPipelineState = DebugPipelineState::deferred;
                    break;
                // mesh density
                case GLFW_KEY_8:
                    state->debugPipelineState = DebugPipelineState::meshDensity;
                    break;

            }

            // switch for post process pipeline state
            switch (aKey)
            {
                // no post process pipeline
                case GLFW_KEY_M:
                    if (state->postProcessPipelineState != PostProcessPipelineState::mosaic)
                        state->postProcessPipelineState = PostProcessPipelineState::mosaic;
                    else
                        state->postProcessPipelineState = PostProcessPipelineState::none;
                    break;

                // bloom
                case GLFW_KEY_B:
                    if (state->postProcessPipelineState != PostProcessPipelineState::bloom)
                        state->postProcessPipelineState = PostProcessPipelineState::bloom;
                    else
                        state->postProcessPipelineState = PostProcessPipelineState::none;
                    break;


            }

            // DEBUG - button to print the current position of the camera
            if (aKey == GLFW_KEY_P)
            {
                printf("Camera position: (%f, %f, %f, 1.0)\n", state->cameraPos.x, state->cameraPos.y, state->cameraPos.z);
            }
        }
    }

    // function that handles the mouse button presses
    void user_state_button(GLFWwindow *aWindow, int aButton, int aAction, int /*aModifierFlags*/)
    {
        auto state = static_cast<UserState *>(glfwGetWindowUserPointer(aWindow));
        assert(state);

        if (GLFW_MOUSE_BUTTON_RIGHT == aButton && GLFW_PRESS == aAction)
        {
            auto &flag = state->inputMap[std::size_t(EInputState::mousing)];

            flag = !flag;
            // if we are mousing, disable the cursor
            if (flag)
            {
                glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            } else
            {
                glfwSetInputMode(aWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
        }
    }


    void update_user_state(UserState &aState, float aElapsedTime, CameraSettings const &cfg)
    {
        auto &cam = aState.camera2world;
        auto &pos = aState.cameraPos;

        // if we are mousing, rotate the camera
        if (aState.inputMap[std::size_t(EInputState::mousing)])
        {
            // only mouse on the second frame so the previous position is valid
            if (aState.wasMousing)
            {
                auto const sens = cfg.kCameraMouseSensitivity;
                aState.phi += sens * (aState.previousX - aState.mouseX);
                aState.theta += sens * (aState.previousY - aState.mouseY);
            }
            aState.previousX = aState.mouseX;
            aState.previousY = aState.mouseY;
            aState.wasMousing = true;
        } else
        {
            aState.wasMousing = false;
        }

        glm::mat4 rotation = glm::rotate(aState.phi, glm::vec3(0.f, 1.f, 0.f)) *
                             glm::rotate(aState.theta, glm::vec3(1.f, 0.f, 0.f));


        auto const movementSpeed = aElapsedTime * cfg.kCameraBaseSpeed *
                                   (aState.inputMap[std::size_t(EInputState::fast)] ? cfg.kCameraFastMultiplier
                                                                                    : 1.f) *
                                   (aState.inputMap[std::size_t(EInputState::slow)] ? cfg.kCameraSlowMultiplier : 1.f);

        glm::vec4 forward = rotation * glm::vec4(0.f, 0.f, -movementSpeed, 0.f);
        glm::vec4 right = rotation * glm::vec4(movementSpeed, 0.f, 0.f, 0.f);
        glm::vec4 up = glm::vec4(0.f, movementSpeed, 0.f, 0.f);

        if(aState.inputMap[std::size_t(EInputState::forward)])
            pos += glm::vec3(forward);
        if(aState.inputMap[std::size_t(EInputState::backward)])
            pos -= glm::vec3(forward);
        if(aState.inputMap[std::size_t(EInputState::strafeLeft)])
            pos -= glm::vec3(right);
        if(aState.inputMap[std::size_t(EInputState::strafeRight)])
            pos += glm::vec3(right);
        if(aState.inputMap[std::size_t(EInputState::levitate)])
            pos += glm::vec3(up);
        if(aState.inputMap[std::size_t(EInputState::sink)])
            pos -= glm::vec3(up);

        glm::mat4 translation = glm::translate(pos);
        cam = translation * rotation;
    }
}
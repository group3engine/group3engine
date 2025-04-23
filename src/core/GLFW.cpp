/* Code in this file is based on code from the raylib programming library. */

/**********************************************************************************************
*
*   LICENSE: zlib/libpng
*
*   Copyright (c) 2013-2025 Ramon Santamaria (@raysan5) and contributors
*
*   This software is provided "as-is", without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such, and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
**********************************************************************************************/

#include "GLFW.hpp"
#include "controllers.h"

#include <cstdlib>

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

// Using system from https://github.com/raysan5/raylib/blob/master/src/platforms/rcore_desktop_glfw.c
static void KeyCallback([[maybe_unused]] GLFWwindow *window,
                        int key,
                        [[maybe_unused]] int scancode,
                        int action,
                        [[maybe_unused]] int mods) {
    if (key < 0) {
        return;
    }

    if (action == GLFW_RELEASE) {
        gInputData.keyboard.currentKeyState[key] = 0;
    } else if (action == GLFW_PRESS) {
        gInputData.keyboard.currentKeyState[key] = 1;
    }
}

static void MouseButtonCallback([[maybe_unused]] GLFWwindow *window,
                                int button,
                                int action,
                                [[maybe_unused]] int mods) {
    if (action == GLFW_RELEASE) {
        gInputData.mouse.currentButtonState[button] = 0;
    } else if (action == GLFW_PRESS) {
        gInputData.mouse.currentButtonState[button] = 1;
    }
}

static void MouseCursorPosCallback([[maybe_unused]] GLFWwindow *window, double x, double y) {
    gInputData.mouse.currentPosition = {x, y};
}

static void PollGamepadJoysticks()
{
    // Register previous gamepad button states
    for(size_t j = 0; j <= GLFW_JOYSTICK_LAST; j++) {
        for (uint8_t i = 0; i < static_cast<uint8_t>(GAMEPAD_BUTTON::eLAST); ++i) {
            gInputData.gamepadButtons[j].previousButtonState[i] =
                    gInputData.gamepadButtons[j].currentButtonState[i];
        }
    }

    for (size_t j = 0; j <= GLFW_JOYSTICK_LAST; j++) {
        for (uint8_t i = 0; i < static_cast<uint8_t>(GAMEPAD_AXIS::eLAST); ++i) {
            gInputData.gamepadAxis[j].previousAxisState[i] =
                    gInputData.gamepadAxis[j].currentAxisState[i];
        }
    }

    // Check all possible gamepad connections (GLFW supports up to 16 gamepads)
    for (int gamepad = GLFW_JOYSTICK_1; gamepad <= GLFW_JOYSTICK_LAST; gamepad++) {
        if (glfwJoystickPresent(gamepad)) {
            // Poll button states
            int buttonCount;
            const unsigned char* buttons = glfwGetJoystickButtons(gamepad, &buttonCount);

            int count = (buttonCount < static_cast<int>(GAMEPAD_BUTTON::eLAST)) ?
                         buttonCount : static_cast<int>(GAMEPAD_BUTTON::eLAST);

            for (int i = 0; i < count; i++) {
                gInputData.gamepadButtons[gamepad].currentButtonState[i] = buttons[i];
            }

            // Poll axes (focused on analog sticks)
            int axisCount;
            const float* axes = glfwGetJoystickAxes(gamepad, &axisCount);

            int axesCount = (axisCount < static_cast<int>(GAMEPAD_AXIS::eLAST)) ?
                             axisCount : static_cast<int>(GAMEPAD_AXIS::eLAST);

            for (int i = 0; i < axesCount; i++) {
                gInputData.gamepadAxis[gamepad].currentAxisState[i] = axes[i];
            }

        }
    }
}



void Platform::StartUp(int windowWidth, int windowHeight) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    glfwUpdateGamepadMappings(controllerDB.c_str());

    GLFWmonitor *monitor = glfwGetPrimaryMonitor();

#ifdef PLATINUM
    const GLFWvidmode *mode = glfwGetVideoMode(monitor);
    // set the window size to the monitor size
    windowWidth = mode->width;
    windowHeight = mode->height;
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
    glfwWindowHint(GLFW_FOCUSED, GLFW_TRUE);
#endif

#ifdef PLATINUM
    Platform::get().window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", monitor, nullptr);
#else
    Platform::get().window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);
#endif

    if (!Platform::get().window) {
        SPDLOG_ERROR("Failed to create GLFW window");
        std::exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(Platform::get().window, &KeyCallback);
    glfwSetMouseButtonCallback(Platform::get().window, &MouseButtonCallback);
    glfwSetCursorPosCallback(Platform::get().window, &MouseCursorPosCallback);

}

void Platform::ShutDown() {
    glfwDestroyWindow(Platform::get().window);
    glfwTerminate();
}


void PollInputEvents() {
    // TODO: More polling handling

    // Register previous key states
    for (uint16_t i = 0; i < static_cast<uint16_t>(KEY::eLAST); ++i) {
        gInputData.keyboard.previousKeyState[i] =
            gInputData.keyboard.currentKeyState[i];
    }

    // Register previous mouse states
    for (uint8_t i = 0; i < static_cast<uint8_t>(MOUSE_BUTTON::eLAST); ++i) {
        gInputData.mouse.previousButtonState[i] =
            gInputData.mouse.currentButtonState[i];
    }


    // Register previous mouse position
    gInputData.mouse.previousPosition = gInputData.mouse.currentPosition;

    glfwPollEvents();

    // Poll gamepad joysticks
    PollGamepadJoysticks();
}


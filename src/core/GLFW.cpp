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
    // Register previous gamepad button and axis states
    for (uint8_t i = 0; i < static_cast<uint8_t>(GAMEPAD_BUTTON::eLAST); ++i) {
        gInputData.gamepadButtons.previousButtonState[i] =
            gInputData.gamepadButtons.currentButtonState[i];
    }

    for (uint8_t i = 0; i < static_cast<uint8_t>(GAMEPAD_AXIS::eLAST); ++i) {
        gInputData.gamepadAxis.previousAxisState[i] =
            gInputData.gamepadAxis.currentAxisState[i];
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
                gInputData.gamepadButtons.currentButtonState[i] = buttons[i];
            }

            // Poll axes (focused on analog sticks)
            int axisCount;
            const float* axes = glfwGetJoystickAxes(gamepad, &axisCount);

            int axesCount = (axisCount < static_cast<int>(GAMEPAD_AXIS::eLAST)) ?
                             axisCount : static_cast<int>(GAMEPAD_AXIS::eLAST);

            for (int i = 0; i < axesCount; i++) {
                gInputData.gamepadAxis.currentAxisState[i] = axes[i];
            }

            // Only process the first connected gamepad
            break;
        }
    }
    // print the right stick
    SPDLOG_INFO("Right stick: ({}, {})", gInputData.gamepadAxis.currentAxisState[2], gInputData.gamepadAxis.currentAxisState[3]);
    SPDLOG_INFO("All gamepad axis values: ({}, {}, {}, {}, {}, {})", gInputData.gamepadAxis.currentAxisState[0], gInputData.gamepadAxis.currentAxisState[1], gInputData.gamepadAxis.currentAxisState[2], gInputData.gamepadAxis.currentAxisState[3], gInputData.gamepadAxis.currentAxisState[4], gInputData.gamepadAxis.currentAxisState[5]);
}



void Platform::StartUp(int windowWidth, int windowHeight) {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    Platform::get().window = glfwCreateWindow(windowWidth, windowHeight, "Vulkan", nullptr, nullptr);

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


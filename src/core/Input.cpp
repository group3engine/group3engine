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

#include "Input.hpp"

#include <cmath>

InputData gInputData;

// Using system from https://github.com/raysan5/raylib/blob/master/src/rcore.c

// Check if a key has been pressed once
bool IsKeyPressed(KEY key) {
    return gInputData.keyboard.previousKeyState[static_cast<uint16_t>(key)] == 0 &&
           gInputData.keyboard.currentKeyState[static_cast<uint16_t>(key)] == 1;
}

bool IsKeyDown(KEY key) {
    return gInputData.keyboard.currentKeyState[static_cast<uint16_t>(key)] == 1;
}

// Check if a key has been released once
bool IsKeyReleased(KEY key) {
    return gInputData.keyboard.previousKeyState[static_cast<uint16_t>(key)] == 1 &&
           gInputData.keyboard.currentKeyState[static_cast<uint16_t>(key)] == 0;
}

// get an axis value from the gamepad
float GetGamepadAxis(GAMEPAD_AXIS axis) {
    // only return the axis value if it isn't almost zero
    if (std::abs(gInputData.gamepadAxis.currentAxisState[static_cast<uint8_t>(axis)]) < 0.1f) {
        return 0.0f;
    }
    return gInputData.gamepadAxis.currentAxisState[static_cast<uint8_t>(axis)];
}

// Check if a gamepad button has been pressed once
bool IsGamepadButtonPressed(GAMEPAD_BUTTON button) {
    return gInputData.gamepadButtons.currentButtonState[static_cast<uint8_t>(button)] == 1 &&
           gInputData.gamepadButtons.previousButtonState[static_cast<uint8_t>(button)] == 0;
}

bool IsGamepadButtonDown(GAMEPAD_BUTTON button) {
    return gInputData.gamepadButtons.currentButtonState[static_cast<uint8_t>(button)] == 1;
}



// Check if a mouse button has been pressed once
bool IsMouseButtonPressed(MOUSE_BUTTON button) {
    return gInputData.mouse.currentButtonState[static_cast<uint8_t>(button)] == 1 &&
           gInputData.mouse.previousButtonState[static_cast<uint8_t>(button)] == 0;
}

bool IsMouseButtonDown(MOUSE_BUTTON button) {
    return gInputData.mouse.currentButtonState[static_cast<uint8_t>(button)] == 1;
}

// Check if a mouse button has been released once
bool IsMouseButtonReleased(MOUSE_BUTTON button) {
    return gInputData.mouse.currentButtonState[static_cast<uint8_t>(button)] == 0 &&
           gInputData.mouse.previousButtonState[static_cast<uint8_t>(button)] == 1;
}



float GetMouseX() {
    return gInputData.mouse.currentPosition.x;
}

float GetMouseY() {
    return gInputData.mouse.currentPosition.y;
}

glm::vec2 GetMousePosition() {
    return gInputData.mouse.currentPosition;
}

glm::vec2 GetMouseDelta() {
    return gInputData.mouse.currentPosition - gInputData.mouse.previousPosition;
}

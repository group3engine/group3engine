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
SDL_INPUT::InputData SDL_INPUT::INPUT;

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
float GetGamepadAxis(SDL_INPUT::GamepadAxis axis, int gamepad) {
    return SDL_INPUT::INPUT.Gamepad.axisState[gamepad][static_cast<uint8_t>(axis)];
}

// Check if a gamepad button has been pressed once
bool IsGamepadButtonPressed(SDL_INPUT::GamepadButton button, int gamepad) {
    return SDL_INPUT::INPUT.Gamepad.currentButtonState[gamepad][static_cast<uint8_t>(button)] == 1 &&
              SDL_INPUT::INPUT.Gamepad.previousButtonState[gamepad][static_cast<uint8_t>(button)] == 0;
}

bool IsGamepadButtonDown(SDL_INPUT::GamepadButton button, int gamepad) {
    return SDL_INPUT::INPUT.Gamepad.currentButtonState[gamepad][static_cast<uint8_t>(button)] == 1;
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

void NormaliseDPad(float& x, float& y)
{
    // if there is only 0 or one axis pressed, return the value. Otherwise, we want to normalize it
    if(x != 0.f && y != 0.f)
    {
        x = x > 0.f ? 1.f / sqrt(2.f) : -1.f / sqrt(2.f);
        y = y > 0.f ? 1.f / sqrt(2.f) : -1.f / sqrt(2.f);
    }
}


#include "InputData.hpp"
#include "Input.hpp"

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

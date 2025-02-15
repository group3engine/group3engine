#include "InputData.hpp"
#include "Input.hpp"

InputData gInputData;

// Using system from https://github.com/raysan5/raylib/blob/master/src/rcore.c
bool IsKeyDown(KEY key) {
    return gInputData.keyboard.currentKeyState[static_cast<uint16_t>(key)] == 1;
}

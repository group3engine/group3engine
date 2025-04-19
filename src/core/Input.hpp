#ifndef GROUP3ENGINE_INPUT_HPP
#define GROUP3ENGINE_INPUT_HPP

#include "InputData.hpp"

bool IsKeyPressed(KEY key);

bool IsKeyDown(KEY key);

bool IsKeyReleased(KEY key);

bool IsMouseButtonPressed(MOUSE_BUTTON button);

bool IsMouseButtonDown(MOUSE_BUTTON button);

bool IsMouseButtonReleased(MOUSE_BUTTON button);

float GetGamepadAxis(GAMEPAD_AXIS axis, int gamepad);

bool IsGamepadButtonPressed(GAMEPAD_BUTTON button, int gamepad);

bool IsGamepadButtonDown(GAMEPAD_BUTTON button, int gamepad);

float GetMouseX();

float GetMouseY();

glm::vec2 GetMousePosition();

glm::vec2 GetMouseDelta();
#endif // GROUP3ENGINE_INPUT_HPP

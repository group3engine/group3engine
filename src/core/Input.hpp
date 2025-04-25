#ifndef GROUP3ENGINE_INPUT_HPP
#define GROUP3ENGINE_INPUT_HPP

#include "InputData.hpp"
#include "SDL.hpp"

/// Returns true if the key transitioned from unpressed to pressed during the current frame.
bool IsKeyPressed(KEY key);

/// Returns true if the key is held down.
bool IsKeyDown(KEY key);

/// Returns true if the key transitioned from pressed to unpressed during the current frame.
bool IsKeyReleased(KEY key);

/// Returns true if the mouse button transitioned from unpressed to pressed during the current frame.
bool IsMouseButtonPressed(MOUSE_BUTTON button);

/// Returns true if the mouse button is held down.
bool IsMouseButtonDown(MOUSE_BUTTON button);

/// Returns true if the mouse button transitioned from pressed to unpressed during the current frame.
bool IsMouseButtonReleased(MOUSE_BUTTON button);

/// Returns the value of the gamepad axis
float GetGamepadAxis(SDL_INPUT::GamepadAxis axis, int gamepad);

/// Returns true if the gamepad button transitioned from unpressed to pressed during the current frame.
bool IsGamepadButtonPressed(SDL_INPUT::GamepadButton button, int gamepad);

/// Returns true if the gamepad button is held down.
bool IsGamepadButtonDown(SDL_INPUT::GamepadButton button, int gamepad);

float GetMouseX();

float GetMouseY();

glm::vec2 GetMousePosition();

glm::vec2 GetMouseDelta();
/// Normalises the dpad input to be between either magnitude 0 and 1
void NormaliseDPad(float& x, float& y);
#endif // GROUP3ENGINE_INPUT_HPP

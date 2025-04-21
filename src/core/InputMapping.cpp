//
// Created by thomas on 19/04/25.
//

#include "InputMapping.hpp"
#include "InputData.hpp"
#include "Input.hpp"

float InputMapping::GetActionDown(const std::string &action) const
{
    // find the max value of the action
    float maxValue = 0;
    auto it = mKeyBindings.find(action);
    if (it != mKeyBindings.end()) {
        for (int key : it->second) {
            if (IsKeyDown(static_cast<KEY>(key))) {
                if(std::abs(maxValue) < std::abs(key)) {
                    maxValue = key;
                }
            }
        }
    }
    auto itg = mGamepadBindings.find(action);
    if (itg != mGamepadBindings.end()) {
        for (const auto &[button, gamepad] : itg->second) {
            if (IsGamepadButtonDown(static_cast<GAMEPAD_BUTTON>(button), gamepad)) {
                if(std::abs(maxValue) < std::abs(button)) {
                    maxValue = button;
                }
            }
        }
    }
    itg = mGamepadAxisBindings.find(action);
    if (itg != mGamepadAxisBindings.end()) {
        for (const auto &[axis, gamepad] : itg->second) {
            float value = GetGamepadAxis(static_cast<GAMEPAD_AXIS>(axis), gamepad);
            if (std::abs(value) > mGamepadDeadzone) {
                if (std::abs(maxValue) < std::abs(value)) {
                    maxValue = value;
                }
            }
        }
    }
    it = mMouseBindings.find(action);
    if (it != mMouseBindings.end()) {
        for (int button : it->second) {
            if (IsMouseButtonDown(static_cast<MOUSE_BUTTON>(button))) {
                if (std::abs(maxValue) < std::abs(button)) {
                    maxValue = button;
                }
            }
        }
    }
    it = mMouseAxisBindings.find(action);
    if (it != mMouseAxisBindings.end()) {
        for (int axis : it->second) {
            float value = GetMouseDelta()[axis];
            if (std::abs(maxValue) < std::abs(value)) {
                maxValue = value;
            }

        }
    }
    return maxValue;
}

void InputMapping::RemoveBinding(std::unordered_map<std::string, std::vector<int>> &bindings, const std::string &action,
                                 int value)
{
    auto it = bindings.find(action);
    if (it != bindings.end()) {
        auto &values = it->second;
        values.erase(std::remove(values.begin(), values.end(), value), values.end());
        if (values.empty()) {
            bindings.erase(it);
        }
    }
}

void InputMapping::RemoveBinding(std::unordered_map<std::string, std::vector<std::tuple<int, int>>> &bindings,
                                 const std::string &action, int value, int gamepad)
{
    auto it = bindings.find(action);
    if (it != bindings.end()) {
        auto &values = it->second;
        values.erase(std::remove_if(values.begin(), values.end(),
                                     [value, gamepad](const std::tuple<int, int> &binding) {
                                         return std::get<0>(binding) == value && std::get<1>(binding) == gamepad;
                                     }), values.end());
        if (values.empty()) {
            bindings.erase(it);
        }
    }
}

float InputMapping::GetActionPressed(const std::string &action) const
{
    // find the max value of the action
    float maxValue = 0;
    auto it = mKeyBindings.find(action);
    if (it != mKeyBindings.end()) {
        for (int key : it->second) {
            if (IsKeyPressed(static_cast<KEY>(key))) {
                if(std::abs(maxValue) < std::abs(key)) {
                    maxValue = key;
                }
            }
        }
    }
    auto itg = mGamepadBindings.find(action);
    if (itg != mGamepadBindings.end()) {
        for (const auto &[button, gamepad] : itg->second) {
            if (IsGamepadButtonPressed(static_cast<GAMEPAD_BUTTON>(button), gamepad)) {
                if(std::abs(maxValue) < std::abs(button)) {
                    maxValue = button;
                }
            }
        }
    }
    itg = mGamepadAxisBindings.find(action);
    if (itg != mGamepadAxisBindings.end()) {
        for (const auto &[axis, gamepad] : itg->second) {
            float value = GetGamepadAxis(static_cast<GAMEPAD_AXIS>(axis), gamepad);
            if (value > mGamepadDeadzone) {
                if (std::abs(maxValue) < std::abs(value)) {
                    maxValue = value;
                }
            }
        }
    }
    it = mMouseBindings.find(action);
    if (it != mMouseBindings.end()) {
        for (int button : it->second) {
            if (IsMouseButtonPressed(static_cast<MOUSE_BUTTON>(button))) {
                if (std::abs(maxValue) < std::abs(button)) {
                    maxValue = button;
                }
            }
        }
    }
    it = mMouseAxisBindings.find(action);
    if (it != mMouseAxisBindings.end()) {
        for (int axis : it->second) {
            float value = GetMouseDelta()[axis];
            if (std::abs(maxValue) < std::abs(value)) {
                maxValue = value;
            }

        }
    }
    return maxValue;
}

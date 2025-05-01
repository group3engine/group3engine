//
// Created by thomas on 19/04/25.
//

#include "InputMapping.hpp"

#include <algorithm>

#include "InputData.hpp"
#include "Input.hpp"

float InputMapping::GetActionDown(const std::string &action) const
{
    // get the bindings for this action
    float maxValue = 0;
    auto it = mBindings.find(action);
    if (it != mBindings.end()) {
        for (Binding key : it->second) {
            float value;
            switch(key.type) {
                case BindingType::Key:
                    if (IsKeyDown(key.mapping.key)) {
                        value = 1.0f;
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::GamepadButton:
                    if (IsGamepadButtonDown(key.mapping.gamepadButton.value, key.mapping.gamepadButton.index)) {
                        value = 1.f;
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::GamepadAxis:
                    value = GetGamepadAxis(key.mapping.gamepadAxis.value, key.mapping.gamepadAxis.index);
                    if (std::abs(value) > mGamepadDeadzone) {
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::MouseButton:
                    if (IsMouseButtonDown(key.mapping.mouseButton)) {
                        value = 1.0f;
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::MouseAxis:
                    value = GetMouseDelta()[static_cast<int>(key.mapping.mouseAxis)];
                    if(std::abs(maxValue) < std::abs(value)) {
                        maxValue = value;
                    }
                    break;
            }
        }
    }
    return maxValue;
}

float InputMapping::GetActionPressed(const std::string &action) const
{
    // get the bindings for this action
    float maxValue = 0;
    auto it = mBindings.find(action);
    if (it != mBindings.end()) {
        for (Binding key : it->second) {
            float value;
            switch(key.type) {
                case BindingType::Key:
                    if (IsKeyPressed(key.mapping.key)) {
                        value = 1.0f;
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::GamepadButton:
                    if (IsGamepadButtonPressed(key.mapping.gamepadButton.value, key.mapping.gamepadButton.index)) {
                        value = 1.0f;
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::GamepadAxis:
                    value = GetGamepadAxis(key.mapping.gamepadAxis.value, key.mapping.gamepadAxis.index);
                    if (std::abs(value) > mGamepadDeadzone) {
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::MouseButton:
                    if (IsMouseButtonPressed(key.mapping.mouseButton)) {
                        value = 1.0f;
                        if(std::abs(maxValue) < std::abs(value)) {
                            maxValue = value;
                        }
                    }
                    break;
                case BindingType::MouseAxis:
                    value = GetMouseDelta()[static_cast<int>(key.mapping.mouseAxis)];
                    if(std::abs(maxValue) < std::abs(value)) {
                        maxValue = value;
                    }
                    break;
            }
        }
    }
    return maxValue;
}

void InputMapping::RemoveBinding(const std::string &action,
                                 Binding binding)
{
    auto it = mBindings.find(action);
    if (it != mBindings.end()) {
        auto &values = it->second;
        auto bindingIt = std::remove_if(values.begin(), values.end(),
                                         [&binding](const Binding &b) { return b == binding; });
        if (bindingIt != values.end()) {
            values.erase(bindingIt, values.end());
        }
        if (values.empty()) {
            mBindings.erase(it);
        }
    }
}

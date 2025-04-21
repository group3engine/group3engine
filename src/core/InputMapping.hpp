//
// Created by thomas on 19/04/25.
//

#ifndef GROUP3ENGINE_INPUTMAPPING_HPP
#define GROUP3ENGINE_INPUTMAPPING_HPP
#include <string>
#include <unordered_map>
#include <vector>

#include "InputData.hpp"

/// @brief The InputMapping class is a singleton that manages the input mappings for the game.
class InputMapping {
public:
    /// @brief Default constructor.
    InputMapping() = default;
    ~InputMapping() = default;

    /// @brief Add a keyboard binding for an action.
    void AddKeyBinding(const std::string &action, KEY key) { mKeyBindings[action].push_back(static_cast<int>(key)); }
    /// @brief Add a gamepad button binding for an action.
    void AddGamepadButtonBinding(const std::string &action, GAMEPAD_BUTTON button, int gamepad) { mGamepadBindings[action].emplace_back(static_cast<int>(button), gamepad); }
    /// @brief Add a gamepad axis binding for an action.
    void AddGamepadAxisBinding(const std::string &action, GAMEPAD_AXIS axis, int gamepad) { mGamepadAxisBindings[action].emplace_back(static_cast<int>(axis), gamepad); }
    /// @brief Add a mouse button binding for an action.
    void AddMouseBinding(const std::string &action, MOUSE_BUTTON button) { mMouseBindings[action].push_back(static_cast<int>(button)); }
    /// @brief Add a mouse axis binding for an action.
    void AddMouseAxisBinding(const std::string &action, MOUSE_AXIS axis) { mMouseAxisBindings[action].push_back(static_cast<int>(axis)); }
    /// @brief Remove a keyboard binding for an action.
    void RemoveKeyBinding(const std::string &action, KEY key) { 
        RemoveBinding(mKeyBindings, action, static_cast<int>(key)); 
    }
    /// @brief Remove a gamepad button binding for an action.
    void RemoveGamepadBinding(const std::string &action, GAMEPAD_BUTTON button, int gamepad) { 
        RemoveBinding(mGamepadBindings, action, static_cast<int>(button), gamepad); 
    }
    /// @brief Remove a gamepad axis binding for an action.
    void RemoveGamepadAxisBinding(const std::string &action, GAMEPAD_AXIS axis, int gamepad) { 
        RemoveBinding(mGamepadAxisBindings, action, static_cast<int>(axis), gamepad); 
    }
    /// @brief Remove a mouse button binding for an action.
    void RemoveMouseBinding(const std::string &action, MOUSE_BUTTON button) { 
        RemoveBinding(mMouseBindings, action, static_cast<int>(button)); 
    }
    /// @brief Remove a mouse axis binding for an action.
    void RemoveMouseAxisBinding(const std::string &action, MOUSE_AXIS axis) { 
        RemoveBinding(mMouseAxisBindings, action, static_cast<int>(axis)); 
    }
    /// @brief Remove all bindings on all devices for an action.
    void RemoveAllBindings(const std::string &action) {
        mKeyBindings.erase(action);
        mGamepadBindings.erase(action);
        mGamepadAxisBindings.erase(action);
        mMouseBindings.erase(action);
        mMouseAxisBindings.erase(action);
    }

    /// @brief Get the value of an action. Returns the maximum magnitude value of all bindings for the action.
    float GetActionDown(const std::string &action) const;
    /// @brief For buttons and keys, this only returns 1 if the button is first pressed down this frame. Get the value of an action.
    float GetActionPressed(const std::string &action) const;

    /// @brief Set the gamepad deadzone.
    void SetGamepadDeadzone(float deadzone) { mGamepadDeadzone = deadzone; }


private:
    std::unordered_map<std::string, std::vector<int>> mKeyBindings;
    std::unordered_map<std::string, std::vector<std::tuple<int, int>>> mGamepadBindings;
    std::unordered_map<std::string, std::vector<std::tuple<int, int>>> mGamepadAxisBindings;
    std::unordered_map<std::string, std::vector<int>> mMouseBindings;
    std::unordered_map<std::string, std::vector<int>> mMouseAxisBindings;

    void RemoveBinding(std::unordered_map<std::string, std::vector<int>> &bindings, const std::string &action, int value);
    void RemoveBinding(std::unordered_map<std::string, std::vector<std::tuple<int, int>>> &bindings, const std::string &action, int value, int gamepad);


    float mGamepadDeadzone = 0.1f; // Deadzone for gamepad axis
};


#endif //GROUP3ENGINE_INPUTMAPPING_HPP

//
// Created by thomas on 19/04/25.
//

#ifndef GROUP3ENGINE_INPUTMAPPING_HPP
#define GROUP3ENGINE_INPUTMAPPING_HPP
#include <string>
#include <unordered_map>
#include <vector>

#include "InputData.hpp"
#include "SDL.hpp"

enum class BindingType {
    Key,
    GamepadButton,
    GamepadAxis,
    MouseButton,
    MouseAxis,
};
struct GamepadButtonMapping {
    SDL_INPUT::GamepadButton value;    // Represents button or axis value
    int index;    // Represents the gamepad index or ID
};
struct GamepadAxisMapping {
    SDL_INPUT::GamepadAxis value;    // Represents button or axis value
    int index;    // Represents the gamepad index or ID
};

struct Binding {
    BindingType type;
    union {
        KEY key;
        // button/axis, gamepad
        GamepadButtonMapping gamepadButton;
        GamepadAxisMapping gamepadAxis;
        MOUSE_BUTTON mouseButton;
        MOUSE_AXIS mouseAxis;
    } mapping {};
    // Constructor for key binding
    Binding(KEY k) : type(BindingType::Key){mapping.key = k;}
    // Constructor for gamepad button binding
    Binding(SDL_INPUT::GamepadButton b, int index) : type(BindingType::GamepadButton){mapping.gamepadButton = {b, index};}
    // Constructor for gamepad axis binding
    Binding(SDL_INPUT::GamepadAxis a, int index) : type(BindingType::GamepadAxis){mapping.gamepadAxis = {a, index};}
    // Constructor for mouse button binding
    Binding(MOUSE_BUTTON b) : type(BindingType::MouseButton){mapping.mouseButton = b;}
    // Constructor for mouse axis binding
    Binding(MOUSE_AXIS a) : type(BindingType::MouseAxis){mapping.mouseAxis = a;}
    // equality operator
    bool operator==(const Binding &other) const {
        if (type != other.type) {
            return false;
        }
        switch (type) {
            case BindingType::Key:
                return mapping.key == other.mapping.key;
            case BindingType::GamepadButton:
                return mapping.gamepadButton.value == other.mapping.gamepadButton.value &&
                       mapping.gamepadButton.index == other.mapping.gamepadButton.index;
            case BindingType::GamepadAxis:
                return mapping.gamepadAxis.value == other.mapping.gamepadAxis.value &&
                       mapping.gamepadAxis.index == other.mapping.gamepadAxis.index;
            case BindingType::MouseButton:
                return mapping.mouseButton == other.mapping.mouseButton;
            case BindingType::MouseAxis:
                return mapping.mouseAxis == other.mapping.mouseAxis;
        }
        return false;
    }


};

/// @brief The InputMapping class is a singleton that manages the input mappings for the game.
class InputMapping {
public:
    /// @brief Default constructor.
    InputMapping() = default;
    ~InputMapping() = default;

    /// @brief Add a keyboard binding for an action.
    void AddBinding(const std::string &action, KEY key) { mBindings[action].emplace_back(key); }
    /// @brief Add a gamepad button binding for an action.
    void AddBinding(const std::string &action, SDL_INPUT::GamepadButton button, int gamepad) { mBindings[action].emplace_back(button, gamepad); }
    /// @brief Add a gamepad axis binding for an action.
    void AddBinding(const std::string &action, SDL_INPUT::GamepadAxis axis, int gamepad) { mBindings[action].emplace_back(axis, gamepad); }
    /// @brief Add a mouse button binding for an action.
    void AddBinding(const std::string &action, MOUSE_BUTTON button) { mBindings[action].push_back(button); }
    /// @brief Add a mouse axis binding for an action.
    void AddBinding(const std::string &action, MOUSE_AXIS axis) { mBindings[action].push_back(axis); }
    /// @brief Remove a keyboard binding for an action.
    void RemoveBinding(const std::string &action, KEY key) {
        Binding binding = {key};
        RemoveBinding(action, binding);
    }
    /// @brief Remove a gamepad button binding for an action.
    void RemoveBinding(const std::string &action, SDL_INPUT::GamepadButton button, int gamepad) {
        Binding binding = {button, gamepad};
        RemoveBinding(action, binding);
    }
    /// @brief Remove a gamepad axis binding for an action.
    void RemoveBinding(const std::string &action, SDL_INPUT::GamepadAxis axis, int gamepad) {
        Binding binding = {axis, gamepad};
        RemoveBinding(action, binding);
    }
    /// @brief Remove a mouse button binding for an action.
    void RemoveBinding(const std::string &action, MOUSE_BUTTON button) {
        Binding binding = {button};
        RemoveBinding(action, binding);
    }
    /// @brief Remove a mouse axis binding for an action.
    void RemoveBinding(const std::string &action, MOUSE_AXIS axis) {
        Binding binding = {axis};
        RemoveBinding(action, binding);
    }
    /// @brief Remove all bindings on all devices for an action.
    void RemoveAllBindings(const std::string &action) {
        mBindings.erase(action);
    }

    /// @brief Get the value of an action. Returns the maximum magnitude value of all bindings for the action.
    float GetActionDown(const std::string &action) const;
    /// @brief For buttons and keys, this only returns 1 if the button is first pressed down this frame. Get the value of an action.
    float GetActionPressed(const std::string &action) const;

    /// @brief Set the gamepad deadzone.
    void SetGamepadDeadzone(float deadzone) { mGamepadDeadzone = deadzone; }


private:

    std::unordered_map<std::string, std::vector<Binding>> mBindings;

    void RemoveBinding(const std::string &action, Binding binding);



    float mGamepadDeadzone = 0.1f; // Deadzone for gamepad axis
};


#endif //GROUP3ENGINE_INPUTMAPPING_HPP

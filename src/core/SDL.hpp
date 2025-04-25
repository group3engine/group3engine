#pragma once

#include "SDL3/SDL_scancode.h"
#include "SDL3/SDL_video.h"
#include <array>

namespace SDL_INPUT
{
#define MAX_GAMEPADS 4
#define MAX_GAMEPAD_VIBRATION_TIME 10.0f
#define MAX_KEYBOARD_KEYS 512
#define MAX_KEY_PRESSED_QUEUE 16
#define MAX_CHAR_PRESSED_QUEUE 16
#define MAX_MOUSE_BUTTONS 8
#define MAX_TOUCH_POINTS 8
#define MAX_GAMEPAD_NAME_LENGTH 128
#define MAX_GAMEPAD_BUTTONS 32
#define MAX_GAMEPAD_AXIS 8
#define SCANCODE_MAPPED_NUM 232

// Keyboard keys (US keyboard layout)
// NOTE: Use GetKeyPressed() to allow redefining
// required keys for alternative layouts
    typedef enum {
        KEY_NULL = 0,        // Key: NULL, used for no key pressed
        // Alphanumeric keys
        KEY_APOSTROPHE = 39,       // Key: '
        KEY_COMMA = 44,       // Key: ,
        KEY_MINUS = 45,       // Key: -
        KEY_PERIOD = 46,       // Key: .
        KEY_SLASH = 47,       // Key: /
        KEY_ZERO = 48,       // Key: 0
        KEY_ONE = 49,       // Key: 1
        KEY_TWO = 50,       // Key: 2
        KEY_THREE = 51,       // Key: 3
        KEY_FOUR = 52,       // Key: 4
        KEY_FIVE = 53,       // Key: 5
        KEY_SIX = 54,       // Key: 6
        KEY_SEVEN = 55,       // Key: 7
        KEY_EIGHT = 56,       // Key: 8
        KEY_NINE = 57,       // Key: 9
        KEY_SEMICOLON = 59,       // Key: ;
        KEY_EQUAL = 61,       // Key: =
        KEY_A = 65,       // Key: A | a
        KEY_B = 66,       // Key: B | b
        KEY_C = 67,       // Key: C | c
        KEY_D = 68,       // Key: D | d
        KEY_E = 69,       // Key: E | e
        KEY_F = 70,       // Key: F | f
        KEY_G = 71,       // Key: G | g
        KEY_H = 72,       // Key: H | h
        KEY_I = 73,       // Key: I | i
        KEY_J = 74,       // Key: J | j
        KEY_K = 75,       // Key: K | k
        KEY_L = 76,       // Key: L | l
        KEY_M = 77,       // Key: M | m
        KEY_N = 78,       // Key: N | n
        KEY_O = 79,       // Key: O | o
        KEY_P = 80,       // Key: P | p
        KEY_Q = 81,       // Key: Q | q
        KEY_R = 82,       // Key: R | r
        KEY_S = 83,       // Key: S | s
        KEY_T = 84,       // Key: T | t
        KEY_U = 85,       // Key: U | u
        KEY_V = 86,       // Key: V | v
        KEY_W = 87,       // Key: W | w
        KEY_X = 88,       // Key: X | x
        KEY_Y = 89,       // Key: Y | y
        KEY_Z = 90,       // Key: Z | z
        KEY_LEFT_BRACKET = 91,       // Key: [
        KEY_BACKSLASH = 92,       // Key: '\'
        KEY_RIGHT_BRACKET = 93,       // Key: ]
        KEY_GRAVE = 96,       // Key: `
        // Function keys
        KEY_SPACE = 32,       // Key: Space
        KEY_ESCAPE = 256,      // Key: Esc
        KEY_ENTER = 257,      // Key: Enter
        KEY_TAB = 258,      // Key: Tab
        KEY_BACKSPACE = 259,      // Key: Backspace
        KEY_INSERT = 260,      // Key: Ins
        KEY_DELETE = 261,      // Key: Del
        KEY_RIGHT = 262,      // Key: Cursor right
        KEY_LEFT = 263,      // Key: Cursor left
        KEY_DOWN = 264,      // Key: Cursor down
        KEY_UP = 265,      // Key: Cursor up
        KEY_PAGE_UP = 266,      // Key: Page up
        KEY_PAGE_DOWN = 267,      // Key: Page down
        KEY_HOME = 268,      // Key: Home
        KEY_END = 269,      // Key: End
        KEY_CAPS_LOCK = 280,      // Key: Caps lock
        KEY_SCROLL_LOCK = 281,      // Key: Scroll down
        KEY_NUM_LOCK = 282,      // Key: Num lock
        KEY_PRINT_SCREEN = 283,      // Key: Print screen
        KEY_PAUSE = 284,      // Key: Pause
        KEY_F1 = 290,      // Key: F1
        KEY_F2 = 291,      // Key: F2
        KEY_F3 = 292,      // Key: F3
        KEY_F4 = 293,      // Key: F4
        KEY_F5 = 294,      // Key: F5
        KEY_F6 = 295,      // Key: F6
        KEY_F7 = 296,      // Key: F7
        KEY_F8 = 297,      // Key: F8
        KEY_F9 = 298,      // Key: F9
        KEY_F10 = 299,      // Key: F10
        KEY_F11 = 300,      // Key: F11
        KEY_F12 = 301,      // Key: F12
        KEY_LEFT_SHIFT = 340,      // Key: Shift left
        KEY_LEFT_CONTROL = 341,      // Key: Control left
        KEY_LEFT_ALT = 342,      // Key: Alt left
        KEY_LEFT_SUPER = 343,      // Key: Super left
        KEY_RIGHT_SHIFT = 344,      // Key: Shift right
        KEY_RIGHT_CONTROL = 345,      // Key: Control right
        KEY_RIGHT_ALT = 346,      // Key: Alt right
        KEY_RIGHT_SUPER = 347,      // Key: Super right
        KEY_KB_MENU = 348,      // Key: KB menu
        // Keypad keys
        KEY_KP_0 = 320,      // Key: Keypad 0
        KEY_KP_1 = 321,      // Key: Keypad 1
        KEY_KP_2 = 322,      // Key: Keypad 2
        KEY_KP_3 = 323,      // Key: Keypad 3
        KEY_KP_4 = 324,      // Key: Keypad 4
        KEY_KP_5 = 325,      // Key: Keypad 5
        KEY_KP_6 = 326,      // Key: Keypad 6
        KEY_KP_7 = 327,      // Key: Keypad 7
        KEY_KP_8 = 328,      // Key: Keypad 8
        KEY_KP_9 = 329,      // Key: Keypad 9
        KEY_KP_DECIMAL = 330,      // Key: Keypad .
        KEY_KP_DIVIDE = 331,      // Key: Keypad /
        KEY_KP_MULTIPLY = 332,      // Key: Keypad *
        KEY_KP_SUBTRACT = 333,      // Key: Keypad -
        KEY_KP_ADD = 334,      // Key: Keypad +
        KEY_KP_ENTER = 335,      // Key: Keypad Enter
        KEY_KP_EQUAL = 336,      // Key: Keypad =
        // Android key buttons
        KEY_BACK = 4,        // Key: Android back button
        KEY_MENU = 5,        // Key: Android menu button
        KEY_VOLUME_UP = 24,       // Key: Android volume up button
        KEY_VOLUME_DOWN = 25        // Key: Android volume down button
    } KeyboardKey;

// Gamepad axis
    typedef enum {
        GAMEPAD_AXIS_LEFT_X = 0,     // Gamepad left stick X axis
        GAMEPAD_AXIS_LEFT_Y = 1,     // Gamepad left stick Y axis
        GAMEPAD_AXIS_RIGHT_X = 2,     // Gamepad right stick X axis
        GAMEPAD_AXIS_RIGHT_Y = 3,     // Gamepad right stick Y axis
        GAMEPAD_AXIS_LEFT_TRIGGER = 4,     // Gamepad back trigger left, pressure level: [1..-1]
        GAMEPAD_AXIS_RIGHT_TRIGGER = 5      // Gamepad back trigger right, pressure level: [1..-1]
    } GamepadAxis;
// Gamepad buttons
    typedef enum {
        GAMEPAD_BUTTON_UNKNOWN = 0,         // Unknown button, just for error checking
        GAMEPAD_BUTTON_LEFT_FACE_UP,        // Gamepad left DPAD up button
        GAMEPAD_BUTTON_LEFT_FACE_RIGHT,     // Gamepad left DPAD right button
        GAMEPAD_BUTTON_LEFT_FACE_DOWN,      // Gamepad left DPAD down button
        GAMEPAD_BUTTON_LEFT_FACE_LEFT,      // Gamepad left DPAD left button
        GAMEPAD_BUTTON_Y,       // Gamepad right button up (i.e. PS3: Triangle, Xbox: Y)
        GAMEPAD_BUTTON_B,    // Gamepad right button right (i.e. PS3: Circle, Xbox: B)
        GAMEPAD_BUTTON_A,     // Gamepad right button down (i.e. PS3: Cross, Xbox: A)
        GAMEPAD_BUTTON_X,     // Gamepad right button left (i.e. PS3: Square, Xbox: X)
        GAMEPAD_BUTTON_LEFT_TRIGGER_1,      // Gamepad top/back trigger left (first), it could be a trailing button
        GAMEPAD_BUTTON_LEFT_TRIGGER_2,      // Gamepad top/back trigger left (second), it could be a trailing button
        GAMEPAD_BUTTON_RIGHT_TRIGGER_1,     // Gamepad top/back trigger right (first), it could be a trailing button
        GAMEPAD_BUTTON_RIGHT_TRIGGER_2,     // Gamepad top/back trigger right (second), it could be a trailing button
        GAMEPAD_BUTTON_MIDDLE_LEFT,         // Gamepad center buttons, left one (i.e. PS3: Select)
        GAMEPAD_BUTTON_MIDDLE,              // Gamepad center buttons, middle one (i.e. PS3: PS, Xbox: XBOX)
        GAMEPAD_BUTTON_MIDDLE_RIGHT,        // Gamepad center buttons, right one (i.e. PS3: Start)
        GAMEPAD_BUTTON_LEFT_THUMB,          // Gamepad joystick pressed button left
        GAMEPAD_BUTTON_RIGHT_THUMB          // Gamepad joystick pressed button right
    } GamepadButton;
    // Vector2 type
    typedef struct Vector2 {
        float x;
        float y;
    } Vector2;

    typedef struct {
        struct {
            int exitKey;                    // Default exit key
            std::array<char, MAX_KEYBOARD_KEYS> currentKeyState{};        // Registers current frame key state
            std::array<char, MAX_KEYBOARD_KEYS> previousKeyState{};       // Registers previous frame key state

            // NOTE: Since key press logic involves comparing prev vs cur key state, we need to handle key repeats specially
            std::array<char, MAX_KEYBOARD_KEYS> keyRepeatInFrame{};       // Registers key repeats for current frame

            std::array<int, MAX_KEY_PRESSED_QUEUE> keyPressedQueue{};     // Input keys queue
            int keyPressedQueueCount;       // Input keys queue count

            std::array<int, MAX_CHAR_PRESSED_QUEUE> charPressedQueue{};   // Input characters queue (unicode)
            int charPressedQueueCount;      // Input characters queue count

        } Keyboard;
        struct {
            Vector2 offset;                 // Mouse offset
            Vector2 scale;                  // Mouse scaling
            Vector2 currentPosition;        // Mouse position on screen
            Vector2 previousPosition;       // Previous mouse position

            int cursor;                     // Tracks current mouse cursor
            bool cursorHidden;              // Track if cursor is hidden
            bool cursorOnScreen;            // Tracks if cursor is inside client area

            std::array<char, MAX_MOUSE_BUTTONS> currentButtonState;     // Registers current mouse button state
            std::array<char, MAX_MOUSE_BUTTONS> previousButtonState;    // Registers previous mouse button state
            Vector2 currentWheelMove;       // Registers current mouse wheel variation
            Vector2 previousWheelMove;      // Registers previous mouse wheel variation

        } Mouse;
        struct {
            int lastButtonPressed;          // Register last gamepad button pressed
            std::array<int, MAX_GAMEPADS> axisCount;                          // Register number of available gamepad axis
            std::array<bool, MAX_GAMEPADS> ready;                             // Flag to know if gamepad is ready
            std::array<std::array<char, MAX_GAMEPAD_NAME_LENGTH>, MAX_GAMEPADS> name;               // Gamepad name holder
            std::array<std::array<char, MAX_GAMEPAD_BUTTONS>, MAX_GAMEPADS> currentButtonState;     // Current gamepad buttons state
            std::array<std::array<char, MAX_GAMEPAD_BUTTONS>, MAX_GAMEPADS> previousButtonState;    // Previous gamepad buttons state
            std::array<std::array<float, MAX_GAMEPAD_AXIS>, MAX_GAMEPADS> axisState;                // Gamepad axis state
        } Gamepad;
    } InputData;
    int InitPlatform();                                      // Initialize platform (graphics, inputs and more)
    void ClosePlatform();                                    // Close platform
    static KeyboardKey ConvertScancodeToKey(SDL_Scancode sdlScancode);  // Help convert SDL scancodes to raylib key

    void SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration); // Set gamepad vibration
    void PollInputEvents();                                   // Register all input events

    extern InputData INPUT;                // Input data

    inline int numJoysticks = 0;


} // namespace SDL_INPUT
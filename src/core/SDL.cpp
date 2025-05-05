//
// Created by thomas on 25/04/25.
//

#include "SDL.hpp"
#include <spdlog/spdlog.h>
namespace SDL_INPUT
{
/* Code in this file is based on code from the raylib programming library. */

/**********************************************************************************************
*
*   rcore_desktop_sdl - Functions to manage window, graphics device and inputs
*
*   PLATFORM: DESKTOP: SDL
*       - Windows (Win32, Win64)
*       - Linux (X11/Wayland desktop mode)
*       - Others (not tested)
*
*   LIMITATIONS:
*       - Limitation 01
*       - Limitation 02
*
*   POSSIBLE IMPROVEMENTS:
*       - Improvement 01
*       - Improvement 02
*
*   ADDITIONAL NOTES:
*       - TRACELOG() function is located in raylib [utils] module
*
*   CONFIGURATION:
*       #define RCORE_PLATFORM_CUSTOM_FLAG
*           Custom flag for rcore on target platform -not used-
*
*   DEPENDENCIES:
*       - SDL 2 or SDL 3 (main library): Windowing and inputs management
*       - gestures: Gestures system for touch-ready devices (or simulated from mouse inputs)
*
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


#include "SDL.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include "controllers.h"


    static const KeyboardKey mapScancodeToKey[SCANCODE_MAPPED_NUM] = {
            KEY_NULL,           // SDL_SCANCODE_UNKNOWN
            KEY_NULL,
            KEY_NULL,
            KEY_NULL,
            KEY_A,              // SDL_SCANCODE_A
            KEY_B,              // SDL_SCANCODE_B
            KEY_C,              // SDL_SCANCODE_C
            KEY_D,              // SDL_SCANCODE_D
            KEY_E,              // SDL_SCANCODE_E
            KEY_F,              // SDL_SCANCODE_F
            KEY_G,              // SDL_SCANCODE_G
            KEY_H,              // SDL_SCANCODE_H
            KEY_I,              // SDL_SCANCODE_I
            KEY_J,              // SDL_SCANCODE_J
            KEY_K,              // SDL_SCANCODE_K
            KEY_L,              // SDL_SCANCODE_L
            KEY_M,              // SDL_SCANCODE_M
            KEY_N,              // SDL_SCANCODE_N
            KEY_O,              // SDL_SCANCODE_O
            KEY_P,              // SDL_SCANCODE_P
            KEY_Q,              // SDL_SCANCODE_Q
            KEY_R,              // SDL_SCANCODE_R
            KEY_S,              // SDL_SCANCODE_S
            KEY_T,              // SDL_SCANCODE_T
            KEY_U,              // SDL_SCANCODE_U
            KEY_V,              // SDL_SCANCODE_V
            KEY_W,              // SDL_SCANCODE_W
            KEY_X,              // SDL_SCANCODE_X
            KEY_Y,              // SDL_SCANCODE_Y
            KEY_Z,              // SDL_SCANCODE_Z
            KEY_ONE,            // SDL_SCANCODE_1
            KEY_TWO,            // SDL_SCANCODE_2
            KEY_THREE,          // SDL_SCANCODE_3
            KEY_FOUR,           // SDL_SCANCODE_4
            KEY_FIVE,           // SDL_SCANCODE_5
            KEY_SIX,            // SDL_SCANCODE_6
            KEY_SEVEN,          // SDL_SCANCODE_7
            KEY_EIGHT,          // SDL_SCANCODE_8
            KEY_NINE,           // SDL_SCANCODE_9
            KEY_ZERO,           // SDL_SCANCODE_0
            KEY_ENTER,          // SDL_SCANCODE_RETURN
            KEY_ESCAPE,         // SDL_SCANCODE_ESCAPE
            KEY_BACKSPACE,      // SDL_SCANCODE_BACKSPACE
            KEY_TAB,            // SDL_SCANCODE_TAB
            KEY_SPACE,          // SDL_SCANCODE_SPACE
            KEY_MINUS,          // SDL_SCANCODE_MINUS
            KEY_EQUAL,          // SDL_SCANCODE_EQUALS
            KEY_LEFT_BRACKET,   // SDL_SCANCODE_LEFTBRACKET
            KEY_RIGHT_BRACKET,  // SDL_SCANCODE_RIGHTBRACKET
            KEY_BACKSLASH,      // SDL_SCANCODE_BACKSLASH
            KEY_NULL,                  // SDL_SCANCODE_NONUSHASH
            KEY_SEMICOLON,      // SDL_SCANCODE_SEMICOLON
            KEY_APOSTROPHE,     // SDL_SCANCODE_APOSTROPHE
            KEY_GRAVE,          // SDL_SCANCODE_GRAVE
            KEY_COMMA,          // SDL_SCANCODE_COMMA
            KEY_PERIOD,         // SDL_SCANCODE_PERIOD
            KEY_SLASH,          // SDL_SCANCODE_SLASH
            KEY_CAPS_LOCK,      // SDL_SCANCODE_CAPSLOCK
            KEY_F1,             // SDL_SCANCODE_F1
            KEY_F2,             // SDL_SCANCODE_F2
            KEY_F3,             // SDL_SCANCODE_F3
            KEY_F4,             // SDL_SCANCODE_F4
            KEY_F5,             // SDL_SCANCODE_F5
            KEY_F6,             // SDL_SCANCODE_F6
            KEY_F7,             // SDL_SCANCODE_F7
            KEY_F8,             // SDL_SCANCODE_F8
            KEY_F9,             // SDL_SCANCODE_F9
            KEY_F10,            // SDL_SCANCODE_F10
            KEY_F11,            // SDL_SCANCODE_F11
            KEY_F12,            // SDL_SCANCODE_F12
            KEY_PRINT_SCREEN,   // SDL_SCANCODE_PRINTSCREEN
            KEY_SCROLL_LOCK,    // SDL_SCANCODE_SCROLLLOCK
            KEY_PAUSE,          // SDL_SCANCODE_PAUSE
            KEY_INSERT,         // SDL_SCANCODE_INSERT
            KEY_HOME,           // SDL_SCANCODE_HOME
            KEY_PAGE_UP,        // SDL_SCANCODE_PAGEUP
            KEY_DELETE,         // SDL_SCANCODE_DELETE
            KEY_END,            // SDL_SCANCODE_END
            KEY_PAGE_DOWN,      // SDL_SCANCODE_PAGEDOWN
            KEY_RIGHT,          // SDL_SCANCODE_RIGHT
            KEY_LEFT,           // SDL_SCANCODE_LEFT
            KEY_DOWN,           // SDL_SCANCODE_DOWN
            KEY_UP,             // SDL_SCANCODE_UP
            KEY_NUM_LOCK,       // SDL_SCANCODE_NUMLOCKCLEAR
            KEY_KP_DIVIDE,      // SDL_SCANCODE_KP_DIVIDE
            KEY_KP_MULTIPLY,    // SDL_SCANCODE_KP_MULTIPLY
            KEY_KP_SUBTRACT,    // SDL_SCANCODE_KP_MINUS
            KEY_KP_ADD,         // SDL_SCANCODE_KP_PLUS
            KEY_KP_ENTER,       // SDL_SCANCODE_KP_ENTER
            KEY_KP_1,           // SDL_SCANCODE_KP_1
            KEY_KP_2,           // SDL_SCANCODE_KP_2
            KEY_KP_3,           // SDL_SCANCODE_KP_3
            KEY_KP_4,           // SDL_SCANCODE_KP_4
            KEY_KP_5,           // SDL_SCANCODE_KP_5
            KEY_KP_6,           // SDL_SCANCODE_KP_6
            KEY_KP_7,           // SDL_SCANCODE_KP_7
            KEY_KP_8,           // SDL_SCANCODE_KP_8
            KEY_KP_9,           // SDL_SCANCODE_KP_9
            KEY_KP_0,           // SDL_SCANCODE_KP_0
            KEY_KP_DECIMAL,     // SDL_SCANCODE_KP_PERIOD
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_NULL, KEY_NULL, KEY_NULL, KEY_NULL,
            KEY_LEFT_CONTROL,   //SDL_SCANCODE_LCTRL
            KEY_LEFT_SHIFT,     //SDL_SCANCODE_LSHIFT
            KEY_LEFT_ALT,       //SDL_SCANCODE_LALT
            KEY_LEFT_SUPER,     //SDL_SCANCODE_LGUI
            KEY_RIGHT_CONTROL,  //SDL_SCANCODE_RCTRL
            KEY_RIGHT_SHIFT,    //SDL_SCANCODE_RSHIFT
            KEY_RIGHT_ALT,      //SDL_SCANCODE_RALT
            KEY_RIGHT_SUPER     //SDL_SCANCODE_RGUI
    };


//----------------------------------------------------------------------------------
// Types and Structures Definition
//----------------------------------------------------------------------------------
    typedef struct {

        SDL_Gamepad *gamepad[MAX_GAMEPADS];
        SDL_JoystickID gamepadId[MAX_GAMEPADS]; // Joystick instance ids
    } PlatformData;



    static PlatformData platform = {0};   // Platform specific data
    SDL_Window *window = nullptr; // Pointer to the SDL window


//----------------------------------------------------------------------------------
// Module Internal Functions Declaration
//----------------------------------------------------------------------------------


// Set internal gamepad mappings
    int SetGamepadMappings()
    {
        return SDL_AddGamepadMapping(controllerDB.c_str());
    }

// Set gamepad vibration
    void SetGamepadVibration(int gamepad, float leftMotor, float rightMotor, float duration)
    {
        if ((gamepad < MAX_GAMEPADS) && INPUT.Gamepad.ready[gamepad] && (duration > 0.0f)) {
            if (leftMotor < 0.0f) leftMotor = 0.0f;
            if (leftMotor > 1.0f) leftMotor = 1.0f;
            if (rightMotor < 0.0f) rightMotor = 0.0f;
            if (rightMotor > 1.0f) rightMotor = 1.0f;
            if (duration > MAX_GAMEPAD_VIBRATION_TIME) duration = MAX_GAMEPAD_VIBRATION_TIME;

            SDL_RumbleGamepad(platform.gamepad[gamepad], (Uint16) (leftMotor * 65535.0f),
                              (Uint16) (rightMotor * 65535.0f), (Uint32) (duration * 1000.0f));
        }
    }


// Get physical key name.
    const char *GetKeyName(int key)
    {
        return SDL_GetKeyName(key);
    }

// Register all input events
    void PollInputEvents()
    {

        // Reset keys/chars pressed registered
        INPUT.Keyboard.keyPressedQueueCount = 0;
        INPUT.Keyboard.charPressedQueueCount = 0;

        // Reset mouse wheel
        INPUT.Mouse.currentWheelMove.x = 0;
        INPUT.Mouse.currentWheelMove.y = 0;

        INPUT.Mouse.previousPosition = INPUT.Mouse.currentPosition;

        // Reset last gamepad button/axis registered state
        int numJoysticks;
        SDL_JoystickID *joysticks = SDL_GetGamepads(&numJoysticks);
        for (int i = 0; (i < numJoysticks) && (i < MAX_GAMEPADS); i++) {
            // Check if gamepad is available
            if (INPUT.Gamepad.ready[i]) {
                // Register previous gamepad button states
                for (int k = 0; k < MAX_GAMEPAD_BUTTONS; k++) {
                    INPUT.Gamepad.previousButtonState[i][k] = INPUT.Gamepad.currentButtonState[i][k];
                }
            }
        }
        SDL_free(joysticks);
        // Register previous keys states
        // NOTE: Android supports up to 260 keys
        for (int i = 0; i < MAX_KEYBOARD_KEYS; i++) {
            INPUT.Keyboard.previousKeyState[i] = INPUT.Keyboard.currentKeyState[i];
            INPUT.Keyboard.keyRepeatInFrame[i] = 0;
        }

        // Register previous mouse states
        for (int i = 0; i < MAX_MOUSE_BUTTONS; i++)
            INPUT.Mouse.previousButtonState[i] = INPUT.Mouse.currentButtonState[i];

        SDL_Event event = {0};
        while (SDL_PollEvent(&event) != 0) {
            // All input events can be processed after polling
            switch (event.type) {

                // Keyboard events
                case SDL_EVENT_KEY_DOWN: {
                    KeyboardKey key = ConvertScancodeToKey(event.key.scancode);

                    if (key != KEY_NULL) {
                        // If key was up, add it to the key pressed queue
                        if ((INPUT.Keyboard.currentKeyState[key] == 0) &&
                            (INPUT.Keyboard.keyPressedQueueCount < MAX_KEY_PRESSED_QUEUE)) {
                            INPUT.Keyboard.keyPressedQueue[INPUT.Keyboard.keyPressedQueueCount] = key;
                            INPUT.Keyboard.keyPressedQueueCount++;
                        }

                        INPUT.Keyboard.currentKeyState[key] = 1;
                    }

                    if (event.key.repeat) INPUT.Keyboard.keyRepeatInFrame[key] = 1;

                }
                    break;

                case SDL_EVENT_KEY_UP: {

                    KeyboardKey key = ConvertScancodeToKey(event.key.scancode);
                    if (key != KEY_NULL) INPUT.Keyboard.currentKeyState[key] = 0;
                }
                    break;


                    // Check mouse events
                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    // NOTE: SDL2 mouse button order is LEFT, MIDDLE, RIGHT, but raylib uses LEFT, RIGHT, MIDDLE like GLFW
                    //       The following conditions align SDL with raylib.h MouseButton enum order
                    int btn = event.button.button - 1;
                    if (btn == 2) btn = 1;
                    else if (btn == 1) btn = 2;

                    INPUT.Mouse.currentButtonState[btn] = 1;

                }
                    break;
                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    // NOTE: SDL2 mouse button order is LEFT, MIDDLE, RIGHT, but raylib uses LEFT, RIGHT, MIDDLE like GLFW
                    //       The following conditions align SDL with raylib.h MouseButton enum order
                    int btn = event.button.button - 1;
                    if (btn == 2) btn = 1;
                    else if (btn == 1) btn = 2;

                    INPUT.Mouse.currentButtonState[btn] = 0;
                }
                    break;
                case SDL_EVENT_MOUSE_WHEEL: {
                    INPUT.Mouse.currentWheelMove.x = (float) event.wheel.x;
                    INPUT.Mouse.currentWheelMove.y = (float) event.wheel.y;
                }
                    break;
                case SDL_EVENT_MOUSE_MOTION: {
                    INPUT.Mouse.currentPosition.x = (float) event.motion.x;
                    INPUT.Mouse.currentPosition.y = (float) event.motion.y;

                }
                    break;

                    // Check gamepad events
                case SDL_EVENT_GAMEPAD_ADDED: {
                    int jid = event.jdevice.which; // Joystick instance id
                    if(numJoysticks >= MAX_GAMEPADS) {
                        SPDLOG_ERROR("PLATFORM: Maximum number of gamepads reached");
                        break;
                    }
                    int gamepadIndex = numJoysticks++;

                    if (!INPUT.Gamepad.ready[gamepadIndex] && (jid < MAX_GAMEPADS)) {
                        platform.gamepad[gamepadIndex] = SDL_OpenGamepad(jid);
                        platform.gamepadId[gamepadIndex] = SDL_GetJoystickID(SDL_GetGamepadJoystick(platform.gamepad[gamepadIndex]));

                        if (platform.gamepad[gamepadIndex]) {
                            INPUT.Gamepad.ready[gamepadIndex] = true;
                            INPUT.Gamepad.axisCount[gamepadIndex] = SDL_GetNumJoystickAxes(
                                    SDL_GetGamepadJoystick(platform.gamepad[gamepadIndex]));
                            INPUT.Gamepad.axisState[gamepadIndex][GAMEPAD_AXIS_LEFT_TRIGGER] = -1.0f;
                            INPUT.Gamepad.axisState[gamepadIndex][GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.0f;
                            strncpy(INPUT.Gamepad.name[gamepadIndex].data(), SDL_GetGamepadNameForID(jid), MAX_GAMEPAD_NAME_LENGTH - 1);
                            INPUT.Gamepad.name[gamepadIndex][MAX_GAMEPAD_NAME_LENGTH - 1] = '\0';
                        } else {
                            SPDLOG_ERROR("PLATFORM: Unable to open game controller [ERROR: %s]", SDL_GetError());
                        }
                    }
                }
                    break;
                case SDL_EVENT_GAMEPAD_REMOVED: {
                    int jid = event.jdevice.which; // Joystick instance id

                    for (int i = 0; i < MAX_GAMEPADS; i++) {
                        if (platform.gamepadId[i] == jid) {
                            SDL_CloseGamepad(platform.gamepad[i]);
                            INPUT.Gamepad.ready[i] = false;
                            memset(INPUT.Gamepad.name[i].data(), 0, MAX_GAMEPAD_NAME_LENGTH);
                            platform.gamepadId[i] = -1;
                            break;
                        }
                    }
                }
                    break;
                case SDL_EVENT_GAMEPAD_BUTTON_DOWN: {
                    int button = -1;

                    switch (event.jbutton.button) {
                        case SDL_GAMEPAD_BUTTON_NORTH:
                            button = GAMEPAD_BUTTON_Y;
                            break;
                        case SDL_GAMEPAD_BUTTON_EAST:
                            button = GAMEPAD_BUTTON_B;
                            break;
                        case SDL_GAMEPAD_BUTTON_SOUTH:
                            button = GAMEPAD_BUTTON_A;
                            break;
                        case SDL_GAMEPAD_BUTTON_WEST:
                            button = GAMEPAD_BUTTON_X;
                            break;

                        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                            button = GAMEPAD_BUTTON_LEFT_TRIGGER_1;
                            break;
                        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                            button = GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
                            break;

                        case SDL_GAMEPAD_BUTTON_BACK:
                            button = GAMEPAD_BUTTON_MIDDLE_LEFT;
                            break;
                        case SDL_GAMEPAD_BUTTON_GUIDE:
                            button = GAMEPAD_BUTTON_MIDDLE;
                            break;
                        case SDL_GAMEPAD_BUTTON_START:
                            button = GAMEPAD_BUTTON_MIDDLE_RIGHT;
                            break;

                        case SDL_GAMEPAD_BUTTON_DPAD_UP:
                            button = GAMEPAD_BUTTON_LEFT_FACE_UP;
                            break;
                        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                            button = GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
                            break;
                        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                            button = GAMEPAD_BUTTON_LEFT_FACE_DOWN;
                            break;
                        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                            button = GAMEPAD_BUTTON_LEFT_FACE_LEFT;
                            break;

                        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
                            button = GAMEPAD_BUTTON_LEFT_THUMB;
                            break;
                        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
                            button = GAMEPAD_BUTTON_RIGHT_THUMB;
                            break;
                        default:
                            break;
                    }

                    if (button >= 0) {
                        for (int i = 0; i < MAX_GAMEPADS; i++) {
                            if (platform.gamepadId[i] == event.jbutton.which) {
                                INPUT.Gamepad.currentButtonState[i][button] = 1;
                                INPUT.Gamepad.lastButtonPressed = button;
                                break;
                            }
                        }
                    }
                }
                    break;
                case SDL_EVENT_GAMEPAD_BUTTON_UP: {
                    int button = -1;

                    switch (event.jbutton.button) {
                        case SDL_GAMEPAD_BUTTON_NORTH:
                            button = GAMEPAD_BUTTON_Y;
                            break;
                        case SDL_GAMEPAD_BUTTON_EAST:
                            button = GAMEPAD_BUTTON_B;
                            break;
                        case SDL_GAMEPAD_BUTTON_SOUTH:
                            button = GAMEPAD_BUTTON_A;
                            break;
                        case SDL_GAMEPAD_BUTTON_WEST:
                            button = GAMEPAD_BUTTON_X;
                            break;

                        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                            button = GAMEPAD_BUTTON_LEFT_TRIGGER_1;
                            break;
                        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                            button = GAMEPAD_BUTTON_RIGHT_TRIGGER_1;
                            break;

                        case SDL_GAMEPAD_BUTTON_BACK:
                            button = GAMEPAD_BUTTON_MIDDLE_LEFT;
                            break;
                        case SDL_GAMEPAD_BUTTON_GUIDE:
                            button = GAMEPAD_BUTTON_MIDDLE;
                            break;
                        case SDL_GAMEPAD_BUTTON_START:
                            button = GAMEPAD_BUTTON_MIDDLE_RIGHT;
                            break;

                        case SDL_GAMEPAD_BUTTON_DPAD_UP:
                            button = GAMEPAD_BUTTON_LEFT_FACE_UP;
                            break;
                        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                            button = GAMEPAD_BUTTON_LEFT_FACE_RIGHT;
                            break;
                        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                            button = GAMEPAD_BUTTON_LEFT_FACE_DOWN;
                            break;
                        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                            button = GAMEPAD_BUTTON_LEFT_FACE_LEFT;
                            break;

                        case SDL_GAMEPAD_BUTTON_LEFT_STICK:
                            button = GAMEPAD_BUTTON_LEFT_THUMB;
                            break;
                        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
                            button = GAMEPAD_BUTTON_RIGHT_THUMB;
                            break;
                        default:
                            break;
                    }

                    if (button >= 0) {
                        for (int i = 0; i < MAX_GAMEPADS; i++) {
                            if (platform.gamepadId[i] == event.jbutton.which) {
                                INPUT.Gamepad.currentButtonState[i][button] = 0;
                                if (INPUT.Gamepad.lastButtonPressed == button) INPUT.Gamepad.lastButtonPressed = 0;
                                break;
                            }
                        }
                    }
                }
                    break;
                case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
                    int axis = -1;

                    switch (event.jaxis.axis) {
                        case SDL_GAMEPAD_AXIS_LEFTX:
                            axis = GAMEPAD_AXIS_LEFT_X;
                            break;
                        case SDL_GAMEPAD_AXIS_LEFTY:
                            axis = GAMEPAD_AXIS_LEFT_Y;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHTX:
                            axis = GAMEPAD_AXIS_RIGHT_X;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHTY:
                            axis = GAMEPAD_AXIS_RIGHT_Y;
                            break;
                        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:
                            axis = GAMEPAD_AXIS_LEFT_TRIGGER;
                            break;
                        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:
                            axis = GAMEPAD_AXIS_RIGHT_TRIGGER;
                            break;
                        default:
                            break;
                    }

                    if (axis >= 0) {
                        for (int i = 0; i < MAX_GAMEPADS; i++) {
                            if (platform.gamepadId[i] == event.jaxis.which) {
                                // SDL axis value range is -32768 to 32767, we normalize it to RayLib's -1.0 to 1.0f range
                                float value = event.jaxis.value / (float) 32767;
                                INPUT.Gamepad.axisState[i][axis] = value;

                                // Register button state for triggers in addition to their axes
                                if ((axis == GAMEPAD_AXIS_LEFT_TRIGGER) || (axis == GAMEPAD_AXIS_RIGHT_TRIGGER)) {
                                    int button = (axis == GAMEPAD_AXIS_LEFT_TRIGGER) ? GAMEPAD_BUTTON_LEFT_TRIGGER_2
                                                                                     : GAMEPAD_BUTTON_RIGHT_TRIGGER_2;
                                    int pressed = (value > 0.1f);
                                    INPUT.Gamepad.currentButtonState[i][button] = pressed;
                                    if (pressed) INPUT.Gamepad.lastButtonPressed = button;
                                    else if (INPUT.Gamepad.lastButtonPressed == button)
                                        INPUT.Gamepad.lastButtonPressed = 0;
                                }
                                break;
                            }
                        }
                    }
                }
                    break;
                default:
                    break;
            }

#if defined(SUPPORT_GESTURES_SYSTEM)
            if (touchAction > -1)
            {
                // Process mouse events as touches to be able to use mouse-gestures
                GestureEvent gestureEvent = { 0 };

                // Register touch actions
                gestureEvent.touchAction = touchAction;

                // Assign a pointer ID
                gestureEvent.pointId[0] = 0;

                // Register touch points count
                gestureEvent.pointCount = 1;

                // Register touch points position, only one point registered
                if (touchAction == 2 || realTouch) gestureEvent.position[0] = INPUT.Touch.position[0];
                else gestureEvent.position[0] = GetMousePosition();

                // Normalize gestureEvent.position[0] for CORE.Window.screen.width and CORE.Window.screen.height
                gestureEvent.position[0].x /= (float)GetScreenWidth();
                gestureEvent.position[0].y /= (float)GetScreenHeight();

                // Gesture data is sent to gestures-system for processing
                ProcessGestureEvent(gestureEvent);

                touchAction = -1;
            }
#endif
        }
        //-----------------------------------------------------------------------------
    }

//----------------------------------------------------------------------------------
// Module Internal Functions Definition
//----------------------------------------------------------------------------------

// Initialize platform: graphics, inputs and more
    int InitPlatform()
    {
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

        SDL_InitFlags flags = SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC | SDL_INIT_VIDEO;

#include <spdlog/spdlog.h>

        int result = SDL_Init(flags);
        if (!result) {
            SPDLOG_ERROR("Failed to initialize SDL: {}", SDL_GetError());
            std::exit(EXIT_FAILURE);
        }

        SDL_PropertiesID winprops = SDL_CreateProperties();
        if(winprops == 0)
        {
            SPDLOG_ERROR("Unable to create SDL properties: {}.", SDL_GetError());
            std::exit(EXIT_FAILURE);
        }

        SDL_SetBooleanProperty(winprops, SDL_PROP_WINDOW_CREATE_HIDDEN_BOOLEAN, true);

        window = SDL_CreateWindowWithProperties(winprops);
        if(window == NULL)
        {
            SPDLOG_ERROR("Unable to create window: {}.", SDL_GetError());
            std::exit(EXIT_FAILURE);
        }

        SetGamepadMappings();

        // Initialize input events system
        //----------------------------------------------------------------------------
        // Initialize gamepads
        for (int i = 0; i < MAX_GAMEPADS; i++) {
            platform.gamepadId[i] = -1; // Set all gamepad initial instance ids as invalid to not conflict with instance id zero
        }

        SDL_JoystickID *joysticks = SDL_GetGamepads(&numJoysticks);

        for (int i = 0; (i < numJoysticks) && (i < MAX_GAMEPADS); i++) {
            platform.gamepad[i] = SDL_OpenGamepad(joysticks[i]);
            platform.gamepadId[i] = joysticks[i];

            if (platform.gamepad[i]) {
                INPUT.Gamepad.ready[i] = true;
                INPUT.Gamepad.axisCount[i] = SDL_GetNumJoystickAxes(SDL_GetGamepadJoystick(platform.gamepad[i]));
                INPUT.Gamepad.axisState[i][GAMEPAD_AXIS_LEFT_TRIGGER] = -1.0f;
                INPUT.Gamepad.axisState[i][GAMEPAD_AXIS_RIGHT_TRIGGER] = -1.0f;
                strncpy(INPUT.Gamepad.name[i].data(), SDL_GetGamepadNameForID(joysticks[i]), MAX_GAMEPAD_NAME_LENGTH - 1);
                INPUT.Gamepad.name[i][MAX_GAMEPAD_NAME_LENGTH - 1] = '\0';
            } else {
                SPDLOG_ERROR("PLATFORM: Unable to open game controller [ERROR: %s]", SDL_GetError());
            }
        }

        // Disable mouse events being interpreted as touch events
        // NOTE: This is wanted because there are SDL_FINGER* events available which provide unique data
        //       Due to the way PollInputEvents() and rgestures.h are currently implemented, setting this won't break SUPPORT_MOUSE_GESTURES
        SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS, "0");

        // free the joysticks array
        SDL_free(joysticks);

        return 0;
    }

// Close platform
    void ClosePlatform()
    {
        SDL_DestroyWindow(window);
        SDL_Quit(); // Deinitialize SDL internal global state
    }

// Scancode to keycode mapping
    KeyboardKey ConvertScancodeToKey(SDL_Scancode sdlScancode)
    {
        if ((sdlScancode >= 0) && (sdlScancode < SCANCODE_MAPPED_NUM)) {
            return mapScancodeToKey[sdlScancode];
        }

        return KEY_NULL; // No equivalent key in Raylib
    }
} // namespace SDL_INPUT
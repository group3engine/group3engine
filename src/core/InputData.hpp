#ifndef GROUP3ENGINE_INPUTDATA_HPP
#define GROUP3ENGINE_INPUTDATA_HPP

#include <array>
#include <cstdint>

// From GLFW
enum class KEY {
    _SPACE              = 32,
    _APOSTROPHE         = 39,  /* ' */
    _COMMA              = 44,  /* , */
    _MINUS              = 45,  /* - */
    _PERIOD             = 46,  /* . */
    _SLASH              = 47,  /* / */
    _0                  = 48,
    _1                  = 49,
    _2                  = 50,
    _3                  = 51,
    _4                  = 52,
    _5                  = 53,
    _6                  = 54,
    _7                  = 55,
    _8                  = 56,
    _9                  = 57,
    _SEMICOLON          = 59,  /* ; */
    _EQUAL              = 61,  /* = */
    _A                  = 65,
    _B                  = 66,
    _C                  = 67,
    _D                  = 68,
    _E                  = 69,
    _F                  = 70,
    _G                  = 71,
    _H                  = 72,
    _I                  = 73,
    _J                  = 74,
    _K                  = 75,
    _L                  = 76,
    _M                  = 77,
    _N                  = 78,
    _O                  = 79,
    _P                  = 80,
    _Q                  = 81,
    _R                  = 82,
    _S                  = 83,
    _T                  = 84,
    _U                  = 85,
    _V                  = 86,
    _W                  = 87,
    _X                  = 88,
    _Y                  = 89,
    _Z                  = 90,
    _LEFT_BRACKET       = 91,  /* [ */
    _BACKSLASH          = 92,  /* \ */
    _RIGHT_BRACKET      = 93,  /* ] */
    _GRAVE_ACCENT       = 96,  /* ` */
    _WORLD_1            = 161, /* non-US #1 */
    _WORLD_2            = 162, /* non-US #2 */

    /* Function keys */
    _ESCAPE             = 256,
    _ENTER              = 257,
    _TAB                = 258,
    _BACKSPACE          = 259,
    _INSERT             = 260,
    _DELETE             = 261,
    _RIGHT              = 262,
    _LEFT               = 263,
    _DOWN               = 264,
    _UP                 = 265,
    _PAGE_UP            = 266,
    _PAGE_DOWN          = 267,
    _HOME               = 268,
    _END                = 269,
    _CAPS_LOCK          = 280,
    _SCROLL_LOCK        = 281,
    _NUM_LOCK           = 282,
    _PRINT_SCREEN       = 283,
    _PAUSE              = 284,
    _F1                 = 290,
    _F2                 = 291,
    _F3                 = 292,
    _F4                 = 293,
    _F5                 = 294,
    _F6                 = 295,
    _F7                 = 296,
    _F8                 = 297,
    _F9                 = 298,
    _F10                = 299,
    _F11                = 300,
    _F12                = 301,
    _F13                = 302,
    _F14                = 303,
    _F15                = 304,
    _F16                = 305,
    _F17                = 306,
    _F18                = 307,
    _F19                = 308,
    _F20                = 309,
    _F21                = 310,
    _F22                = 311,
    _F23                = 312,
    _F24                = 313,
    _F25                = 314,
    _KP_0               = 320,
    _KP_1               = 321,
    _KP_2               = 322,
    _KP_3               = 323,
    _KP_4               = 324,
    _KP_5               = 325,
    _KP_6               = 326,
    _KP_7               = 327,
    _KP_8               = 328,
    _KP_9               = 329,
    _KP_DECIMAL         = 330,
    _KP_DIVIDE          = 331,
    _KP_MULTIPLY        = 332,
    _KP_SUBTRACT        = 333,
    _KP_ADD             = 334,
    _KP_ENTER           = 335,
    _KP_EQUAL           = 336,
    _LEFT_SHIFT         = 340,
    _LEFT_CONTROL       = 341,
    _LEFT_ALT           = 342,
    _LEFT_SUPER         = 343,
    _RIGHT_SHIFT        = 344,
    _RIGHT_CONTROL      = 345,
    _RIGHT_ALT          = 346,
    _RIGHT_SUPER        = 347,
    _MENU               = 348,

    _LAST               = 512
};

struct InputData {
    struct {
        std::array<uint8_t, static_cast<uint16_t>(KEY::_LAST)> currentKeyState{};
        std::array<uint8_t, static_cast<uint16_t>(KEY::_LAST)> previousKeyState{};
    } keyboard;
};
#endif // GROUP3ENGINE_INPUTDATA_HPP

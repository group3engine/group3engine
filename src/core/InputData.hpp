/* Code in this file is based on code from the raylib programming library. */

/**********************************************************************************************
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

#ifndef GROUP3ENGINE_INPUTDATA_HPP
#define GROUP3ENGINE_INPUTDATA_HPP

#include <array>
#include <cstdint>

#include <glm/vec2.hpp>

// From GLFW
enum class KEY {
    eSPACE              = 32,
    eAPOSTROPHE         = 39,  /* ' */
    eCOMMA              = 44,  /* , */
    eMINUS              = 45,  /* - */
    ePERIOD             = 46,  /* . */
    eSLASH              = 47,  /* / */
    e0                  = 48,
    e1                  = 49,
    e2                  = 50,
    e3                  = 51,
    e4                  = 52,
    e5                  = 53,
    e6                  = 54,
    e7                  = 55,
    e8                  = 56,
    e9                  = 57,
    eSEMICOLON          = 59,  /* ; */
    eEQUAL              = 61,  /* = */
    eA                  = 65,
    eB                  = 66,
    eC                  = 67,
    eD                  = 68,
    eE                  = 69,
    eF                  = 70,
    eG                  = 71,
    eH                  = 72,
    eI                  = 73,
    eJ                  = 74,
    eK                  = 75,
    eL                  = 76,
    eM                  = 77,
    eN                  = 78,
    eO                  = 79,
    eP                  = 80,
    eQ                  = 81,
    eR                  = 82,
    eS                  = 83,
    eT                  = 84,
    eU                  = 85,
    eV                  = 86,
    eW                  = 87,
    eX                  = 88,
    eY                  = 89,
    eZ                  = 90,
    eLEFT_BRACKET       = 91,  /* [ */
    eBACKSLASH          = 92,  /* \ */
    eRIGHT_BRACKET      = 93,  /* ] */
    eGRAVE_ACCENT       = 96,  /* ` */
    eWORLD_1            = 161, /* non-US #1 */
    eWORLD_2            = 162, /* non-US #2 */

    /* Function keys */
    eESCAPE             = 256,
    eENTER              = 257,
    eTAB                = 258,
    eBACKSPACE          = 259,
    eINSERT             = 260,
    eDELETE             = 261,
    eRIGHT              = 262,
    eLEFT               = 263,
    eDOWN               = 264,
    eUP                 = 265,
    ePAGE_UP            = 266,
    ePAGE_DOWN          = 267,
    eHOME               = 268,
    eEND                = 269,
    eCAPS_LOCK          = 280,
    eSCROLL_LOCK        = 281,
    eNUM_LOCK           = 282,
    ePRINT_SCREEN       = 283,
    ePAUSE              = 284,
    eF1                 = 290,
    eF2                 = 291,
    eF3                 = 292,
    eF4                 = 293,
    eF5                 = 294,
    eF6                 = 295,
    eF7                 = 296,
    eF8                 = 297,
    eF9                 = 298,
    eF10                = 299,
    eF11                = 300,
    eF12                = 301,
    eF13                = 302,
    eF14                = 303,
    eF15                = 304,
    eF16                = 305,
    eF17                = 306,
    eF18                = 307,
    eF19                = 308,
    eF20                = 309,
    eF21                = 310,
    eF22                = 311,
    eF23                = 312,
    eF24                = 313,
    eF25                = 314,
    eKP_0               = 320,
    eKP_1               = 321,
    eKP_2               = 322,
    eKP_3               = 323,
    eKP_4               = 324,
    eKP_5               = 325,
    eKP_6               = 326,
    eKP_7               = 327,
    eKP_8               = 328,
    eKP_9               = 329,
    eKP_DECIMAL         = 330,
    eKP_DIVIDE          = 331,
    eKP_MULTIPLY        = 332,
    eKP_SUBTRACT        = 333,
    eKP_ADD             = 334,
    eKP_ENTER           = 335,
    eKP_EQUAL           = 336,
    eLEFT_SHIFT         = 340,
    eLEFT_CONTROL       = 341,
    eLEFT_ALT           = 342,
    eLEFT_SUPER         = 343,
    eRIGHT_SHIFT        = 344,
    eRIGHT_CONTROL      = 345,
    eRIGHT_ALT          = 346,
    eRIGHT_SUPER        = 347,
    eMENU               = 348,

    eLAST               = 512
};

enum class MOUSE_BUTTON {
    eLEFT               = 0,
    eRIGHT              = 1,
    eMIDDLE             = 2,
    e4                  = 3,
    e5                  = 4,
    e6                  = 5,
    e7                  = 6,
    e8                  = 7,
    eLAST               = 8
};

struct InputData {
    struct {
        std::array<uint8_t, static_cast<uint16_t>(KEY::eLAST)> currentKeyState{};
        std::array<uint8_t, static_cast<uint16_t>(KEY::eLAST)> previousKeyState{};
    } keyboard;

    struct {
        glm::vec2 currentPosition{};
        glm::vec2 previousPosition{};

        std::array<uint8_t, static_cast<uint8_t>(MOUSE_BUTTON::eLAST)> currentButtonState{};
        std::array<uint8_t, static_cast<uint8_t>(MOUSE_BUTTON::eLAST)> previousButtonState{};
    } mouse;
};
#endif // GROUP3ENGINE_INPUTDATA_HPP

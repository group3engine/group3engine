//
// Created by thomas on 09/04/25.
//

#ifndef GROUP3ENGINE_FONTS_HPP
#define GROUP3ENGINE_FONTS_HPP

// C++
#include <imgui.h>
#include <iostream>

namespace Fonts {
    inline ImFont* HeadingFont = nullptr;
    inline ImFont* SubHeadingFont = nullptr;
    inline ImFont* TextFont = nullptr;
    inline ImFont* TextFontSubtle = nullptr;
    inline ImFont* TextFontSmall = nullptr;

    bool LoadFonts() ;
}


#endif //GROUP3ENGINE_FONTS_HPP

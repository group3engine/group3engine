//
// shamelessly stolen from https://github.com/shivang51/bess/blob/main/Bess/include/settings/themes.h
//

#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

class ViewportTheme {
public:
    static glm::vec4 componentBGColor;
    static glm::vec4 selectedCompColor;
    static glm::vec4 componentBorderColor;
    static glm::vec4 backgroundColor;
    static glm::vec4 stateHighColor;
    static glm::vec4 wireColor;
    static glm::vec4 selectedWireColor;
    static glm::vec4 compHeaderColor;
    static glm::vec4 textColor;
    static glm::vec4 gridColor;
    static glm::vec4 selectionBoxBorderColor;
    static glm::vec4 selectionBoxFillColor;
    static glm::vec4 stateLowColor;

    static void updateColorsFromImGuiStyle();
};

class Themes {
public:
    Themes();

    void applyTheme(const std::string &theme);

    void addTheme(const std::string &name, const std::function<void()> &callback);

    const std::unordered_map<std::string, std::function<void()>> &getThemes() const;

private:
    static void setModernColors();

    static void setMaterialYouColors();

    static void setDarkThemeColors();

    static void setBessDarkThemeColors();

    static void setCatpuccinMochaColors();

    static void setGlassTheme();

    static void setFluentUITheme();

    static void setFluentUILightTheme();

    static void setNewDarkTheme();

    // theme name and a void callback
    std::unordered_map<std::string, std::function<void()>> m_themes = {};
};

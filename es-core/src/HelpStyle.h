//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HelpStyle.h
//
//  Style (default colors, position and origin) for the help system.
//  Also theme handling.
//

#ifndef ES_CORE_HELP_STYLE_H
#define ES_CORE_HELP_STYLE_H

#include "utils/MathUtil.h"

#include <memory>
#include <string>

class Font;
class ThemeData;

struct HelpStyle {
    glm::vec2 position;
    glm::vec2 origin;
    unsigned int textColor;
    unsigned int textColorDimmed;
    unsigned int iconColor;
    unsigned int iconColorDimmed;
    std::shared_ptr<Font> font;
    float entrySpacing;
    float iconTextSpacing;
    std::string textStyle;

    struct CustomButtonIcons {

        // General.
        std::string dpad_updown;
        std::string dpad_leftright;
        std::string dpad_all;
        std::string thumbstick_click;
        std::string button_l;
        std::string button_r;
        std::string button_lr;

        // SNES.
        std::string button_a_SNES;
        std::string button_b_SNES;
        std::string button_x_SNES;
        std::string button_y_SNES;
        std::string button_start_SNES;
        std::string button_back_SNES;

        // PS.
        std::string button_a_PS;
        std::string button_b_PS;
        std::string button_x_PS;
        std::string button_y_PS;
        std::string button_start_PS4;
        std::string button_back_PS4;
        std::string button_start_PS5;
        std::string button_back_PS5;

        // XBOX.
        std::string button_a_XBOX;
        std::string button_b_XBOX;
        std::string button_x_XBOX;
        std::string button_y_XBOX;
        std::string button_start_XBOX;
        std::string button_back_XBOX;
        std::string button_start_XBOX360;
        std::string button_back_XBOX360;
    };

    CustomButtonIcons mCustomButtons;

    HelpStyle(); // Default values.
    void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);
};

#endif // ES_CORE_HELP_STYLE_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HelpStyle.h
//
//  Style (colors, position, icons etc.) for the help system.
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
    float opacity;
    bool legacyTheme;
    std::string letterCase;

    struct CustomButtonIcons {
        // Generic
        std::string dpad_updown;
        std::string dpad_leftright;
        std::string dpad_all;
        std::string thumbstick_click;
        std::string button_l;
        std::string button_r;
        std::string button_lr;
        std::string button_lt;
        std::string button_rt;
        std::string button_ltrt;

        // SNES
        std::string button_a_SNES;
        std::string button_b_SNES;
        std::string button_x_SNES;
        std::string button_y_SNES;
        std::string button_back_SNES;
        std::string button_start_SNES;

        // Switch Pro
        std::string button_a_switch;
        std::string button_b_switch;
        std::string button_x_switch;
        std::string button_y_switch;
        std::string button_back_switch;
        std::string button_start_switch;

        // PlayStation
        std::string button_a_PS;
        std::string button_b_PS;
        std::string button_x_PS;
        std::string button_y_PS;
        std::string button_back_PS123;
        std::string button_start_PS123;
        std::string button_back_PS4;
        std::string button_start_PS4;
        std::string button_back_PS5;
        std::string button_start_PS5;

        // XBOX
        std::string button_a_XBOX;
        std::string button_b_XBOX;
        std::string button_x_XBOX;
        std::string button_y_XBOX;
        std::string button_back_XBOX;
        std::string button_start_XBOX;
        std::string button_back_XBOX360;
        std::string button_start_XBOX360;
    };

    CustomButtonIcons mCustomButtons;

    HelpStyle();
    void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);
};

#endif // ES_CORE_HELP_STYLE_H

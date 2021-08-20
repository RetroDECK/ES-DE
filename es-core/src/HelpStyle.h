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
    unsigned int iconColor;
    unsigned int textColor;
    std::shared_ptr<Font> font;
    float entrySpacing;
    float iconTextSpacing;
    std::string textStyle;

    HelpStyle(); // Default values.
    void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);
};

#endif // ES_CORE_HELP_STYLE_H

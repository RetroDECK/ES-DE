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

#include "math/Vector2f.h"

#include <memory>
#include <string>

class Font;
class ThemeData;

struct HelpStyle {
    Vector2f position;
    Vector2f origin;
    unsigned int iconColor;
    unsigned int textColor;
    std::shared_ptr<Font> font;

    HelpStyle(); // Default values.
    void applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view);
};

#endif // ES_CORE_HELP_STYLE_H

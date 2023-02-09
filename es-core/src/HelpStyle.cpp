//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HelpStyle.cpp
//
//  Style (colors, position, icons etc.) for the help system.
//

#include "HelpStyle.h"

#include "resources/Font.h"

#define PREFIX "button_"

HelpStyle::HelpStyle()
    : position {Renderer::getScreenWidth() * 0.012f,
                Renderer::getScreenHeight() *
                    (Renderer::getIsVerticalOrientation() ? 0.975f : 0.9515f)}
    , origin {glm::vec2 {0.0f, 0.0f}}
    , textColor {0x777777FF}
    , textColorDimmed {0x777777FF}
    , iconColor {0x777777FF}
    , iconColorDimmed {0x777777FF}
    , font {Renderer::getIsVerticalOrientation() ? Font::get(0.025f * Renderer::getScreenWidth()) :
                                                   Font::get(FONT_SIZE_SMALL)}
    , entrySpacing {0.00833f}
    , iconTextSpacing {0.00416f}
    , opacity {1.0f}
    , legacyTheme {false}
    , letterCase {"uppercase"}
{
}

void HelpStyle::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
    auto elem = theme->getElement(view, "helpsystem_help", "helpsystem");
    if (!elem)
        return;

    legacyTheme = theme->isLegacyTheme();

    if (elem->has("pos"))
        position = elem->get<glm::vec2>("pos") *
                   glm::vec2 {Renderer::getScreenWidth(), Renderer::getScreenHeight()};

    if (elem->has("origin"))
        origin = elem->get<glm::vec2>("origin");

    if (elem->has("textColor"))
        textColor = elem->get<unsigned int>("textColor");

    if (elem->has("textColorDimmed"))
        textColorDimmed = elem->get<unsigned int>("textColorDimmed");
    else
        textColorDimmed = textColor;

    if (elem->has("iconColor"))
        iconColor = elem->get<unsigned int>("iconColor");

    if (elem->has("iconColorDimmed"))
        iconColorDimmed = elem->get<unsigned int>("iconColorDimmed");
    else
        iconColorDimmed = iconColor;

    if (elem->has("fontPath") || elem->has("fontSize"))
        font = Font::getFromTheme(elem, ThemeFlags::ALL, font, 0.0f, false, theme->isLegacyTheme());

    if (elem->has("entrySpacing"))
        entrySpacing = glm::clamp(elem->get<float>("entrySpacing"), 0.0f, 0.04f);

    if (elem->has("iconTextSpacing"))
        iconTextSpacing = glm::clamp(elem->get<float>("iconTextSpacing"), 0.0f, 0.04f);

    if (elem->has("letterCase"))
        letterCase = elem->get<std::string>("letterCase");

    if (elem->has("opacity"))
        opacity = glm::clamp(elem->get<float>("opacity"), 0.2f, 1.0f);

    // Load custom button icons.
    // The names may look a bit strange when combined with the PREFIX string "button_" but it's
    // because ThemeData adds this prefix to avoid name collisions when using XML attributes.

    // General.
    if (elem->has(PREFIX "dpad_updown"))
        mCustomButtons.dpad_updown = elem->get<std::string>(PREFIX "dpad_updown");
    if (elem->has(PREFIX "dpad_leftright"))
        mCustomButtons.dpad_leftright = elem->get<std::string>(PREFIX "dpad_leftright");
    if (elem->has(PREFIX "dpad_all"))
        mCustomButtons.dpad_all = elem->get<std::string>(PREFIX "dpad_all");
    if (elem->has(PREFIX "thumbstick_click"))
        mCustomButtons.thumbstick_click = elem->get<std::string>(PREFIX "thumbstick_click");
    if (elem->has(PREFIX "button_l"))
        mCustomButtons.button_l = elem->get<std::string>(PREFIX "button_l");
    if (elem->has(PREFIX "button_r"))
        mCustomButtons.button_r = elem->get<std::string>(PREFIX "button_r");
    if (elem->has(PREFIX "button_lr"))
        mCustomButtons.button_lr = elem->get<std::string>(PREFIX "button_lr");
    if (elem->has(PREFIX "button_lt"))
        mCustomButtons.button_lt = elem->get<std::string>(PREFIX "button_lt");
    if (elem->has(PREFIX "button_rt"))
        mCustomButtons.button_rt = elem->get<std::string>(PREFIX "button_rt");
    if (elem->has(PREFIX "button_ltrt"))
        mCustomButtons.button_ltrt = elem->get<std::string>(PREFIX "button_ltrt");

    // SNES.
    if (elem->has(PREFIX "button_a_SNES"))
        mCustomButtons.button_a_SNES = elem->get<std::string>(PREFIX "button_a_SNES");
    if (elem->has(PREFIX "button_b_SNES"))
        mCustomButtons.button_b_SNES = elem->get<std::string>(PREFIX "button_b_SNES");
    if (elem->has(PREFIX "button_x_SNES"))
        mCustomButtons.button_x_SNES = elem->get<std::string>(PREFIX "button_x_SNES");
    if (elem->has(PREFIX "button_y_SNES"))
        mCustomButtons.button_y_SNES = elem->get<std::string>(PREFIX "button_y_SNES");
    if (elem->has(PREFIX "button_back_SNES"))
        mCustomButtons.button_back_SNES = elem->get<std::string>(PREFIX "button_back_SNES");
    if (elem->has(PREFIX "button_start_SNES"))
        mCustomButtons.button_start_SNES = elem->get<std::string>(PREFIX "button_start_SNES");

    // Switch Pro.
    if (elem->has(PREFIX "button_a_switch"))
        mCustomButtons.button_a_switch = elem->get<std::string>(PREFIX "button_a_switch");
    if (elem->has(PREFIX "button_b_switch"))
        mCustomButtons.button_b_switch = elem->get<std::string>(PREFIX "button_b_switch");
    if (elem->has(PREFIX "button_x_switch"))
        mCustomButtons.button_x_switch = elem->get<std::string>(PREFIX "button_x_switch");
    if (elem->has(PREFIX "button_y_switch"))
        mCustomButtons.button_y_switch = elem->get<std::string>(PREFIX "button_y_switch");
    if (elem->has(PREFIX "button_back_switch"))
        mCustomButtons.button_back_switch = elem->get<std::string>(PREFIX "button_back_switch");
    if (elem->has(PREFIX "button_start_switch"))
        mCustomButtons.button_start_switch = elem->get<std::string>(PREFIX "button_start_switch");

    // PlayStation.
    if (elem->has(PREFIX "button_a_PS"))
        mCustomButtons.button_a_PS = elem->get<std::string>(PREFIX "button_a_PS");
    if (elem->has(PREFIX "button_b_PS"))
        mCustomButtons.button_b_PS = elem->get<std::string>(PREFIX "button_b_PS");
    if (elem->has(PREFIX "button_x_PS"))
        mCustomButtons.button_x_PS = elem->get<std::string>(PREFIX "button_x_PS");
    if (elem->has(PREFIX "button_y_PS"))
        mCustomButtons.button_y_PS = elem->get<std::string>(PREFIX "button_y_PS");
    if (elem->has(PREFIX "button_back_PS123"))
        mCustomButtons.button_back_PS123 = elem->get<std::string>(PREFIX "button_back_PS123");
    if (elem->has(PREFIX "button_start_PS123"))
        mCustomButtons.button_start_PS123 = elem->get<std::string>(PREFIX "button_start_PS123");
    if (elem->has(PREFIX "button_back_PS4"))
        mCustomButtons.button_back_PS4 = elem->get<std::string>(PREFIX "button_back_PS4");
    if (elem->has(PREFIX "button_start_PS4"))
        mCustomButtons.button_start_PS4 = elem->get<std::string>(PREFIX "button_start_PS4");
    if (elem->has(PREFIX "button_back_PS5"))
        mCustomButtons.button_back_PS5 = elem->get<std::string>(PREFIX "button_back_PS5");
    if (elem->has(PREFIX "button_start_PS5"))
        mCustomButtons.button_start_PS5 = elem->get<std::string>(PREFIX "button_start_PS5");

    // XBOX.
    if (elem->has(PREFIX "button_a_XBOX"))
        mCustomButtons.button_a_XBOX = elem->get<std::string>(PREFIX "button_a_XBOX");
    if (elem->has(PREFIX "button_b_XBOX"))
        mCustomButtons.button_b_XBOX = elem->get<std::string>(PREFIX "button_b_XBOX");
    if (elem->has(PREFIX "button_x_XBOX"))
        mCustomButtons.button_x_XBOX = elem->get<std::string>(PREFIX "button_x_XBOX");
    if (elem->has(PREFIX "button_y_XBOX"))
        mCustomButtons.button_y_XBOX = elem->get<std::string>(PREFIX "button_y_XBOX");
    if (elem->has(PREFIX "button_back_XBOX"))
        mCustomButtons.button_back_XBOX = elem->get<std::string>(PREFIX "button_back_XBOX");
    if (elem->has(PREFIX "button_start_XBOX"))
        mCustomButtons.button_start_XBOX = elem->get<std::string>(PREFIX "button_start_XBOX");
    if (elem->has(PREFIX "button_back_XBOX360"))
        mCustomButtons.button_back_XBOX360 = elem->get<std::string>(PREFIX "button_back_XBOX360");
    if (elem->has(PREFIX "button_start_XBOX360"))
        mCustomButtons.button_start_XBOX360 = elem->get<std::string>(PREFIX "button_start_XBOX360");
}

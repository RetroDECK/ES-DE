//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HelpStyle.cpp
//
//  Style (default colors, position and origin) for the help system.
//  Also theme handling.
//

#include "HelpStyle.h"

#include "resources/Font.h"

HelpStyle::HelpStyle()
{
    position =
        glm::vec2 {Renderer::getScreenWidth() * 0.012f, Renderer::getScreenHeight() * 0.9515f};
    origin = glm::vec2 {};
    textColor = 0x777777FF;
    textColorDimmed = 0x777777FF;
    iconColor = 0x777777FF;
    iconColorDimmed = 0x777777FF;
    entrySpacing = 16.0f;
    iconTextSpacing = 8.0f;
    textStyle = "uppercase";

    if (FONT_SIZE_SMALL != 0)
        font = Font::get(FONT_SIZE_SMALL);
    else
        font = nullptr;
}

void HelpStyle::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view)
{
    auto elem = theme->getElement(view, "help", "helpsystem");
    if (!elem)
        return;

    if (elem->has("pos"))
        position = elem->get<glm::vec2>("pos") *
                   glm::vec2 {static_cast<float>(Renderer::getScreenWidth()),
                              static_cast<float>(Renderer::getScreenHeight())};

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
        font = Font::getFromTheme(elem, ThemeFlags::ALL, font);

    if (elem->has("entrySpacing"))
        entrySpacing = elem->get<float>("entrySpacing");

    if (elem->has("iconTextSpacing"))
        iconTextSpacing = elem->get<float>("iconTextSpacing");

    if (elem->has("textStyle"))
        textStyle = elem->get<std::string>("textStyle");

    // Load custom button icons.

    // General.
    if (elem->has("dpad_updown"))
        mCustomButtons.dpad_updown = elem->get<std::string>("dpad_updown");
    if (elem->has("dpad_leftright"))
        mCustomButtons.dpad_leftright = elem->get<std::string>("dpad_leftright");
    if (elem->has("dpad_all"))
        mCustomButtons.dpad_all = elem->get<std::string>("dpad_all");
    if (elem->has("thumbstick_click"))
        mCustomButtons.thumbstick_click = elem->get<std::string>("thumbstick_click");
    if (elem->has("button_l"))
        mCustomButtons.button_l = elem->get<std::string>("button_l");
    if (elem->has("button_r"))
        mCustomButtons.button_r = elem->get<std::string>("button_r");
    if (elem->has("button_lr"))
        mCustomButtons.button_lr = elem->get<std::string>("button_lr");
    if (elem->has("button_lt"))
        mCustomButtons.button_lt = elem->get<std::string>("button_lt");
    if (elem->has("button_rt"))
        mCustomButtons.button_rt = elem->get<std::string>("button_rt");

    // SNES.
    if (elem->has("button_a_SNES"))
        mCustomButtons.button_a_SNES = elem->get<std::string>("button_a_SNES");
    if (elem->has("button_b_SNES"))
        mCustomButtons.button_b_SNES = elem->get<std::string>("button_b_SNES");
    if (elem->has("button_x_SNES"))
        mCustomButtons.button_x_SNES = elem->get<std::string>("button_x_SNES");
    if (elem->has("button_y_SNES"))
        mCustomButtons.button_y_SNES = elem->get<std::string>("button_y_SNES");
    if (elem->has("button_start_SNES"))
        mCustomButtons.button_start_SNES = elem->get<std::string>("button_start_SNES");
    if (elem->has("button_back_SNES"))
        mCustomButtons.button_back_SNES = elem->get<std::string>("button_back_SNES");

    // PS.
    if (elem->has("button_a_PS"))
        mCustomButtons.button_a_PS = elem->get<std::string>("button_a_PS");
    if (elem->has("button_b_PS"))
        mCustomButtons.button_b_PS = elem->get<std::string>("button_b_PS");
    if (elem->has("button_x_PS"))
        mCustomButtons.button_x_PS = elem->get<std::string>("button_x_PS");
    if (elem->has("button_y_PS"))
        mCustomButtons.button_y_PS = elem->get<std::string>("button_y_PS");
    if (elem->has("button_start_PS4"))
        mCustomButtons.button_start_PS4 = elem->get<std::string>("button_start_PS4");
    if (elem->has("button_back_PS4"))
        mCustomButtons.button_back_PS4 = elem->get<std::string>("button_back_PS4");
    if (elem->has("button_start_PS5"))
        mCustomButtons.button_start_PS5 = elem->get<std::string>("button_start_PS5");
    if (elem->has("button_back_PS5"))
        mCustomButtons.button_back_PS5 = elem->get<std::string>("button_back_PS5");

    // XBOX.
    if (elem->has("button_a_XBOX"))
        mCustomButtons.button_a_XBOX = elem->get<std::string>("button_a_XBOX");
    if (elem->has("button_b_XBOX"))
        mCustomButtons.button_b_XBOX = elem->get<std::string>("button_b_XBOX");
    if (elem->has("button_x_XBOX"))
        mCustomButtons.button_x_XBOX = elem->get<std::string>("button_x_XBOX");
    if (elem->has("button_y_XBOX"))
        mCustomButtons.button_y_XBOX = elem->get<std::string>("button_y_XBOX");
    if (elem->has("button_start_XBOX"))
        mCustomButtons.button_start_XBOX = elem->get<std::string>("button_start_XBOX");
    if (elem->has("button_back_XBOX"))
        mCustomButtons.button_back_XBOX = elem->get<std::string>("button_back_XBOX");
    if (elem->has("button_start_XBOX360"))
        mCustomButtons.button_start_XBOX360 = elem->get<std::string>("button_start_XBOX360");
    if (elem->has("button_back_XBOX360"))
        mCustomButtons.button_back_XBOX360 = elem->get<std::string>("button_back_XBOX360");
}

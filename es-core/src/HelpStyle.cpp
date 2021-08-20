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
        glm::vec2{Renderer::getScreenWidth() * 0.012f, Renderer::getScreenHeight() * 0.9515f};
    origin = glm::vec2{};
    iconColor = 0x777777FF;
    textColor = 0x777777FF;
    entrySpacing = 16.0f;
    iconTextSpacing = 8.0f;

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
                   glm::vec2{static_cast<float>(Renderer::getScreenWidth()),
                             static_cast<float>(Renderer::getScreenHeight())};

    if (elem->has("origin"))
        origin = elem->get<glm::vec2>("origin");

    if (elem->has("textColor"))
        textColor = elem->get<unsigned int>("textColor");

    if (elem->has("iconColor"))
        iconColor = elem->get<unsigned int>("iconColor");

    if (elem->has("fontPath") || elem->has("fontSize"))
        font = Font::getFromTheme(elem, ThemeFlags::ALL, font);

    if (elem->has("entrySpacing"))
        entrySpacing = elem->get<float>("entrySpacing");

    if (elem->has("iconTextSpacing"))
        iconTextSpacing = elem->get<float>("iconTextSpacing");
}

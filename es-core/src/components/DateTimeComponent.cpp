//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DateTimeComponent.cpp
//
//  Provides the date and time, in absolute (actual date) or relative
//  (delta from current date and time) form.
//  Used by the gamelist views.
//

#include "components/DateTimeComponent.h"

#include "Log.h"
#include "Settings.h"
#include "utils/StringUtil.h"

DateTimeComponent::DateTimeComponent(Window* window)
    : TextComponent(window)
    , mDisplayRelative(false)
{
    // ISO 8601 date format.
    setFormat("%Y-%m-%d");
}

DateTimeComponent::DateTimeComponent(Window* window,
                                     const std::string& text,
                                     const std::shared_ptr<Font>& font,
                                     unsigned int color,
                                     Alignment align,
                                     glm::vec3 pos,
                                     glm::vec2 size,
                                     unsigned int bgcolor)
    : TextComponent(window, text, font, color, align, pos, size, bgcolor)
    , mDisplayRelative(false)
{
    // ISO 8601 date format.
    setFormat("%Y-%m-%d");
}

void DateTimeComponent::setValue(const std::string& val)
{
    mTime = val;
    onTextChanged();
}

std::string DateTimeComponent::getValue() const
{
    // Return time value as a string.
    return mTime;
}

void DateTimeComponent::setFormat(const std::string& format)
{
    mFormat = format;
    onTextChanged();
}

void DateTimeComponent::setDisplayRelative(bool displayRelative)
{
    mDisplayRelative = displayRelative;
    onTextChanged();
}

void DateTimeComponent::onTextChanged()
{
    mText = getDisplayString();
    TextComponent::onTextChanged();
}

std::string DateTimeComponent::getDisplayString() const
{
    if (mDisplayRelative) {
        // Relative time.
        if (mTime.getTime() == 0)
            return "never";

        Utils::Time::DateTime now(Utils::Time::now());
        Utils::Time::Duration dur(now.getTime() - mTime.getTime());

        std::string buf;

        if (dur.getDays() > 0)
            buf = std::to_string(dur.getDays()) + " day" + // Line break.
                  (dur.getDays() > 1 ? "s" : "") + " ago";
        else if (dur.getHours() > 0)
            buf = std::to_string(dur.getHours()) + " hour" + // Line break.
                  (dur.getHours() > 1 ? "s" : "") + " ago";
        else if (dur.getMinutes() > 0)
            buf = std::to_string(dur.getMinutes()) + " minute" + // Line break.
                  (dur.getMinutes() > 1 ? "s" : "") + " ago";
        else
            buf = std::to_string(dur.getSeconds()) + " second" + // Line break.
                  (dur.getSeconds() > 1 || dur.getSeconds() == 0 ? "s" : "") + " ago";

        return std::string(buf);
    }

    if (mTime.getTime() == 0)
        return "unknown";

    return Utils::Time::timeToString(mTime.getTime(), mFormat);
}

void DateTimeComponent::render(const glm::mat4& parentTrans)
{
    // Render the component.
    TextComponent::render(parentTrans);
}

void DateTimeComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                   const std::string& view,
                                   const std::string& element,
                                   unsigned int properties)
{
    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "datetime");
    if (!elem)
        return;

    if (elem->has("displayRelative"))
        setDisplayRelative(elem->get<bool>("displayRelative"));

    if (elem->has("format"))
        setFormat(elem->get<std::string>("format"));

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    setRenderBackground(false);
    if (properties & COLOR && elem->has("backgroundColor")) {
        setBackgroundColor(elem->get<unsigned int>("backgroundColor"));
        setRenderBackground(true);
    }

    if (properties & ALIGNMENT && elem->has("alignment")) {
        std::string str = elem->get<std::string>("alignment");
        if (str == "left")
            setHorizontalAlignment(ALIGN_LEFT);
        else if (str == "center")
            setHorizontalAlignment(ALIGN_CENTER);
        else if (str == "right")
            setHorizontalAlignment(ALIGN_RIGHT);
        else
            LOG(LogError) << "Unknown text alignment string: " << str;
    }

    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
        setUppercase(elem->get<bool>("forceUppercase"));

    if (properties & LINE_SPACING && elem->has("lineSpacing"))
        setLineSpacing(elem->get<float>("lineSpacing"));

    setFont(Font::getFromTheme(elem, properties, mFont));
}

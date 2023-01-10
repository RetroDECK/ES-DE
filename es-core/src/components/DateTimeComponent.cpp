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

DateTimeComponent::DateTimeComponent()
    : mDisplayRelative {false}
{
    // ISO 8601 date format.
    setFormat("%Y-%m-%d");
}

DateTimeComponent::DateTimeComponent(const std::string& text,
                                     const std::shared_ptr<Font>& font,
                                     unsigned int color,
                                     Alignment horizontalAlignment,
                                     glm::vec3 pos,
                                     glm::vec2 size,
                                     unsigned int bgcolor)
    : TextComponent {text, font, color, horizontalAlignment, ALIGN_CENTER, pos, size, bgcolor}
    , mDisplayRelative {false}
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
        // Workaround to handle Unix epoch for different time zones.
        if (mTime.getTime() < 82800) {
            if (mDefaultValue == "")
                return "never";
            else
                return mDefaultValue;
        }

        Utils::Time::DateTime now {Utils::Time::now()};
        Utils::Time::Duration dur {now.getTime() - mTime.getTime()};

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

    if (mTime.getTime() == 0) {
        if (mDefaultValue == "")
            return "unknown";
        else
            return mDefaultValue;
    }

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

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "datetime")};
    if (!elem)
        return;

    if (elem->has("format"))
        setFormat(elem->get<std::string>("format"));

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    setRenderBackground(false);
    if (properties & COLOR && elem->has("backgroundColor")) {
        setBackgroundColor(elem->get<unsigned int>("backgroundColor"));
        setRenderBackground(true);
    }

    if (properties & ALIGNMENT && elem->has("horizontalAlignment")) {
        const std::string& horizontalAlignment {elem->get<std::string>("horizontalAlignment")};
        if (horizontalAlignment == "left")
            setHorizontalAlignment(ALIGN_LEFT);
        else if (horizontalAlignment == "center")
            setHorizontalAlignment(ALIGN_CENTER);
        else if (horizontalAlignment == "right")
            setHorizontalAlignment(ALIGN_RIGHT);
        else
            LOG(LogWarning) << "DateTimeComponent: Invalid theme configuration, property "
                               "<horizontalAlignment> defined as \""
                            << horizontalAlignment << "\"";
    }

    if (properties & ALIGNMENT && elem->has("verticalAlignment")) {
        const std::string& verticalAlignment {elem->get<std::string>("verticalAlignment")};
        if (verticalAlignment == "top")
            setVerticalAlignment(ALIGN_TOP);
        else if (verticalAlignment == "center")
            setVerticalAlignment(ALIGN_CENTER);
        else if (verticalAlignment == "bottom")
            setVerticalAlignment(ALIGN_BOTTOM);
        else
            LOG(LogWarning) << "DateTimeComponent: Invalid theme configuration, property "
                               "<verticalAlignment> defined as \""
                            << verticalAlignment << "\"";
    }

    // Legacy themes only.
    if (properties & ALIGNMENT && elem->has("alignment")) {
        const std::string& alignment {elem->get<std::string>("alignment")};
        if (alignment == "left")
            setHorizontalAlignment(ALIGN_LEFT);
        else if (alignment == "center")
            setHorizontalAlignment(ALIGN_CENTER);
        else if (alignment == "right")
            setHorizontalAlignment(ALIGN_RIGHT);
        else
            LOG(LogWarning) << "DateTimeComponent: Invalid theme configuration, property "
                               "<alignment> defined as \""
                            << alignment << "\"";
    }

    if (properties & METADATA && elem->has("metadata")) {
        mThemeMetadata = "";
        const std::string& metadata {elem->get<std::string>("metadata")};
        if (metadata == "releasedate" || metadata == "lastplayed") {
            if (elem->has("defaultValue")) {
                const std::string& defaultValue {elem->get<std::string>("defaultValue")};
                if (defaultValue == ":space:")
                    mDefaultValue = " ";
                else
                    mDefaultValue = defaultValue;
            }
            mThemeMetadata = metadata;
        }
        else {
            LOG(LogError) << "DateTimeComponent: Invalid theme configuration, property "
                             "<metadata> defined as \""
                          << metadata << "\"";
        }
    }

    if (mThemeMetadata == "lastplayed")
        setDisplayRelative(true);

    if (elem->has("displayRelative"))
        setDisplayRelative(elem->get<bool>("displayRelative"));

    if (properties & LETTER_CASE && elem->has("letterCase")) {
        const std::string& letterCase {elem->get<std::string>("letterCase")};
        if (letterCase == "uppercase") {
            setUppercase(true);
        }
        else if (letterCase == "lowercase") {
            setLowercase(true);
        }
        else if (letterCase == "capitalize") {
            setCapitalize(true);
        }
        else if (letterCase != "none") {
            LOG(LogWarning) << "DateTimeComponent: Invalid theme configuration, property "
                               "<letterCase> defined as \""
                            << letterCase << "\"";
        }
    }

    float maxHeight {0.0f};

    if (!theme->isLegacyTheme() && elem->has("size")) {
        const glm::vec2 size {elem->get<glm::vec2>("size")};
        if (size.x != 0.0f && size.y != 0.0f)
            maxHeight = mSize.y * 2.0f;
    }

    // Legacy themes only.
    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
        setUppercase(elem->get<bool>("forceUppercase"));

    if (properties & LINE_SPACING && elem->has("lineSpacing"))
        setLineSpacing(glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f));

    setFont(Font::getFromTheme(elem, properties, mFont, maxHeight, false, theme->isLegacyTheme()));
}

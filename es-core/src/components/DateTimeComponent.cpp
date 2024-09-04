//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  DateTimeComponent.cpp
//
//  Provides the date and time, in absolute (actual date) or relative
//  (delta from current date and time) form.
//  Used by the gamelist views.
//

#include "components/DateTimeComponent.h"

#include "Log.h"
#include "Settings.h"
#include "utils/LocalizationUtil.h"
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
    : TextComponent {text, font, color,  horizontalAlignment, ALIGN_CENTER, glm::vec2 {1, 0},
                     pos,  size, bgcolor}
    , mRenderer {Renderer::getInstance()}
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
                return _p("theme", "never");
            else
                return mDefaultValue;
        }

        Utils::Time::DateTime now {Utils::Time::now()};
        Utils::Time::Duration dur {now.getTime() - mTime.getTime()};

        std::string buf;

        if (dur.getDays() > 0) {
            buf = Utils::String::format(_np("theme", "%i day ago", "%i days ago", dur.getDays()),
                                        dur.getDays());
        }
        else if (dur.getHours() > 0) {
            buf = Utils::String::format(_np("theme", "%i hour ago", "%i hours ago", dur.getHours()),
                                        dur.getHours());
        }
        else if (dur.getMinutes() > 0) {
            buf = Utils::String::format(
                _np("theme", "%i minute ago", "%i minutes ago", dur.getMinutes()),
                dur.getMinutes());
        }
        else {
            buf = Utils::String::format(
                _np("theme", "%i second ago", "%i seconds ago", dur.getSeconds()),
                dur.getSeconds());
        }

        return std::string(buf);
    }

    if (mTime.getTime() == 0) {
        if (mDefaultValue == "")
            return _p("theme", "unknown");
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
    using namespace ThemeFlags;
    GuiComponent::applyTheme(theme, view, element, properties);

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "datetime")};
    if (!elem)
        return;

    if (properties & ThemeFlags::POSITION && elem->has("stationary")) {
        const std::string& stationary {elem->get<std::string>("stationary")};
        if (stationary == "never")
            mStationary = Stationary::NEVER;
        else if (stationary == "always")
            mStationary = Stationary::ALWAYS;
        else if (stationary == "withinView")
            mStationary = Stationary::WITHIN_VIEW;
        else if (stationary == "betweenViews")
            mStationary = Stationary::BETWEEN_VIEWS;
        else
            LOG(LogWarning) << "DateTimeComponent: Invalid theme configuration, property "
                               "\"stationary\" for element \""
                            << element.substr(9) << "\" defined as \"" << stationary << "\"";
    }

    if (elem->has("format"))
        setFormat(elem->get<std::string>("format"));

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    setRenderBackground(false);
    if (properties & COLOR && elem->has("backgroundColor")) {
        setBackgroundColor(elem->get<unsigned int>("backgroundColor"));
        setRenderBackground(true);
    }

    if (elem->has("backgroundMargins")) {
        setBackgroundMargins(glm::clamp(elem->get<glm::vec2>("backgroundMargins"), 0.0f, 0.5f) *
                             mRenderer->getScreenWidth());
    }

    if (elem->has("backgroundCornerRadius")) {
        setBackgroundCornerRadius(
            glm::clamp(elem->get<float>("backgroundCornerRadius"), 0.0f, 0.5f) *
            mRenderer->getScreenWidth());
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
                               "\"horizontalAlignment\" for element \""
                            << element.substr(9) << "\" defined as \"" << horizontalAlignment
                            << "\"";
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
                               "\"verticalAlignment\" for element \""
                            << element.substr(9) << "\" defined as \"" << verticalAlignment << "\"";
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
            LOG(LogWarning) << "DateTimeComponent: Invalid theme configuration, property "
                               "\"metadata\" for element \""
                            << element.substr(9) << "\" defined as \"" << metadata << "\"";
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
                               "\"letterCase\" for element \""
                            << element.substr(9) << "\" defined as \"" << letterCase << "\"";
        }
    }

    float maxHeight {0.0f};
    bool hasSize {false};

    if (elem->has("size")) {
        const glm::vec2 size {elem->get<glm::vec2>("size")};
        if (size.x != 0.0f && size.y != 0.0f) {
            maxHeight = mSize.y * 2.0f;
            hasSize = true;
        }
    }

    if (properties & LINE_SPACING && elem->has("lineSpacing"))
        setLineSpacing(glm::clamp(elem->get<float>("lineSpacing"), 0.5f, 3.0f));

    if (getAutoCalcExtent() == glm::ivec2 {1, 0} && !hasSize)
        mSize.y = 0.0f;

    setFont(Font::getFromTheme(elem, properties, mFont, maxHeight));
    mSize = glm::round(mSize);
}

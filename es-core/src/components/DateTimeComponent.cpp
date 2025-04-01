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
    : mRenderer {Renderer::getInstance()}
    , mClockAccumulator {0}
    , mClockMode {false}
    , mDisplayRelative {false}
    , mBackgroundHorizontalPadding {0.0f, 0.0f}
    , mBackgroundVerticalPadding {0.0f, 0.0f}
    , mClockBgColor {0x00000000}
    , mClockBgColorEnd {0x00000000}
    , mClockColorGradientHorizontal {true}
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
    , mClockAccumulator {0}
    , mClockMode {false}
    , mDisplayRelative {false}
    , mBackgroundHorizontalPadding {0.0f, 0.0f}
    , mBackgroundVerticalPadding {0.0f, 0.0f}
    , mClockBgColor {0x00000000}
    , mClockBgColorEnd {0x00000000}
    , mClockColorGradientHorizontal {true}
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
    if (mClockMode)
        return (Utils::Time::timeToString(Utils::Time::DateTime {Utils::Time::now()}.getTime(),
                                          mFormat));

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

void DateTimeComponent::update(int deltaTime)
{
    updateSelf(deltaTime);

    if (!mClockMode || (mClockMode && !Settings::getInstance()->getBool("DisplayClock")))
        return;

    mClockAccumulator += deltaTime;

    if (mClockAccumulator >= 500) {
        mClockAccumulator = 0;
        mTime = Utils::Time::now();
        const std::string newTime {Utils::Time::timeToString(mTime, mFormat)};
        // The setValue() function with its text cache rebuild is an expensive operation so we only
        // call this when the actual date/time string needs updating.
        if (newTime != mText)
            setValue(newTime);
    }
}

void DateTimeComponent::render(const glm::mat4& parentTrans)
{
    if (mClockMode && !Settings::getInstance()->getBool("DisplayClock"))
        return;

    if (mClockMode && mClockBgColor != 0x00000000) {
        glm::mat4 trans {parentTrans * getTransform()};
        trans = glm::translate(trans, glm::vec3 {-mBackgroundHorizontalPadding.x,
                                                 -mBackgroundVerticalPadding.x, 0.0f});
        mRenderer->setMatrix(trans);

        mRenderer->drawRect(
            0.0f, 0.0f, mSize.x + mBackgroundHorizontalPadding.x + mBackgroundHorizontalPadding.y,
            mSize.y + mBackgroundVerticalPadding.x + mBackgroundVerticalPadding.y, mClockBgColor,
            mClockBgColorEnd, mClockColorGradientHorizontal, mThemeOpacity, 1.0f,
            Renderer::BlendFactor::SRC_ALPHA, Renderer::BlendFactor::ONE_MINUS_SRC_ALPHA,
            mBackgroundCornerRadius);
    }

    // Render the component.
    TextComponent::render(parentTrans);
}

void DateTimeComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                   const std::string& view,
                                   const std::string& element,
                                   unsigned int properties)
{
    using namespace ThemeFlags;

    std::string elementType {"datetime"};
    std::string componentName {"DateTimeComponent"};

    if (element.substr(0, 6) == "clock_") {
        mClockMode = true;
        elementType = "clock";
        componentName = "ClockComponent";
        // Apply default clock settings as the theme may not define any configuration for it.
        setFont(Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT));
        setLineSpacing(1.0f);
        const glm::vec2 scale {
            getParent() ? getParent()->getSize() :
                          glm::vec2 {mRenderer->getScreenWidth(), mRenderer->getScreenHeight()}};
        setPosition(0.018f * scale.x, 0.016f * scale.y);
        mSize.y = mFont->getLetterHeight();
        setColor(0xFFFFFFFF);
        setFormat("%H:%M");
    }

    GuiComponent::applyTheme(theme, view, element, properties);

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, elementType)};
    if (!elem)
        return;

    if (mClockMode && elem->has("scope")) {
        const std::string& scope {elem->get<std::string>("scope")};
        if (scope == "shared") {
            mComponentScope = ComponentScope::SHARED;
        }
        else if (scope == "view") {
            mComponentScope = ComponentScope::VIEW;
        }
        else if (scope == "menu") {
            mComponentScope = ComponentScope::MENU;
        }
        else if (scope == "none") {
            mComponentScope = ComponentScope::NONE;
        }
        else {
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
                               "\"scope\" for element \""
                            << element.substr(6) << "\" defined as \"" << scope << "\"";
        }
    }

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

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    setRenderBackground(false);
    if (properties & COLOR && elem->has("backgroundColor")) {
        if (mClockMode) {
            mClockBgColor = elem->get<unsigned int>("backgroundColor");

            if (elem->has("backgroundColorEnd"))
                mClockBgColorEnd = elem->get<unsigned int>("backgroundColorEnd");
            else
                mClockBgColorEnd = mClockBgColor;

            if (elem->has("backgroundGradientType")) {
                const std::string& backgroundGradientType {
                    elem->get<std::string>("backgroundGradientType")};
                if (backgroundGradientType == "horizontal") {
                    mClockColorGradientHorizontal = true;
                }
                else if (backgroundGradientType == "vertical") {
                    mClockColorGradientHorizontal = false;
                }
                else {
                    mClockColorGradientHorizontal = true;
                    LOG(LogWarning) << componentName
                                    << ": Invalid theme configuration, property "
                                       "\"backgroundGradientType\" for element \""
                                    << element.substr(6) << "\" defined as \""
                                    << backgroundGradientType << "\"";
                }
            }
        }
        else {
            setBackgroundColor(elem->get<unsigned int>("backgroundColor"));
            setRenderBackground(true);
        }
    }

    if (!mClockMode && elem->has("backgroundMargins")) {
        setBackgroundMargins(glm::clamp(elem->get<glm::vec2>("backgroundMargins"), 0.0f, 0.5f) *
                             mRenderer->getScreenWidth());
    }

    if (mClockMode && elem->has("backgroundHorizontalPadding")) {
        const glm::vec2 backgroundHorizontalPadding {
            glm::clamp(elem->get<glm::vec2>("backgroundHorizontalPadding"), 0.0f, 0.2f)};
        mBackgroundHorizontalPadding.x =
            backgroundHorizontalPadding.x * mRenderer->getScreenWidth();
        mBackgroundHorizontalPadding.y =
            backgroundHorizontalPadding.y * mRenderer->getScreenWidth();
    }

    if (mClockMode && elem->has("backgroundVerticalPadding")) {
        const glm::vec2 backgroundVerticalPadding {
            glm::clamp(elem->get<glm::vec2>("backgroundVerticalPadding"), 0.0f, 0.2f)};
        mBackgroundVerticalPadding.x = backgroundVerticalPadding.x * mRenderer->getScreenHeight();
        mBackgroundVerticalPadding.y = backgroundVerticalPadding.y * mRenderer->getScreenHeight();
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
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
                               "\"horizontalAlignment\" for element \""
                            << element.substr(elementType == "clock" ? 6 : 9) << "\" defined as \""
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
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
                               "\"verticalAlignment\" for element \""
                            << element.substr(elementType == "clock" ? 6 : 9) << "\" defined as \""
                            << verticalAlignment << "\"";
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
            LOG(LogWarning) << componentName
                            << ": Invalid theme configuration, property "
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

    if (elem->has("format"))
        setFormat(elem->get<std::string>("format"));
    else if (mClockMode)
        setFormat("%H:%M");
}

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DateTimeEditComponent.cpp
//
//  Date and time edit component.
//

#include "components/DateTimeEditComponent.h"

#include "Settings.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"

DateTimeEditComponent::DateTimeEditComponent(Window* window, bool alignRight, DisplayMode dispMode)
    : GuiComponent(window)
    , mEditing(false)
    , mEditIndex(0)
    , mDisplayMode(dispMode)
    , mRelativeUpdateAccumulator(0)
    , mColor(0x777777FF)
    , mFont(Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT))
    , mUppercase(false)
    , mAutoSize(true)
    , mAlignRight(alignRight)
{
    updateTextCache();
}

void DateTimeEditComponent::setDisplayMode(DisplayMode mode)
{
    mDisplayMode = mode;
    updateTextCache();
}

bool DateTimeEditComponent::input(InputConfig* config, Input input)
{
    if (input.value == 0)
        return false;

    if (config->isMappedTo("a", input)) {
        if (mDisplayMode != DISP_RELATIVE_TO_NOW) // Don't allow editing for relative times.
            mEditing = !mEditing;

        // Change the color of the text to reflect the changes.
        if (mTime == mOriginalValue)
            setColor(mColorOriginalValue);
        else
            setColor(mColorChangedValue);

        if (mEditing) {
            // Started editing.
            mTimeBeforeEdit = mTime;

            // Initialize to now if unset.
            if (mTime.getTime() == Utils::Time::DEFAULT_TIMEVALUE) {
                mTime = Utils::Time::now();
                updateTextCache();
            }
        }

        return true;
    }

    if (mEditing) {

        if (config->isMappedLike("lefttrigger", input) ||
            config->isMappedLike("righttrigger", input)) {
            return true;
        }

        if (config->isMappedTo("b", input)) {
            mEditing = false;
            mTime = mTimeBeforeEdit;
            updateTextCache();
            return true;
        }

        int incDir = 0;
        if (config->isMappedLike("up", input) || config->isMappedLike("rightshoulder", input))
            incDir = 1;
        else if (config->isMappedLike("down", input) || config->isMappedLike("leftshoulder", input))
            incDir = -1;

        if (incDir != 0) {
            tm new_tm = mTime;

            // ISO 8601 date format.
            if (mEditIndex == 0) {
                new_tm.tm_year += incDir;

                if (new_tm.tm_year < 0)
                    new_tm.tm_year = 0;
            }
            else if (mEditIndex == 1) {
                new_tm.tm_mon += incDir;

                if (new_tm.tm_mon > 11)
                    new_tm.tm_mon = 0;
                else if (new_tm.tm_mon < 0)
                    new_tm.tm_mon = 11;
            }
            else if (mEditIndex == 2) {
                const int days_in_month =
                    Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1);
                new_tm.tm_mday += incDir;

                if (new_tm.tm_mday > days_in_month)
                    new_tm.tm_mday = 1;
                else if (new_tm.tm_mday < 1)
                    new_tm.tm_mday = days_in_month;
            }

            // Validate day.
            const int days_in_month =
                Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1);
            if (new_tm.tm_mday > days_in_month)
                new_tm.tm_mday = days_in_month;

            mTime = new_tm;

            updateTextCache();
            return true;
        }

        if (config->isMappedLike("right", input)) {
            mEditIndex++;
            if (mEditIndex >= static_cast<int>(mCursorBoxes.size()))
                mEditIndex--;
            return true;
        }

        if (config->isMappedLike("left", input)) {
            mEditIndex--;
            if (mEditIndex < 0)
                mEditIndex++;
            return true;
        }
    }

    return GuiComponent::input(config, input);
}

void DateTimeEditComponent::update(int deltaTime)
{
    if (mDisplayMode == DISP_RELATIVE_TO_NOW) {
        mRelativeUpdateAccumulator += deltaTime;
        if (mRelativeUpdateAccumulator > 1000) {
            mRelativeUpdateAccumulator = 0;
            updateTextCache();
        }
    }

    GuiComponent::update(deltaTime);
}

void DateTimeEditComponent::render(const Transform4x4f& parentTrans)
{
    Transform4x4f trans = parentTrans * getTransform();

    if (mTextCache) {
        std::shared_ptr<Font> font = getFont();
        float referenceSize;

        if (mAlignRight) {
            if (mTime != 0)
                referenceSize = font->sizeText("ABCDEFG").x();
            else
                referenceSize = font->sizeText("ABCDEIJ").x();
        }

        // Vertically center.
        Vector3f off(0, (mSize.y() - mTextCache->metrics.size.y()) / 2.0f, 0.0f);

        if (mAlignRight)
            off.x() += referenceSize - mTextCache->metrics.size.x();
        trans.translate(off);

        Renderer::setMatrix(trans);

        if (Settings::getInstance()->getBool("DebugText")) {
            Renderer::setMatrix(trans);
            if (mTextCache->metrics.size.x() > 0) {
                Renderer::drawRect(0.0f, 0.0f - off.y(), mSize.x() - off.x(), mSize.y(), 0x0000FF33,
                                   0x0000FF33);
            }
            Renderer::drawRect(0.0f, 0.0f, mTextCache->metrics.size.x(),
                               mTextCache->metrics.size.y(), 0x00000033, 0x00000033);
        }

        mTextCache->setColor((mColor & 0xFFFFFF00) | getOpacity());
        font->renderTextCache(mTextCache.get());

        if (mEditing) {
            if (mEditIndex >= 0 && static_cast<unsigned int>(mEditIndex) < mCursorBoxes.size())
                Renderer::drawRect(mCursorBoxes[mEditIndex][0], mCursorBoxes[mEditIndex][1],
                                   mCursorBoxes[mEditIndex][2], mCursorBoxes[mEditIndex][3],
                                   0x00000022, 0x00000022);
        }
    }
}

void DateTimeEditComponent::setValue(const std::string& val)
{
    mTime = val;
    mOriginalValue = val;
    updateTextCache();
}

std::string DateTimeEditComponent::getDisplayString(DisplayMode mode) const
{
    // ISO 8601 date format.
    std::string fmt;
    switch (mode) {
        case DISP_DATE: {
            if (mTime.getTime() == 0)
                return "unknown";
            fmt = "%Y-%m-%d";
            break;
        }
        case DISP_DATE_TIME: {
            if (mTime.getTime() == 0)
                return "unknown";
            fmt = "%Y-%m-%d %H:%M:%S";
            break;
        }
        case DISP_RELATIVE_TO_NOW: {
            // Relative time.
            if (mTime.getTime() == 0)
                return "never";

            Utils::Time::DateTime now(Utils::Time::now());
            Utils::Time::Duration dur(now.getTime() - mTime.getTime());

            std::string buf;

            if (dur.getDays() > 0)
                buf = std::to_string(dur.getDays()) + // Line break.
                      " day" + (dur.getDays() > 1 ? "s" : "") + " ago";
            else if (dur.getHours() > 0)
                buf = std::to_string(dur.getHours()) + // Line break.
                      " hour" + (dur.getHours() > 1 ? "s" : "") + " ago";
            else if (dur.getMinutes() > 0)
                buf = std::to_string(dur.getMinutes()) + // Line break.
                      " minute" + (dur.getMinutes() > 1 ? "s" : "") + " ago";
            else
                buf = std::to_string(dur.getSeconds()) + // Line break.
                      " second" + (dur.getSeconds() > 1 || dur.getSeconds() == 0 ? "s" : "") +
                      " ago";

            return std::string(buf);
            break;
        }
    }

    return Utils::Time::timeToString(mTime, fmt);
}

std::shared_ptr<Font> DateTimeEditComponent::getFont() const
{
    if (mFont)
        return mFont;

    return Font::get(FONT_SIZE_MEDIUM);
}

void DateTimeEditComponent::updateTextCache()
{
    DisplayMode mode = getCurrentDisplayMode();

    std::string dispString;

    // Hack to set date string to blank instead of 'unknown'.
    // The calling function simply needs to set this string using setValue().
    if (mTime.getIsoString() == "19700101T010101") {
        dispString = "";
    }
    else {
        dispString =
            mUppercase ? Utils::String::toUpper(getDisplayString(mode)) : getDisplayString(mode);
    }
    std::shared_ptr<Font> font = getFont();
    mTextCache = std::unique_ptr<TextCache>(font->buildTextCache(dispString, 0, 0, mColor));

    if (mAutoSize) {
        mSize = mTextCache->metrics.size;
        mAutoSize = false;

        if (getParent())
            getParent()->onSizeChanged();
    }

    if (dispString == "unknown   " || dispString == "")
        return;

    // Set up cursor positions.
    mCursorBoxes.clear();

    if (dispString.empty() || mode == DISP_RELATIVE_TO_NOW)
        return;

    // Year.
    Vector2f start(0, 0);
    Vector2f end = font->sizeText(dispString.substr(0, 4));
    Vector2f diff = end - start;
    mCursorBoxes.push_back(Vector4f(start[0], start[1], diff[0], diff[1]));

    // Month.
    start[0] = font->sizeText(dispString.substr(0, 5)).x();
    end = font->sizeText(dispString.substr(0, 7));
    diff = end - start;
    mCursorBoxes.push_back(Vector4f(start[0], start[1], diff[0], diff[1]));

    // Day.
    start[0] = font->sizeText(dispString.substr(0, 8)).x();
    end = font->sizeText(dispString.substr(0, 10));
    diff = end - start;
    mCursorBoxes.push_back(Vector4f(start[0], start[1], diff[0], diff[1]));

    // The logic for handling time for 'mode = DISP_DATE_TIME' is missing, but
    // nobody will use it anyway so it's not worthwhile implementing.
}

void DateTimeEditComponent::setColor(unsigned int color)
{
    mColor = color;
    if (mTextCache)
        mTextCache->setColor(color);
}

void DateTimeEditComponent::setFont(std::shared_ptr<Font> font)
{
    mFont = font;
    updateTextCache();
}

void DateTimeEditComponent::onSizeChanged()
{
    mAutoSize = false;
    updateTextCache();
}

void DateTimeEditComponent::setUppercase(bool uppercase)
{
    mUppercase = uppercase;
    updateTextCache();
}

void DateTimeEditComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                       const std::string& view,
                                       const std::string& element,
                                       unsigned int properties)
{
    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "datetime");

    if (!elem)
        return;

    // We set mAutoSize BEFORE calling GuiComponent::applyTheme because it calls
    // setSize(), which will call updateTextCache(), which will reset mSize if
    // mAutoSize == true, ignoring the theme's value.
    if (properties & ThemeFlags::SIZE)
        mAutoSize = !elem->has("size");

    GuiComponent::applyTheme(theme, view, element, properties);

    using namespace ThemeFlags;

    if (properties & COLOR && elem->has("color"))
        setColor(elem->get<unsigned int>("color"));

    if (properties & FORCE_UPPERCASE && elem->has("forceUppercase"))
        setUppercase(elem->get<bool>("forceUppercase"));

    setFont(Font::getFromTheme(elem, properties, mFont));
}

std::vector<HelpPrompt> DateTimeEditComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", "edit date"));
    return prompts;
}

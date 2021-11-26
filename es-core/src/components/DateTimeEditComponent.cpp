//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DateTimeEditComponent.cpp
//
//  Date and time edit component.
//

#define KEY_REPEAT_START_DELAY 600
#define KEY_REPEAT_SPEED 150 // Lower is faster.

#include "components/DateTimeEditComponent.h"

#include "Settings.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"

DateTimeEditComponent::DateTimeEditComponent(Window* window, bool alignRight, DisplayMode dispMode)
    : GuiComponent{window}
    , mEditing{false}
    , mEditIndex{0}
    , mDisplayMode{dispMode}
    , mKeyRepeatDir{0}
    , mKeyRepeatTimer{0}
    , mRelativeUpdateAccumulator{0}
    , mColor{0x777777FF}
    , mFont{Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT)}
    , mAlignRight{alignRight}
    , mUppercase{false}
    , mAutoSize{true}
{
    updateTextCache();
}

void DateTimeEditComponent::onSizeChanged()
{
    mAutoSize = false;
    updateTextCache();
}

void DateTimeEditComponent::setValue(const std::string& val)
{
    mTime = val;
    mOriginalValue = val;
    updateTextCache();
}

bool DateTimeEditComponent::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value) {
        if (mDisplayMode != DISP_RELATIVE_TO_NOW) // Don't allow editing for relative times.
            mEditing = !mEditing;

        mKeyRepeatDir = 0;

        // Change the color of the text to reflect the changes.
        if (mTime == mOriginalValue)
            setColor(mColorOriginalValue);
        else
            setColor(mColorChangedValue);

        if (mEditing) {
            // Started editing.
            mTimeBeforeEdit = mTime;

            // Initialize to the arbitrary value 1999-01-01 if unset.
            if (mTime == 0) {
                mTime = Utils::Time::stringToTime("19990101T000000");
                updateTextCache();
            }
        }

        updateHelpPrompts();
        return true;
    }

    if (mEditing) {

        if ((config->isMappedLike("lefttrigger", input) ||
             config->isMappedLike("righttrigger", input))) {
            mKeyRepeatDir = 0;
            return true;
        }

        if (config->isMappedTo("y", input) && input.value) {
            mEditing = false;
            mTime = mTimeBeforeEdit;
            mKeyRepeatDir = 0;
            updateTextCache();
            return false;
        }

        if (config->isMappedTo("b", input) && input.value) {
            mEditing = false;
            mTime = mTimeBeforeEdit;
            mKeyRepeatDir = 0;
            updateTextCache();
            updateHelpPrompts();
            return true;
        }

        if (config->isMappedLike("up", input) || config->isMappedLike("rightshoulder", input)) {
            if (input.value) {
                mKeyRepeatDir = 1;
                mKeyRepeatTimer = -(KEY_REPEAT_START_DELAY - KEY_REPEAT_SPEED);
                changeDate();
                return true;
            }
            else {
                mKeyRepeatDir = 0;
            }
        }
        else if (config->isMappedLike("down", input) ||
                 config->isMappedLike("leftshoulder", input)) {
            if (input.value) {
                mKeyRepeatDir = -1;
                mKeyRepeatTimer = -(KEY_REPEAT_START_DELAY - KEY_REPEAT_SPEED);
                changeDate();
                return true;
            }
            else {
                mKeyRepeatDir = 0;
            }
        }

        if (mTime != 0 && config->isMappedLike("right", input) && input.value) {
            ++mEditIndex;
            if (mEditIndex >= static_cast<int>(mCursorBoxes.size()))
                --mEditIndex;
            mKeyRepeatDir = 0;
            return true;
        }

        if (mTime != 0 && config->isMappedLike("left", input) && input.value) {
            --mEditIndex;
            if (mEditIndex < 0)
                ++mEditIndex;
            mKeyRepeatDir = 0;
            return true;
        }
    }

    return GuiComponent::input(config, input);
}

void DateTimeEditComponent::update(int deltaTime)
{
    if (mKeyRepeatDir != 0) {
        mKeyRepeatTimer += deltaTime;
        while (mKeyRepeatTimer >= KEY_REPEAT_SPEED) {
            mKeyRepeatTimer -= KEY_REPEAT_SPEED;
            changeDate();
        }
    }

    if (mDisplayMode == DISP_RELATIVE_TO_NOW) {
        mRelativeUpdateAccumulator += deltaTime;
        if (mRelativeUpdateAccumulator > 1000) {
            mRelativeUpdateAccumulator = 0;
            updateTextCache();
        }
    }

    GuiComponent::update(deltaTime);
}

void DateTimeEditComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans{parentTrans * getTransform()};

    if (mTextCache) {
        std::shared_ptr<Font> font = getFont();
        float referenceSize;

        if (mAlignRight)
            referenceSize = std::round(mParent->getSize().x * 0.1045f);

        // Vertically center.
        glm::vec3 off{0.0f, (mSize.y - mTextCache->metrics.size.y) / 2.0f, 0.0f};

        if (mAlignRight)
            off.x += referenceSize - mTextCache->metrics.size.x;
        trans = glm::translate(trans, off);

        Renderer::setMatrix(trans);

        if (Settings::getInstance()->getBool("DebugText")) {
            Renderer::setMatrix(trans);
            if (mTextCache->metrics.size.x > 0.0f) {
                Renderer::drawRect(0.0f, 0.0f - off.y, mSize.x - off.x, mSize.y, 0x0000FF33,
                                   0x0000FF33);
            }
            Renderer::drawRect(0.0f, 0.0f, mTextCache->metrics.size.x, mTextCache->metrics.size.y,
                               0x00000033, 0x00000033);
        }

        mTextCache->setColor((mColor & 0xFFFFFF00) | getOpacity());
        font->renderTextCache(mTextCache.get());

        if (mEditing && mTime != 0) {
            if (mEditIndex >= 0 && static_cast<unsigned int>(mEditIndex) < mCursorBoxes.size())
                Renderer::drawRect(mCursorBoxes[mEditIndex][0], mCursorBoxes[mEditIndex][1],
                                   mCursorBoxes[mEditIndex][2], mCursorBoxes[mEditIndex][3],
                                   0x00000022, 0x00000022);
        }
    }
}

void DateTimeEditComponent::setDisplayMode(DisplayMode mode)
{
    mDisplayMode = mode;
    updateTextCache();
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

void DateTimeEditComponent::setUppercase(bool uppercase)
{
    mUppercase = uppercase;
    updateTextCache();
}

std::vector<HelpPrompt> DateTimeEditComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (!mEditing) {
        prompts.push_back(HelpPrompt("a", "edit date"));
    }
    else {
        prompts.push_back(HelpPrompt("b", "cancel"));
        prompts.push_back(HelpPrompt("a", "apply"));
        prompts.push_back(HelpPrompt("left/right", "Y-M-D"));
        prompts.push_back(HelpPrompt("up/down", "modify"));
    }
    return prompts;
}

std::shared_ptr<Font> DateTimeEditComponent::getFont() const
{
    if (mFont)
        return mFont;

    return Font::get(FONT_SIZE_MEDIUM);
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

void DateTimeEditComponent::changeDate()
{
    tm new_tm = mTime;

    if (mTime.getIsoString() == "19700101T000000")
#if defined(_WIN64)
        new_tm = {0, 0, 0, 1, 0, 70, 0, 0, -1};
#else
        new_tm = {0, 0, 0, 1, 0, 70, 0, 0, -1, 0, 0};
#endif

    // ISO 8601 date format.
    if (mEditIndex == 0) {
        new_tm.tm_year += mKeyRepeatDir;

        if (new_tm.tm_year < 0)
            new_tm.tm_year = 0;
    }
    else if (mEditIndex == 1) {
        new_tm.tm_mon += mKeyRepeatDir;

        if (new_tm.tm_mon > 11)
            new_tm.tm_mon = 0;
        else if (new_tm.tm_mon < 0)
            new_tm.tm_mon = 11;
    }
    else if (mEditIndex == 2) {
        const int days_in_month =
            Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1);
        new_tm.tm_mday += mKeyRepeatDir;

        if (new_tm.tm_mday > days_in_month)
            new_tm.tm_mday = 1;
        else if (new_tm.tm_mday < 1)
            new_tm.tm_mday = days_in_month;
    }

    // Validate day.
    const int days_in_month = Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1);
    if (new_tm.tm_mday > days_in_month)
        new_tm.tm_mday = days_in_month;

    if (mktime(&new_tm) <= 0)
        mTime = 0;
    else
        mTime = new_tm;

    updateTextCache();
}

void DateTimeEditComponent::updateTextCache()
{
    DisplayMode mode = getCurrentDisplayMode();

    std::string dispString;

    // Hack to set date string to blank instead of 'unknown'.
    // The calling function simply needs to set this string using setValue().
    if (mTime.getIsoString() == "19710101T010101") {
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
    glm::vec2 start{};
    glm::vec2 end{font->sizeText(dispString.substr(0, 4))};
    glm::vec2 diff{end - start};
    mCursorBoxes.push_back(glm::vec4{start[0], start[1], diff[0], diff[1]});

    // Month.
    start[0] = font->sizeText(dispString.substr(0, 5)).x;
    end = font->sizeText(dispString.substr(0, 7));
    diff = end - start;
    mCursorBoxes.push_back(glm::vec4{start[0], start[1], diff[0], diff[1]});

    // Day.
    start[0] = font->sizeText(dispString.substr(0, 8)).x;
    end = font->sizeText(dispString.substr(0, 10));
    diff = end - start;
    mCursorBoxes.push_back(glm::vec4{start[0], start[1], diff[0], diff[1]});

    // The logic for handling time for 'mode = DISP_DATE_TIME' is missing, but
    // nobody will use it anyway so it's not worthwhile implementing.
}

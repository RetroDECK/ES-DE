//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  DateTimeEditComponent.cpp
//
//  Date and time edit component.
//

#define KEY_REPEAT_START_DELAY 600
#define KEY_REPEAT_SPEED 150 // Lower is faster.

#include "components/DateTimeEditComponent.h"

#include "Settings.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

DateTimeEditComponent::DateTimeEditComponent(bool alignRight)
    : mRenderer {Renderer::getInstance()}
    , mEditing {false}
    , mEditIndex {0}
    , mKeyRepeatDir {0}
    , mKeyRepeatTimer {0}
    , mColor {mMenuColorPrimary}
    , mAlignRight {alignRight}
    , mUppercase {false}
    , mAutoSize {true}
{
    mDateText = std::make_unique<TextComponent>("", Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT));
    updateText();
}

void DateTimeEditComponent::onSizeChanged()
{
    mAutoSize = false;
    updateText();
}

void DateTimeEditComponent::setValue(const std::string& val)
{
    mTime = val;
    mOriginalValue = val;
    if (mAlignRight)
        mAutoSize = true;
    updateText();
}

bool DateTimeEditComponent::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value) {
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
                mAutoSize = true;
                updateText();
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
            updateText();
            return false;
        }

        if (config->isMappedTo("b", input) && input.value) {
            mEditing = false;
            mTime = mTimeBeforeEdit;
            mKeyRepeatDir = 0;
            mAutoSize = true;
            updateText();
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

    GuiComponent::update(deltaTime);
}

void DateTimeEditComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    // Center vertically.
    glm::vec3 off {0.0f, (mSize.y - mDateText->getSize().y) / 2.0f, 0.0f};

    trans = glm::translate(trans, glm::round(off));
    mRenderer->setMatrix(trans);

    if (Settings::getInstance()->getBool("DebugText") && mDateText->getValue() != "") {
        mRenderer->setMatrix(trans);
        mDateText->setDebugRendering(false);
        if (mDateText->getSize().x > 0.0f) {
            mRenderer->drawRect(0.0f, 0.0f - off.y, mSize.x - off.x, mSize.y, 0x0000FF33,
                                0x0000FF33);
        }
        mRenderer->drawRect(0.0f, 0.0f, mDateText->getSize().x, mDateText->getSize().y, 0x00000033,
                            0x00000033);
    }

    mDateText->setColor((mColor & 0xFFFFFF00) | static_cast<int>(getOpacity() * 255.0f));
    mDateText->render(trans);

    if (mEditing && mTime != 0) {
        if (mEditIndex >= 0 && static_cast<unsigned int>(mEditIndex) < mCursorBoxes.size())
            mRenderer->drawRect(mCursorBoxes[mEditIndex][0], mCursorBoxes[mEditIndex][1],
                                mCursorBoxes[mEditIndex][2], mCursorBoxes[mEditIndex][3],
                                mMenuColorDateTimeEditMarker, mMenuColorDateTimeEditMarker);
    }
}

void DateTimeEditComponent::setColor(unsigned int color)
{
    mColor = color;
    mDateText->setColor(color);
}

void DateTimeEditComponent::setFont(std::shared_ptr<Font> font)
{
    mDateText->setFont(font);
    updateText();
}

void DateTimeEditComponent::setUppercase(bool uppercase)
{
    mUppercase = uppercase;
    updateText();
}

std::vector<HelpPrompt> DateTimeEditComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (!mEditing) {
        prompts.push_back(HelpPrompt("a", _("edit date")));
    }
    else {
        prompts.push_back(HelpPrompt("b", _("cancel")));
        prompts.push_back(HelpPrompt("a", _("apply")));
        prompts.push_back(HelpPrompt("left/right", _("Y-M-D")));
        prompts.push_back(HelpPrompt("up/down", _("modify")));
    }
    return prompts;
}

std::string DateTimeEditComponent::getDisplayString() const
{
    // ISO 8601 date format.
    std::string fmt;
    if (mTime.getTime() == 0)
        return _("unknown");
    fmt = "%Y-%m-%d";

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
        const int days_in_month {
            Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1)};
        new_tm.tm_mday += mKeyRepeatDir;

        if (new_tm.tm_mday > days_in_month)
            new_tm.tm_mday = 1;
        else if (new_tm.tm_mday < 1)
            new_tm.tm_mday = days_in_month;
    }

    // Validate day.
    const int days_in_month {Utils::Time::daysInMonth(new_tm.tm_year + 1900, new_tm.tm_mon + 1)};
    if (new_tm.tm_mday > days_in_month)
        new_tm.tm_mday = days_in_month;

    if (mktime(&new_tm) <= 0)
        mTime = 0;
    else
        mTime = new_tm;

    mAutoSize = true;
    updateText();
}

void DateTimeEditComponent::updateText()
{
    std::string dispString;

    // Hack to set date string to blank instead of 'unknown'.
    // The calling function simply needs to set this string using setValue().
    if (mTime.getIsoString() == "19710101T010101")
        dispString = "";
    else
        dispString = mUppercase ? Utils::String::toUpper(getDisplayString()) : getDisplayString();

    mDateText->setText(dispString);
    mDateText->setColor(mColor);

    if (mAlignRight)
        mSize = mDateText->getSize();

    if (mAutoSize) {
        mSize = mDateText->getSize();
        mAutoSize = false;

        if (getParent())
            getParent()->onSizeChanged();
    }

    mCursorBoxes.clear();

    if (dispString.empty() || dispString == _("unknown"))
        return;

    // Set up cursor positions.

    std::shared_ptr<Font> font {mDateText->getFont()};

    // Year.
    glm::vec2 start {0.0f, 0.0f};
    glm::vec2 end {font->sizeText(dispString.substr(0, 4))};
    glm::vec2 diff {end - start};
    mCursorBoxes.push_back(glm::vec4 {start[0], start[1], diff[0], diff[1]});

    // Month.
    start[0] = font->sizeText(dispString.substr(0, 5)).x;
    end = font->sizeText(dispString.substr(0, 7));
    diff = end - start;
    mCursorBoxes.push_back(glm::vec4 {start[0], start[1], diff[0], diff[1]});

    // Day.
    start[0] = font->sizeText(dispString.substr(0, 8)).x;
    end = font->sizeText(dispString.substr(0, 10));
    diff = end - start;
    mCursorBoxes.push_back(glm::vec4 {start[0], start[1], diff[0], diff[1]});
}

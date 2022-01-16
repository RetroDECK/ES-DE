//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextEditComponent.cpp
//
//  Component for editing text fields in menus.
//

#include "components/TextEditComponent.h"

#include "utils/StringUtil.h"

#define TEXT_PADDING_HORIZ 12.0f
#define TEXT_PADDING_VERT 2.0f

#define CURSOR_REPEAT_START_DELAY 500
#define CURSOR_REPEAT_SPEED 28 // Lower is faster.

#define BLINKTIME 1000

TextEditComponent::TextEditComponent(Window* window)
    : GuiComponent {window}
    , mFocused {false}
    , mEditing {false}
    , mCursor {0}
    , mBlinkTime {0}
    , mCursorRepeatDir {0}
    , mScrollOffset {0.0f, 0.0f}
    , mBox {window, ":/graphics/textinput.svg"}
    , mFont {Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT)}
{
    mBox.setSharpCorners(true);
    addChild(&mBox);
    onFocusLost();
    setSize(4096, mFont->getHeight() + (TEXT_PADDING_VERT * Renderer::getScreenHeightModifier()));
}

void TextEditComponent::onFocusGained()
{
    mFocused = true;
    mBox.setImagePath(":/graphics/textinput_focused.svg");
    startEditing();
}

void TextEditComponent::onFocusLost()
{
    mFocused = false;
    mBox.setImagePath(":/graphics/textinput.svg");
}

void TextEditComponent::onSizeChanged()
{
    mBox.fitTo(
        mSize, glm::vec3 {},
        glm::vec2 {-34.0f, -32.0f - (TEXT_PADDING_VERT * Renderer::getScreenHeightModifier())});
    onTextChanged(); // Wrap point probably changed.
}

void TextEditComponent::setValue(const std::string& val)
{
    mText = val;
    mTextOrig = val;
    onTextChanged();
    onCursorChanged();
}

void TextEditComponent::textInput(const std::string& text)
{
    if (mEditing) {
        mBlinkTime = 0;
        mCursorRepeatDir = 0;
        if (text[0] == '\b') {
            if (mCursor > 0) {
                size_t newCursor = Utils::String::prevCursor(mText, mCursor);
                mText.erase(mText.begin() + newCursor, mText.begin() + mCursor);
                mCursor = static_cast<unsigned int>(newCursor);
            }
        }
        else {
            mText.insert(mCursor, text);
            mCursor += static_cast<unsigned int>(text.size());
        }
    }

    onTextChanged();
    onCursorChanged();
}

std::string TextEditComponent::getValue() const
{
    if (mText.empty())
        return "";

    // If mText only contains whitespace characters, then return an empty string.
    if (std::find_if(mText.cbegin(), mText.cend(), [](char c) {
            return !std::isspace(static_cast<unsigned char>(c));
        }) == mText.cend()) {
        return "";
    }
    else {
        return mText;
    }
}

void TextEditComponent::startEditing()
{
    SDL_StartTextInput();
    mEditing = true;
    updateHelpPrompts();
    mBlinkTime = BLINKTIME / 6;
}

void TextEditComponent::stopEditing()
{
    SDL_StopTextInput();
    mEditing = false;
    mCursorRepeatDir = 0;
    updateHelpPrompts();
}

bool TextEditComponent::input(InputConfig* config, Input input)
{
    bool const cursor_left =
        (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedLike("left", input)) ||
        (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_LEFT);
    bool const cursor_right =
        (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedLike("right", input)) ||
        (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_RIGHT);
    bool const cursor_up =
        (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedLike("up", input)) ||
        (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_UP);
    bool const cursor_down =
        (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedLike("down", input)) ||
        (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_DOWN);
    bool const shoulder_left = (config->isMappedLike("leftshoulder", input));
    bool const shoulder_right = (config->isMappedLike("rightshoulder", input));
    bool const trigger_left = (config->isMappedLike("lefttrigger", input));
    bool const trigger_right = (config->isMappedLike("righttrigger", input));

    if (input.value == 0) {
        if (cursor_left || cursor_right || cursor_up || cursor_down || shoulder_left ||
            shoulder_right | trigger_left || trigger_right) {
            mCursorRepeatDir = 0;
        }

        return false;
    }

    if ((config->isMappedTo("a", input) ||
         (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_RETURN)) &&
        mFocused && !mEditing) {
        startEditing();
        return true;
    }

    if (mEditing) {
        if (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_RETURN) {
            if (isMultiline())
                textInput("\n");
            else
                stopEditing();

            return true;
        }

        if (cursor_left || cursor_right) {
            mBlinkTime = 0;
            mCursorRepeatDir = cursor_left ? -1 : 1;
            mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
            moveCursor(mCursorRepeatDir);
        }
        // Stop editing and let the button down event be captured by the parent component.
        else if (cursor_down) {
            stopEditing();
            return false;
        }
        else if (shoulder_left || shoulder_right) {
            mBlinkTime = 0;
            mCursorRepeatDir = shoulder_left ? -10 : 10;
            mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
            moveCursor(mCursorRepeatDir);
        }
        // Jump to beginning of text.
        else if (trigger_left) {
            mBlinkTime = 0;
            setCursor(0);
        }
        // Jump to end of text.
        else if (trigger_right) {
            mBlinkTime = 0;
            setCursor(mText.length());
        }
        else if (config->getDeviceId() == DEVICE_KEYBOARD) {
            switch (input.id) {
                case SDLK_HOME: {
                    setCursor(0);
                    break;
                }
                case SDLK_END: {
                    setCursor(std::string::npos);
                    break;
                }
                case SDLK_DELETE: {
                    if (mCursor < static_cast<int>(mText.length())) {
                        // Fake as Backspace one char to the right.
                        moveCursor(1);
                        textInput("\b");
                    }
                    break;
                }
            }
        }
        // Consume all input when editing text.
        return true;
    }

    return false;
}

void TextEditComponent::update(int deltaTime)
{
    updateCursorRepeat(deltaTime);
    GuiComponent::update(deltaTime);

    mBlinkTime += deltaTime;
    if (mBlinkTime >= BLINKTIME)
        mBlinkTime = 0;
}

void TextEditComponent::updateCursorRepeat(int deltaTime)
{
    if (mCursorRepeatDir == 0)
        return;

    mCursorRepeatTimer += deltaTime;
    while (mCursorRepeatTimer >= CURSOR_REPEAT_SPEED) {
        mBlinkTime = 0;
        moveCursor(mCursorRepeatDir);
        mCursorRepeatTimer -= CURSOR_REPEAT_SPEED;
    }
}

void TextEditComponent::moveCursor(int amt)
{
    mCursor = static_cast<unsigned int>(Utils::String::moveCursor(mText, mCursor, amt));
    onCursorChanged();
}

void TextEditComponent::setCursor(size_t pos)
{
    if (pos == std::string::npos)
        mCursor = static_cast<unsigned int>(mText.length());
    else
        mCursor = static_cast<int>(pos);

    moveCursor(0);
}

void TextEditComponent::onTextChanged()
{
    std::string wrappedText = (isMultiline() ? mFont->wrapText(mText, getTextAreaSize().x) : mText);
    mTextCache = std::unique_ptr<TextCache>(
        mFont->buildTextCache(wrappedText, 0.0f, 0.0f, 0x77777700 | getOpacity()));

    if (mCursor > static_cast<int>(mText.length()))
        mCursor = static_cast<int>(mText.length());
}

void TextEditComponent::onCursorChanged()
{
    if (isMultiline()) {
        glm::vec2 textSize {mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x, mCursor)};

        // Need to scroll down?
        if (mScrollOffset.y + getTextAreaSize().y < textSize.y + mFont->getHeight())
            mScrollOffset.y = textSize.y - getTextAreaSize().y + mFont->getHeight();
        // Need to scroll up?
        else if (mScrollOffset.y > textSize.y)
            mScrollOffset.y = textSize.y;
    }
    else {
        glm::vec2 cursorPos {mFont->sizeText(mText.substr(0, mCursor))};

        if (mScrollOffset.x + getTextAreaSize().x < cursorPos.x)
            mScrollOffset.x = cursorPos.x - getTextAreaSize().x;
        else if (mScrollOffset.x > cursorPos.x)
            mScrollOffset.x = cursorPos.x;
    }
}

void TextEditComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {getTransform() * parentTrans};
    renderChildren(trans);

    // Text + cursor rendering.
    // Offset into our "text area" (padding).
    trans = glm::translate(trans, glm::vec3 {getTextAreaPos().x, getTextAreaPos().y, 0.0f});

    glm::ivec2 clipPos {static_cast<int>(trans[3].x), static_cast<int>(trans[3].y)};
    // Use "text area" size for clipping.
    glm::vec3 dimScaled {};
    dimScaled.x = std::fabs(trans[3].x + getTextAreaSize().x);
    dimScaled.y = std::fabs(trans[3].y + getTextAreaSize().y);

    glm::ivec2 clipDim {static_cast<int>(dimScaled.x - trans[3].x),
                        static_cast<int>(dimScaled.y - trans[3].y)};
    Renderer::pushClipRect(clipPos, clipDim);

    trans = glm::translate(trans, glm::vec3 {-mScrollOffset.x, -mScrollOffset.y, 0.0f});
    Renderer::setMatrix(trans);

    if (mTextCache)
        mFont->renderTextCache(mTextCache.get());

    // Pop the clip early to allow the cursor to be drawn outside of the "text area".
    Renderer::popClipRect();

    // Draw cursor.
    glm::vec2 cursorPos;
    if (isMultiline()) {
        cursorPos = mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x, mCursor);
    }
    else {
        cursorPos = mFont->sizeText(mText.substr(0, mCursor));
        cursorPos[1] = 0;
    }

    float cursorHeight = mFont->getHeight() * 0.8f;

    if (!mEditing) {
        Renderer::drawRect(cursorPos.x, cursorPos.y + (mFont->getHeight() - cursorHeight) / 2.0f,
                           2.0f * Renderer::getScreenWidthModifier(), cursorHeight, 0xC7C7C7FF,
                           0xC7C7C7FF);
    }

    if (mEditing && mBlinkTime < BLINKTIME / 2) {
        Renderer::drawRect(cursorPos.x, cursorPos.y + (mFont->getHeight() - cursorHeight) / 2.0f,
                           2.0f * Renderer::getScreenWidthModifier(), cursorHeight, 0x777777FF,
                           0x777777FF);
    }
}

glm::vec2 TextEditComponent::getTextAreaPos() const
{
    return glm::vec2 {(TEXT_PADDING_HORIZ * Renderer::getScreenWidthModifier()) / 2.0f,
                      (TEXT_PADDING_VERT * Renderer::getScreenHeightModifier()) / 2.0f};
}

glm::vec2 TextEditComponent::getTextAreaSize() const
{
    return glm::vec2 {mSize.x - (TEXT_PADDING_HORIZ * Renderer::getScreenWidthModifier()),
                      mSize.y - (TEXT_PADDING_VERT * Renderer::getScreenHeightModifier())};
}

std::vector<HelpPrompt> TextEditComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mEditing) {
        prompts.push_back(HelpPrompt("lt", "first"));
        prompts.push_back(HelpPrompt("rt", "last"));
        prompts.push_back(HelpPrompt("left/right", "move cursor"));
        prompts.push_back(HelpPrompt("b", "back"));
    }
    else {
        prompts.push_back(HelpPrompt("a", "edit"));
    }
    return prompts;
}

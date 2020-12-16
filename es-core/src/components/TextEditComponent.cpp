//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  TextEditComponent.cpp
//
//  Component for editing text fields in menus.
//

#include "components/TextEditComponent.h"

#include "resources/Font.h"
#include "utils/StringUtil.h"

#define TEXT_PADDING_HORIZ 10
#define TEXT_PADDING_VERT 2

#define CURSOR_REPEAT_START_DELAY 500
#define CURSOR_REPEAT_SPEED 28 // Lower is faster.

TextEditComponent::TextEditComponent(
        Window* window)
        : GuiComponent(window),
        mBox(window, ":/graphics/textinput_ninepatch.png"),
        mFocused(false),
        mScrollOffset(0.0f, 0.0f),
        mCursor(0), mEditing(false),
        mFont(Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT)),
        mCursorRepeatDir(0)
{
    addChild(&mBox);
    onFocusLost();
    setSize(4096, mFont->getHeight() + TEXT_PADDING_VERT);
}

void TextEditComponent::onFocusGained()
{
    mFocused = true;
    mBox.setImagePath(":/graphics/textinput_ninepatch_active.png");
}

void TextEditComponent::onFocusLost()
{
    mFocused = false;
    mBox.setImagePath(":/graphics/textinput_ninepatch.png");
}

void TextEditComponent::onSizeChanged()
{
    mBox.fitTo(mSize, Vector3f::Zero(), Vector2f(-34, -32 - TEXT_PADDING_VERT));
    onTextChanged(); // Wrap point probably changed.
}

void TextEditComponent::setValue(const std::string& val)
{
    mText = val;
    mTextOrig = val;
    onTextChanged();
}

std::string TextEditComponent::getValue() const
{
    return mText;
}

void TextEditComponent::textInput(const std::string& text)
{
    if (mEditing) {
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

void TextEditComponent::startEditing()
{
    if (!isMultiline())
        setCursor(mText.size());
    SDL_StartTextInput();
    mEditing = true;
    updateHelpPrompts();
}

void TextEditComponent::stopEditing()
{
    SDL_StopTextInput();
    mEditing = false;
    updateHelpPrompts();
}

bool TextEditComponent::input(InputConfig* config, Input input)
{
    bool const cursor_left = (config->getDeviceId() != DEVICE_KEYBOARD &&
            config->isMappedLike("left", input)) || (config->getDeviceId() == DEVICE_KEYBOARD &&
            input.id == SDLK_LEFT);
    bool const cursor_right = (config->getDeviceId() != DEVICE_KEYBOARD &&
            config->isMappedLike("right", input)) || (config->getDeviceId() == DEVICE_KEYBOARD &&
            input.id == SDLK_RIGHT);
    bool const cursor_up = (config->getDeviceId() != DEVICE_KEYBOARD &&
            config->isMappedLike("up", input)) || (config->getDeviceId() == DEVICE_KEYBOARD &&
            input.id == SDLK_UP);
    bool const cursor_down = (config->getDeviceId() != DEVICE_KEYBOARD &&
            config->isMappedLike("down", input)) || (config->getDeviceId() == DEVICE_KEYBOARD &&
            input.id == SDLK_DOWN);
    bool const shoulder_left = (config->isMappedLike("leftshoulder", input));
    bool const shoulder_right = (config->isMappedLike("rightshoulder", input));
    bool const trigger_left = (config->isMappedLike("lefttrigger", input));
    bool const trigger_right = (config->isMappedLike("righttrigger", input));

    if (input.value == 0) {
        if (cursor_left || cursor_right || cursor_up || cursor_down ||
            shoulder_left || shoulder_right | trigger_left || trigger_right)
            mCursorRepeatDir = 0;

        return false;
    }

    if ((config->isMappedTo("a", input) || (config->getDeviceId() == DEVICE_KEYBOARD &&
            input.id == SDLK_RETURN)) && mFocused && !mEditing) {
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

        // Done editing (accept changes).
        if ((config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_ESCAPE) ||
                (config->getDeviceId() != DEVICE_KEYBOARD && (config->isMappedTo("a", input) ||
                config->isMappedTo("b", input)))) {
            mTextOrig = mText;
            stopEditing();
            return true;
        }
        else if (cursor_left || cursor_right) {
            mCursorRepeatDir = cursor_left ? -1 : 1;
            mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
            moveCursor(mCursorRepeatDir);
        }
        else if (cursor_up) {
            // TODO
        }
        else if (cursor_down) {
            // TODO
        }
        else if (shoulder_left || shoulder_right) {
            mCursorRepeatDir = shoulder_left ? -10 : 10;
            mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
            moveCursor(mCursorRepeatDir);
        }
        // Jump to beginning of text.
        else if (trigger_left) {
            setCursor(0);
        }
        // Jump to end of text.
        else if (trigger_right) {
            setCursor(mText.length());
        }
        else if (config->getDeviceId() != DEVICE_KEYBOARD && config->isMappedTo("y", input)) {
            textInput("\b");
        }
        else if (config->getDeviceId() == DEVICE_KEYBOARD) {
            switch (input.id) {
            case SDLK_HOME:
                setCursor(0);
                break;

            case SDLK_END:
                setCursor(std::string::npos);
                break;

            case SDLK_DELETE:
                if (mCursor < mText.length()) {
                    // Fake as Backspace one char to the right.
                    moveCursor(1);
                    textInput("\b");
                }
                break;
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
}

void TextEditComponent::updateCursorRepeat(int deltaTime)
{
    if (mCursorRepeatDir == 0)
        return;

    mCursorRepeatTimer += deltaTime;
    while (mCursorRepeatTimer >= CURSOR_REPEAT_SPEED) {
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
    std::string wrappedText = (isMultiline() ?
            mFont->wrapText(mText, getTextAreaSize().x()) : mText);
    mTextCache = std::unique_ptr<TextCache>
            (mFont->buildTextCache(wrappedText, 0, 0, 0x77777700 | getOpacity()));

    if (mCursor > static_cast<int>(mText.length()))
        mCursor = static_cast<unsigned int>(mText.length());
}

void TextEditComponent::onCursorChanged()
{
    if (isMultiline()) {
        Vector2f textSize = mFont->
                getWrappedTextCursorOffset(mText, getTextAreaSize().x(), mCursor);

        if (mScrollOffset.y() + getTextAreaSize().y() < textSize.y() +
                mFont->getHeight())  // Need to scroll down?
            mScrollOffset[1] = textSize.y() - getTextAreaSize().y() + mFont->getHeight();
        else if (mScrollOffset.y() > textSize.y())  // Need to scroll up?
            mScrollOffset[1] = textSize.y();
    }
    else {
        Vector2f cursorPos = mFont->sizeText(mText.substr(0, mCursor));

        if (mScrollOffset.x() + getTextAreaSize().x() < cursorPos.x())
            mScrollOffset[0] = cursorPos.x() - getTextAreaSize().x();
        else if (mScrollOffset.x() > cursorPos.x())
            mScrollOffset[0] = cursorPos.x();
    }
}

void TextEditComponent::render(const Transform4x4f& parentTrans)
{
    Transform4x4f trans = getTransform() * parentTrans;
    renderChildren(trans);

    // Text + cursor rendering.
    // Offset into our "text area" (padding).
    trans.translation() += Vector3f(getTextAreaPos().x(), getTextAreaPos().y(), 0);

    Vector2i clipPos(static_cast<int>(trans.translation().x()),
            static_cast<int>(trans.translation().y()));
    // Use "text area" size for clipping.
    Vector3f dimScaled = trans * Vector3f(getTextAreaSize().x(), getTextAreaSize().y(), 0);
    Vector2i clipDim(static_cast<int>((dimScaled.x()) - trans.translation().x()),
            static_cast<int>((dimScaled.y()) - trans.translation().y()));
    Renderer::pushClipRect(clipPos, clipDim);

    trans.translate(Vector3f(-mScrollOffset.x(), -mScrollOffset.y(), 0));
    Renderer::setMatrix(trans);

    if (mTextCache)
        mFont->renderTextCache(mTextCache.get());

    // Pop the clip early to allow the cursor to be drawn outside of the "text area".
    Renderer::popClipRect();

    // Draw cursor.
    if (mEditing) {
        Vector2f cursorPos;
        if (isMultiline()) {
            cursorPos = mFont->getWrappedTextCursorOffset(mText, getTextAreaSize().x(), mCursor);
        }
        else {
            cursorPos = mFont->sizeText(mText.substr(0, mCursor));
            cursorPos[1] = 0;
        }

        float cursorHeight = mFont->getHeight() * 0.8f;
        Renderer::drawRect(cursorPos.x(), cursorPos.y() + (mFont->getHeight() -
                cursorHeight) / 2, 2.0f, cursorHeight, 0x000000FF, 0x000000FF);
    }
}

bool TextEditComponent::isMultiline()
{
    return (getSize().y() > mFont->getHeight() * 1.25f);
}

Vector2f TextEditComponent::getTextAreaPos() const
{
    return Vector2f(TEXT_PADDING_HORIZ / 2.0f, TEXT_PADDING_VERT / 2.0f);
}

Vector2f TextEditComponent::getTextAreaSize() const
{
    return Vector2f(mSize.x() - TEXT_PADDING_HORIZ, mSize.y() - TEXT_PADDING_VERT);
}

std::vector<HelpPrompt> TextEditComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mEditing) {
        prompts.push_back(HelpPrompt("up/down/left/right", "move cursor"));
        prompts.push_back(HelpPrompt("y", "backspace"));
        prompts.push_back(HelpPrompt("a", "accept changes"));
        prompts.push_back(HelpPrompt("b", "accept changes"));
    }
    else {
        prompts.push_back(HelpPrompt("a", "edit"));
    }
    return prompts;
}

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

TextEditComponent::TextEditComponent()
    : mRenderer {Renderer::getInstance()}
    , mFocused {false}
    , mEditing {false}
    , mMaskInput {true}
    , mCursor {0}
    , mBlinkTime {0}
    , mCursorRepeatDir {0}
    , mScrollOffset {0.0f, 0.0f}
    , mCursorPos {0.0f, 0.0f}
    , mBox {":/graphics/textinput.svg"}
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
    if (mSize.x == 0.0f || mSize.y == 0.0f)
        return;

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
    if (mMaskInput)
        return;

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
    mMaskInput = false;
    mCursorRepeatDir = 0;
    updateHelpPrompts();
}

bool TextEditComponent::input(InputConfig* config, Input input)
{
    bool const cursorLeft {config->isMappedLike("left", input)};
    bool const cursorRight {config->isMappedLike("right", input)};
    bool const cursorUp {config->isMappedLike("up", input)};
    bool const cursorDown {config->isMappedLike("down", input)};
    bool const shoulderLeft {config->isMappedLike("leftshoulder", input)};
    bool const shoulderRight {config->isMappedLike("rightshoulder", input)};
    bool const triggerLeft {config->isMappedLike("lefttrigger", input)};
    bool const triggerRight {config->isMappedLike("righttrigger", input)};

    mMaskInput = true;

    if (cursorLeft || cursorRight || cursorUp || cursorDown || shoulderLeft ||
        shoulderRight | triggerLeft || triggerRight) {
        if (input.value == 0)
            mCursorRepeatDir = 0;
    }

    if (input.value == 0)
        return false;

    if ((config->isMappedTo("a", input) ||
         (config->getDeviceId() == DEVICE_KEYBOARD && input.id == SDLK_RETURN)) &&
        mFocused && !mEditing) {
        startEditing();
        return true;
    }

    if (mEditing) {
        if (config->getDeviceId() == DEVICE_KEYBOARD) {
            // Special handling for keyboard input as the "A" and "B" buttons are overridden.
            if (input.id == SDLK_RETURN || input.id == SDLK_KP_ENTER) {
                if (isMultiline()) {
                    const bool maskValue {mMaskInput};
                    mMaskInput = false;
                    textInput("\n");
                    mMaskInput = maskValue;
                }
                else {
                    stopEditing();
                }

                return true;
            }
            else if (input.id == SDLK_DELETE) {
                if (mCursor < static_cast<int>(mText.length())) {
                    // Fake as Backspace one char to the right.
                    mMaskInput = false;
                    moveCursor(1);
                    textInput("\b");
                }
                return true;
            }
            else if (input.id == SDLK_BACKSPACE) {
                mMaskInput = false;
                textInput("\b");
                return true;
            }
        }

        if (cursorLeft || cursorRight) {
            mBlinkTime = 0;
            mCursorRepeatDir = cursorLeft ? -1 : 1;
            mCursorRepeatTimer = -(CURSOR_REPEAT_START_DELAY - CURSOR_REPEAT_SPEED);
            moveCursor(mCursorRepeatDir);
            return false;
        }
        else if (cursorDown) {
            // Stop editing and let the button down event be captured by the parent component.
            stopEditing();
            return false;
        }
        else if (shoulderLeft) {
            mMaskInput = false;
            textInput("\b");
            return true;
        }
        else if (triggerLeft) {
            // Jump to beginning of text.
            mBlinkTime = 0;
            setCursor(0);
            return true;
        }
        else if (triggerRight) {
            // Jump to end of text.
            mBlinkTime = 0;
            setCursor(mText.length());
            return true;
        }

        // Consume all input when editing text.
        mMaskInput = false;
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
    mWrappedText =
        (isMultiline() ? mFont->wrapText(mText, getTextAreaSize().x, 0.0f, 1.5f, true) : mText);
    mTextCache = std::unique_ptr<TextCache>(mFont->buildTextCache(
        mWrappedText, 0.0f, 0.0f, 0x77777700 | static_cast<unsigned char>(mOpacity * 255.0f)));

    if (mCursor > static_cast<int>(mText.length()))
        mCursor = static_cast<int>(mText.length());
}

void TextEditComponent::onCursorChanged()
{
    if (isMultiline()) {
        mCursorPos = mFont->getWrappedTextCursorOffset(mWrappedText, mCursor);

        // Need to scroll down?
        if (mScrollOffset.y + getTextAreaSize().y < mCursorPos.y + mFont->getHeight())
            mScrollOffset.y = mCursorPos.y - getTextAreaSize().y + mFont->getHeight();
        // Need to scroll up?
        else if (mScrollOffset.y > mCursorPos.y)
            mScrollOffset.y = mCursorPos.y;
    }
    else {
        mCursorPos = mFont->sizeText(mText.substr(0, mCursor));
        mCursorPos.y = 0.0f;

        if (mScrollOffset.x + getTextAreaSize().x < mCursorPos.x)
            mScrollOffset.x = mCursorPos.x - getTextAreaSize().x;
        else if (mScrollOffset.x > mCursorPos.x)
            mScrollOffset.x = mCursorPos.x;
    }
}

void TextEditComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {getTransform() * parentTrans};
    renderChildren(trans);

    // Text + cursor rendering.
    // Offset into our "text area" (padding).
    trans =
        glm::translate(trans, glm::round(glm::vec3 {getTextAreaPos().x, getTextAreaPos().y, 0.0f}));

    glm::ivec2 clipPos {static_cast<int>(trans[3].x), static_cast<int>(trans[3].y)};
    // Use "text area" size for clipping.
    glm::vec3 dimScaled {};
    dimScaled.x = std::fabs(trans[3].x + getTextAreaSize().x);
    dimScaled.y = std::fabs(trans[3].y + getTextAreaSize().y);

    glm::ivec2 clipDim {static_cast<int>(dimScaled.x - trans[3].x),
                        static_cast<int>(dimScaled.y - trans[3].y)};
    mRenderer->pushClipRect(clipPos, clipDim);

    trans = glm::translate(trans, glm::round(glm::vec3 {-mScrollOffset.x, -mScrollOffset.y, 0.0f}));
    mRenderer->setMatrix(trans);

    if (mTextCache)
        mFont->renderTextCache(mTextCache.get());

    // Pop the clip early to allow the cursor to be drawn outside of the "text area".
    mRenderer->popClipRect();

    // Draw cursor.
    float cursorHeight {mFont->getHeight() * 0.8f};

    if (!mEditing) {
        mRenderer->drawRect(mCursorPos.x, mCursorPos.y + (mFont->getHeight() - cursorHeight) / 2.0f,
                            2.0f * Renderer::getScreenWidthModifier(), cursorHeight, 0xC7C7C7FF,
                            0xC7C7C7FF);
    }

    if (mEditing && mBlinkTime < BLINKTIME / 2) {
        mRenderer->drawRect(mCursorPos.x, mCursorPos.y + (mFont->getHeight() - cursorHeight) / 2.0f,
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

//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  TextEditComponent.cpp
//
//  Component for editing text fields.
//  TODO: Add support for editing shaped text.
//

#include "components/TextEditComponent.h"

#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

#if defined(__ANDROID__)
#include "Settings.h"
#endif

#define TEXT_PADDING_HORIZ 12.0f
#define TEXT_PADDING_VERT 2.0f

#define CURSOR_REPEAT_START_DELAY 500
#define CURSOR_REPEAT_SPEED 28 // Lower is faster.

#define BLINKTIME 1000

TextEditComponent::TextEditComponent(bool multiLine)
    : mRenderer {Renderer::getInstance()}
    , mFocused {false}
    , mEditing {false}
    , mMaskInput {true}
    , mMultiLine {multiLine}
    , mCursor {0}
    , mCursorShapedText {0}
    , mBlinkTime {0}
    , mCursorRepeatDir {0}
    , mScrollOffset {0.0f, 0.0f}
    , mCursorPos {0.0f, 0.0f}
    , mBox {":/graphics/textinput.svg"}
{
    mEditText = std::make_unique<TextComponent>("", Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT));
    mEditText->setNeedGlyphsPos(true);
    mEditText->setTextShaping(false);

    if (mMultiLine)
        mEditText->setAutoCalcExtent(glm::ivec2 {0, 1});
    else
        mEditText->setAutoCalcExtent(glm::ivec2 {1, 0});

    mBox.setSharpCorners(true);
    addChild(&mBox);

    onFocusLost();
}

TextEditComponent::~TextEditComponent()
{
    mEditText->setTextShaping(true);

    // Always disable text input when destroying this component.
    SDL_StopTextInput();
}

void TextEditComponent::onFocusGained()
{
    mFocused = true;
    mBox.setImagePath(":/graphics/textinput_focused.svg");
    mBox.setFrameColor(mMenuColorTextInputFrameFocused);
    startEditing();
}

void TextEditComponent::onFocusLost()
{
    mFocused = false;
    mBox.setImagePath(":/graphics/textinput.svg");
    mBox.setFrameColor(mMenuColorTextInputFrameUnfocused);
}

void TextEditComponent::onSizeChanged()
{
    if (mSize.x == 0.0f || mSize.y == 0.0f)
        return;

    mBox.fitTo(
        mSize, glm::vec3 {0.0f, 0.0f, 0.0f},
        glm::vec2 {-32.0f, -32.0f - (TEXT_PADDING_VERT * mRenderer->getScreenHeightModifier())});

    if (mMultiLine)
        mEditText->setSize(getTextAreaSize().x, 0.0f);

    onTextChanged(); // Wrap point probably changed.
}

void TextEditComponent::setText(const std::string& val, bool update)
{
    mText = val;

    if (update) {
        onTextChanged();
        onCursorChanged();
    }
}

void TextEditComponent::textInput(const std::string& text, const bool pasting)
{
#if !defined(__ANDROID__)
    if (mMaskInput && !pasting)
        return;
#endif

    // Allow pasting up to a reasonable max clipboard size.
    if (pasting && text.length() > (mMultiLine ? 16384 : 300))
        return;

    if (mEditing) {
        mBlinkTime = 0;
        mCursorRepeatDir = 0;
        if (text[0] == '\b') {
            if (mCursor > 0) {
                size_t newCursor = Utils::String::prevCursor(mText, mCursor);
                mText.erase(mText.begin() + newCursor, mText.begin() + mCursor);
                mCursor = static_cast<unsigned int>(newCursor);
                --mCursorShapedText;
            }
        }
        else {
            mText.insert(mCursor,
                         (pasting && !mMultiLine ? Utils::String::replace(text, "\n", " ") : text));
            mCursor += static_cast<unsigned int>(
                (pasting && !mMultiLine ? Utils::String::replace(text, "\n", " ") : text).size());
            mCursorShapedText += static_cast<unsigned int>(Utils::String::unicodeLength(
                (pasting && !mMultiLine ? Utils::String::replace(text, "\n", " ") : text)));
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
    if (mEditing)
        return;

    SDL_StartTextInput();
    mEditing = true;
    updateHelpPrompts();
    mBlinkTime = BLINKTIME / 6;
}

void TextEditComponent::stopEditing()
{
    if (!mEditing)
        return;

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
                if (mMultiLine) {
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
#if defined(__ANDROID__)
            else if (!Settings::getInstance()->getBool("VirtualKeyboard") &&
                     input.id == SDLK_BACKSPACE) {
                return false;
            }
            else if (Settings::getInstance()->getBool("VirtualKeyboard") &&
                     input.id == SDLK_BACKSPACE) {
                mMaskInput = false;
#else
            else if (input.id == SDLK_BACKSPACE) {
                mMaskInput = false;
                textInput("\b");
#endif
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
        else if (cursorDown && isEditing()) {
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

        if (config->isMappedTo("b", input))
            stopEditing();

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
    mCursorShapedText += amt;

    if (mCursorShapedText < 0)
        mCursorShapedText = 0;
    else if (mCursorShapedText > static_cast<int>(Utils::String::unicodeLength(mText)))
        mCursorShapedText = static_cast<int>(Utils::String::unicodeLength(mText));

    onCursorChanged();
}

void TextEditComponent::setCursor(size_t pos)
{
    if (pos == std::string::npos) {
        mCursor = static_cast<unsigned int>(mText.length());
        mCursorShapedText = static_cast<int>(Utils::String::unicodeLength(mText));
    }
    else {
        mCursor = static_cast<int>(pos);
        mCursorShapedText = static_cast<int>(Utils::String::unicodeLength(mText.substr(0, pos)));
    }

    moveCursor(0);
}

void TextEditComponent::onTextChanged()
{
    mEditText->setText(mText);
    mEditText->setColor(mMenuColorKeyboardText | static_cast<unsigned char>(mOpacity * 255.0f));

    if (mCursor > static_cast<int>(mText.length()))
        mCursor = static_cast<int>(mText.length());

    if (mCursorShapedText > static_cast<int>(Utils::String::unicodeLength(mText)))
        mCursorShapedText = static_cast<int>(Utils::String::unicodeLength(mText));
}

void TextEditComponent::onCursorChanged()
{
    if (mMultiLine) {
        mCursorPos = mEditText->getGlyphPosition(mCursorShapedText);

        // Need to scroll down?
        if (mScrollOffset.y + getTextAreaSize().y < mCursorPos.y + getFont()->getHeight())
            mScrollOffset.y = mCursorPos.y - getTextAreaSize().y + getFont()->getHeight();
        // Need to scroll up?
        else if (mScrollOffset.y > mCursorPos.y)
            mScrollOffset.y = mCursorPos.y;
    }
    else {
        mCursorPos = mEditText->getGlyphPosition(mCursorShapedText);

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
    glm::vec3 dimScaled {0.0f, 0.0f, 0.0f};
    dimScaled.x = std::fabs(trans[3].x + getTextAreaSize().x);
    dimScaled.y = std::fabs(trans[3].y + getTextAreaSize().y);

    glm::ivec2 clipDim {static_cast<int>(dimScaled.x - trans[3].x),
                        static_cast<int>(dimScaled.y - trans[3].y)};
    mRenderer->pushClipRect(clipPos, clipDim);

    trans = glm::translate(trans, glm::round(glm::vec3 {-mScrollOffset.x, -mScrollOffset.y, 0.0f}));
    mRenderer->setMatrix(trans);
    mEditText->render(trans);

    // Pop the clip early to allow the cursor to be drawn outside of the "text area".
    mRenderer->popClipRect();

    // Draw cursor.
    const float textHeight {getFont()->getHeight()};
    const float cursorHeight {textHeight * 0.8f};

    if (!mEditing) {
        mRenderer->drawRect(mCursorPos.x, mCursorPos.y + (textHeight - cursorHeight) / 2.0f,
                            2.0f * mRenderer->getScreenResolutionModifier(), cursorHeight,
                            mMenuColorKeyboardCursorUnfocused, mMenuColorKeyboardCursorUnfocused);
    }

    if (mEditing && mBlinkTime < BLINKTIME / 2) {
        mRenderer->drawRect(mCursorPos.x, mCursorPos.y + (textHeight - cursorHeight) / 2.0f,
                            2.0f * mRenderer->getScreenResolutionModifier(), cursorHeight,
                            mMenuColorKeyboardCursorFocused, mMenuColorKeyboardCursorFocused);
    }
}

glm::vec2 TextEditComponent::getTextAreaPos() const
{
    return glm::vec2 {(TEXT_PADDING_HORIZ * mRenderer->getScreenResolutionModifier()) / 2.0f,
                      (TEXT_PADDING_VERT * mRenderer->getScreenResolutionModifier()) / 2.0f};
}

glm::vec2 TextEditComponent::getTextAreaSize() const
{
    return glm::vec2 {mSize.x - (TEXT_PADDING_HORIZ * mRenderer->getScreenResolutionModifier()),
                      mSize.y - (TEXT_PADDING_VERT * mRenderer->getScreenResolutionModifier())};
}

std::vector<HelpPrompt> TextEditComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mEditing) {
        prompts.push_back(HelpPrompt("lt", _("first")));
        prompts.push_back(HelpPrompt("rt", _("last")));
        prompts.push_back(HelpPrompt("left/right", _("move cursor")));
        prompts.push_back(HelpPrompt("b", _("back")));
    }
    else {
        prompts.push_back(HelpPrompt("a", _("edit")));
    }
    return prompts;
}

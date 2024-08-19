//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  ButtonComponent.cpp
//
//  Basic button, used as a GUI element and for the virtual keyboard buttons.
//

#include "components/ButtonComponent.h"

#include "Settings.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"

ButtonComponent::ButtonComponent(const std::string& text,
                                 const std::string& helpText,
                                 const std::function<void()>& func,
                                 bool upperCase,
                                 bool flatStyle)
    : mRenderer {Renderer::getInstance()}
    , mBox {":/graphics/button.svg"}
    , mPadding {0.0f, 0.0f, 0.0f, 0.0f}
    , mFocused {false}
    , mEnabled {true}
    , mFlatStyle {flatStyle}
    , mMinWidth {0.0f}
    , mTextColorFocused {mMenuColorButtonTextFocused}
    , mTextColorUnfocused {mMenuColorButtonTextUnfocused}
    , mFlatColorFocused {mMenuColorButtonFlatFocused}
    , mFlatColorUnfocused {mMenuColorButtonFlatUnfocused}

{
    if (mFlatStyle) {
        mButtonText = std::make_unique<TextComponent>("", Font::get(FONT_SIZE_MEDIUM), 0xFFFFFFFF,
                                                      ALIGN_CENTER);
    }
    else {
        mButtonText = std::make_unique<TextComponent>("DELETE", Font::get(FONT_SIZE_MEDIUM),
                                                      0xFFFFFFFF, ALIGN_CENTER);
        const glm::vec2 textCacheSize {mButtonText->getTextCache() == nullptr ?
                                           glm::vec2 {0.0f, 0.0f} :
                                           mButtonText->getTextCache()->metrics.size};
        mMinWidth = textCacheSize.x + (12.0f * mRenderer->getScreenResolutionModifier());
    }

    mBox.setSharpCorners(true);
    setPressedFunc(func);
    setText(text, helpText, upperCase);

    if (!mFlatStyle)
        updateImage();
}

void ButtonComponent::onSizeChanged()
{
    if (mFlatStyle)
        return;

    auto cornerSize = mBox.getCornerSize();

    mBox.fitTo(glm::vec2 {mSize.x - mPadding.x - mPadding.z, mSize.y - mPadding.y - mPadding.w},
               glm::vec3 {mPadding.x, mPadding.y, 0.0f},
               glm::vec2 {-cornerSize.x * 2.0f, -cornerSize.y * 2.0f});
}

void ButtonComponent::onFocusGained()
{
    mFocused = true;
    if (!mFlatStyle)
        updateImage();
}

void ButtonComponent::onFocusLost()
{
    mFocused = false;
    if (!mFlatStyle)
        updateImage();
}

void ButtonComponent::setText(const std::string& text,
                              const std::string& helpText,
                              bool upperCase,
                              bool resize)
{
    mText = upperCase ? Utils::String::toUpper(text) : text;
    mHelpText = helpText;
    mButtonText->setText(mText);

    if (resize) {
        setSize(
            std::max(mButtonText->getSize().x + (12.0f * mRenderer->getScreenResolutionModifier()),
                     mMinWidth),
            mButtonText->getSize().y);
    }

    updateHelpPrompts();
}

void ButtonComponent::setEnabled(bool state)
{
    mEnabled = state;
    if (!mFlatStyle)
        updateImage();
}

void ButtonComponent::setPadding(const glm::vec4 padding)
{
    if (mPadding == padding)
        return;

    mPadding = padding;
    onSizeChanged();
}

bool ButtonComponent::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value != 0) {
        if (mPressedFunc && mEnabled)
            mPressedFunc();
        return true;
    }

    return GuiComponent::input(config, input);
}

void ButtonComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    if (mFlatStyle) {
        if (mFocused) {
            mRenderer->setMatrix(trans);
            mRenderer->drawRect(mPadding.x, mPadding.y, mSize.x - mPadding.x - mPadding.z,
                                mSize.y - mPadding.y - mPadding.w, mFlatColorFocused,
                                mFlatColorFocused);
        }
        else {
            mRenderer->setMatrix(trans);
            mRenderer->drawRect(mPadding.x, mPadding.y, mSize.x - mPadding.x - mPadding.z,
                                mSize.y - mPadding.y - mPadding.w, mFlatColorUnfocused,
                                mFlatColorUnfocused);
        }
    }
    else {
        mBox.render(trans);
    }

    const glm::vec3 centerOffset {(mSize.x - mButtonText->getSize().x) / 2.0f,
                                  (mSize.y - mButtonText->getSize().y) / 2.0f, 0.0f};
    trans = glm::translate(trans, glm::round(centerOffset));

    if (Settings::getInstance()->getBool("DebugText")) {
        mButtonText->setDebugRendering(false);
        mRenderer->drawRect(centerOffset.x, 0.0f, mButtonText->getSize().x, mSize.y, 0x00000033,
                            0x00000033);
        mRenderer->drawRect(mBox.getPosition().x, 0.0f, mBox.getSize().x, mSize.y, 0x0000FF33,
                            0x0000FF33);
    }

    mButtonText->setColor(getCurTextColor());
    mButtonText->render(trans);
}

std::vector<HelpPrompt> ButtonComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", mHelpText.empty() ? mText.c_str() : mHelpText.c_str()));
    return prompts;
}

unsigned int ButtonComponent::getCurTextColor() const
{
    if (!mFocused)
        return mTextColorUnfocused;
    else
        return mTextColorFocused;
}

void ButtonComponent::updateImage()
{
    if (!mEnabled || !mPressedFunc) {
        mBox.setImagePath(":/graphics/button_filled.svg");
        mBox.setFrameColor(0x770000FF);
        return;
    }

    mBox.setFrameColor(mMenuColorButtonFocused);
    mBox.setImagePath(mFocused ? ":/graphics/button_filled.svg" : ":/graphics/button.svg");
}

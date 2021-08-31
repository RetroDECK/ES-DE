//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ButtonComponent.cpp
//
//  Basic on/off button.
//

#include "components/ButtonComponent.h"

#include "Settings.h"
#include "resources/Font.h"
#include "utils/StringUtil.h"

ButtonComponent::ButtonComponent(Window* window,
                                 const std::string& text,
                                 const std::string& helpText,
                                 const std::function<void()>& func)
    : GuiComponent(window)
    , mBox(window, ":/graphics/button.svg")
    , mFont(Font::get(FONT_SIZE_MEDIUM))
    , mFocused(false)
    , mEnabled(true)
    , mTextColorFocused(0xFFFFFFFF)
    , mTextColorUnfocused(0x777777FF)
{
    setPressedFunc(func);
    setText(text, helpText);
    updateImage();
}

void ButtonComponent::onSizeChanged()
{
    // Fit to mBox.
    mBox.fitTo(mSize, glm::vec3{}, glm::vec2{-32.0f, -32.0f});
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

void ButtonComponent::setText(const std::string& text, const std::string& helpText)
{
    mText = Utils::String::toUpper(text);
    mHelpText = helpText;

    mTextCache = std::unique_ptr<TextCache>(mFont->buildTextCache(mText, 0, 0, getCurTextColor()));

    float minWidth = mFont->sizeText("DELETE").x + (12.0f * Renderer::getScreenWidthModifier());
    setSize(std::max(mTextCache->metrics.size.x + (12.0f * Renderer::getScreenWidthModifier()),
                     minWidth),
            mTextCache->metrics.size.y);

    updateHelpPrompts();
}

void ButtonComponent::onFocusGained()
{
    mFocused = true;
    updateImage();
}

void ButtonComponent::onFocusLost()
{
    mFocused = false;
    updateImage();
}

void ButtonComponent::setEnabled(bool state)
{
    mEnabled = state;
    updateImage();
}

void ButtonComponent::updateImage()
{
    if (!mEnabled || !mPressedFunc) {
        mBox.setImagePath(":/graphics/button_filled.svg");
        mBox.setCenterColor(0x770000FF);
        mBox.setEdgeColor(0x770000FF);
        return;
    }

    mBox.setCenterColor(0xFFFFFFFF);
    mBox.setEdgeColor(0xFFFFFFFF);
    mBox.setImagePath(mFocused ? ":/graphics/button_filled.svg" : ":/graphics/button.svg");
}

void ButtonComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans{parentTrans * getTransform()};

    mBox.render(trans);

    if (mTextCache) {
        glm::vec3 centerOffset{(mSize.x - mTextCache->metrics.size.x) / 2.0f,
                               (mSize.y - mTextCache->metrics.size.y) / 2.0f, 0.0f};
        trans = glm::translate(trans, centerOffset);

        if (Settings::getInstance()->getBool("DebugText")) {
            Renderer::drawRect(centerOffset.x, 0.0f, mTextCache->metrics.size.x, mSize.y,
                               0x00000033, 0x00000033);
            Renderer::drawRect(mBox.getPosition().x, 0.0f, mBox.getSize().x, mSize.y, 0x0000FF33,
                               0x0000FF33);
        }

        Renderer::setMatrix(trans);

        mTextCache->setColor(getCurTextColor());
        mFont->renderTextCache(mTextCache.get());
        trans = glm::translate(trans, -centerOffset);
    }

    renderChildren(trans);
}

unsigned int ButtonComponent::getCurTextColor() const
{
    if (!mFocused)
        return mTextColorUnfocused;
    else
        return mTextColorFocused;
}

std::vector<HelpPrompt> ButtonComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", mHelpText.empty() ? mText.c_str() : mHelpText.c_str()));
    return prompts;
}

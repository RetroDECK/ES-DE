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

ButtonComponent::ButtonComponent(const std::string& text,
                                 const std::string& helpText,
                                 const std::function<void()>& func,
                                 bool upperCase,
                                 bool flatStyle)
    : mRenderer {Renderer::getInstance()}
    , mBox {":/graphics/button.svg"}
    , mFont {Font::get(FONT_SIZE_MEDIUM)}
    , mPadding {0.0f, 0.0f, 0.0f, 0.0f}
    , mFocused {false}
    , mEnabled {true}
    , mFlatStyle {flatStyle}
    , mTextColorFocused {0xFFFFFFFF}
    , mTextColorUnfocused {0x777777FF}
    , mFlatColorFocused {0x878787FF}
    , mFlatColorUnfocused {0x60606025}

{
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

void ButtonComponent::setText(const std::string& text, const std::string& helpText, bool upperCase)
{
    mText = upperCase ? Utils::String::toUpper(text) : text;
    mHelpText = helpText;

    mTextCache =
        std::unique_ptr<TextCache>(mFont->buildTextCache(mText, 0.0f, 0.0f, getCurTextColor()));

    const float minWidth {mFont->sizeText("DELETE").x +
                          (12.0f * mRenderer->getScreenResolutionModifier())};
    setSize(
        std::max(mTextCache->metrics.size.x + (12.0f * mRenderer->getScreenResolutionModifier()),
                 minWidth),
        mTextCache->metrics.size.y);

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

    if (mTextCache) {
        glm::vec3 centerOffset {(mSize.x - mTextCache->metrics.size.x) / 2.0f,
                                (mSize.y - mTextCache->metrics.size.y) / 2.0f, 0.0f};
        trans = glm::translate(trans, glm::round(centerOffset));

        if (Settings::getInstance()->getBool("DebugText")) {
            mRenderer->drawRect(centerOffset.x, 0.0f, mTextCache->metrics.size.x, mSize.y,
                                0x00000033, 0x00000033);
            mRenderer->drawRect(mBox.getPosition().x, 0.0f, mBox.getSize().x, mSize.y, 0x0000FF33,
                                0x0000FF33);
        }

        mRenderer->setMatrix(trans);

        mTextCache->setColor(getCurTextColor());
        mFont->renderTextCache(mTextCache.get());
        trans = glm::translate(trans, glm::round(-centerOffset));
    }

    renderChildren(trans);
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
        mBox.setCenterColor(0x770000FF);
        mBox.setEdgeColor(0x770000FF);
        return;
    }

    mBox.setCenterColor(0xFFFFFFFF);
    mBox.setEdgeColor(0xFFFFFFFF);
    mBox.setImagePath(mFocused ? ":/graphics/button_filled.svg" : ":/graphics/button.svg");
}

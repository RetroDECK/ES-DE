//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SliderComponent.cpp
//
//  Slider to set value in a predefined range.
//

#include "components/SliderComponent.h"

#include "Window.h"
#include "resources/Font.h"

#define MOVE_REPEAT_DELAY 500
#define MOVE_REPEAT_RATE 40

SliderComponent::SliderComponent(float min, float max, float increment, const std::string& suffix)
    : mRenderer {Renderer::getInstance()}
    , mMin {min}
    , mMax {max}
    , mSingleIncrement {increment}
    , mMoveRate {0.0f}
    , mBarHeight {0.0f}
    , mSuffix {suffix}
{
    assert((min - max) != 0.0f);

    // Some sane default value.
    mValue = (max + min) / 2.0f;

    mKnob.setOrigin(0.5f, 0.5f);
    mKnob.setImage(":/graphics/slider_knob.svg");

    setSize(mWindow->peekGui()->getSize().x * 0.26f,
            Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
}

bool SliderComponent::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        if (config->isMappedLike("left", input)) {
            if (input.value)
                setValue(mValue - mSingleIncrement);

            mMoveRate = input.value ? -mSingleIncrement : 0.0f;
            mMoveAccumulator = -MOVE_REPEAT_DELAY;
            return true;
        }
        if (config->isMappedLike("right", input)) {
            if (input.value)
                setValue(mValue + mSingleIncrement);

            mMoveRate = input.value ? mSingleIncrement : 0.0f;
            mMoveAccumulator = -MOVE_REPEAT_DELAY;
            return true;
        }
    }
    else {
        mMoveRate = 0.0f;
    }

    return GuiComponent::input(config, input);
}

void SliderComponent::update(int deltaTime)
{
    if (mMoveRate != 0) {
        mMoveAccumulator += deltaTime;
        while (mMoveAccumulator >= MOVE_REPEAT_RATE) {
            setValue(mValue + mMoveRate);
            mMoveAccumulator -= MOVE_REPEAT_RATE;
        }
    }

    GuiComponent::update(deltaTime);
}

void SliderComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    mRenderer->setMatrix(trans);

    if (Settings::getInstance()->getBool("DebugText")) {
        mRenderer->drawRect(
            mSize.x - mTextCache->metrics.size.x, (mSize.y - mTextCache->metrics.size.y) / 2.0f,
            mTextCache->metrics.size.x, mTextCache->metrics.size.y, 0x0000FF33, 0x0000FF33);
        mRenderer->drawRect(mSize.x - mTextCache->metrics.size.x, 0.0f, mTextCache->metrics.size.x,
                            mSize.y, 0x00000033, 0x00000033);
    }

    float width {mSize.x - mKnob.getSize().x -
                 (mTextCache ?
                      mTextCache->metrics.size.x + (4.0f * mRenderer->getScreenWidthModifier()) :
                      0.0f)};

    // Render suffix.
    if (mTextCache)
        mFont->renderTextCache(mTextCache.get());

    // Render bar.
    mRenderer->drawRect(mKnob.getSize().x / 2.0f, mSize.y / 2.0f, width, mBarHeight, 0x777777FF,
                        0x777777FF);

    // Render knob.
    mKnob.render(trans);

    GuiComponent::renderChildren(trans);
}

void SliderComponent::setValue(float value)
{
    mValue = value;
    if (mValue < mMin)
        mValue = mMin;
    else if (mValue > mMax)
        mValue = mMax;

    onValueChanged();
}

void SliderComponent::onSizeChanged()
{
    if (!mSuffix.empty())
        mFont = Font::get(static_cast<int>(mSize.y), FONT_PATH_LIGHT);

    onValueChanged();
}

void SliderComponent::onValueChanged()
{
    // Update suffix textcache.
    if (mFont) {
        std::stringstream ss;
        ss << std::fixed;
        ss.precision(0);
        ss << mValue;
        ss << mSuffix;
        const std::string val {ss.str()};

        ss.str("");
        ss.clear();
        ss << std::fixed;
        ss.precision(0);
        ss << mMax;
        ss << mSuffix;
        const std::string max {ss.str()};

        glm::vec2 textSize {mFont->sizeText(max)};
        mTextCache = std::shared_ptr<TextCache>(mFont->buildTextCache(
            val, mSize.x - textSize.x, (mSize.y - textSize.y) / 2.0f, 0x777777FF));
        mTextCache->metrics.size.x = textSize.x; // Fudge the width.
    }

    mKnob.setResize(0.0f, mSize.y * 0.7f);

    float barLength {
        mSize.x - mKnob.getSize().x -
        (mTextCache ? mTextCache->metrics.size.x + (4.0f * mRenderer->getScreenWidthModifier()) :
                      0.0f)};

    int barHeight {0};
    if (mRenderer->getScreenWidth() > mRenderer->getScreenHeight()) {
        barHeight = static_cast<int>(std::round(2.0f * mRenderer->getScreenHeightModifier()));
    }
    else {
        barHeight =
            static_cast<int>(std::round(2.0f * std::round(mRenderer->getScreenWidthModifier())));
    }

    // For very low resolutions, make sure the bar height is not rounded to zero.
    if (barHeight == 0)
        barHeight = 1;

    // Resize the knob one pixel if necessary to keep the bar centered.
    if (barHeight % 2 == 0 && static_cast<int>(std::round(mKnob.getSize().y)) % 2 != 0) {
        mKnob.setResize(mKnob.getSize().x - 1.0f, mKnob.getSize().y - 1.0f);
        setSize(getSize().x, getSize().y - 1.0f);
    }
    else if (barHeight == 1 && static_cast<int>(std::round(mKnob.getSize().y)) % 2 == 0) {
        mKnob.setResize(mKnob.getSize().x - 1.0f, mKnob.getSize().y - 1);
        setSize(getSize().x, getSize().y - 1.0f);
    }

    mBarHeight = static_cast<float>(barHeight);
    const float val {(mValue - mMin) / (mMax - mMin)};
    // For smooth outer boundaries.
    // const float val {glm::smoothstep(mMin, mMax, mValue)};

    mKnob.setPosition(val * barLength + mKnob.getSize().x / 2.0f, mSize.y / 2.0f);
}

std::vector<HelpPrompt> SliderComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("left/right", "change value"));
    return prompts;
}

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SliderComponent.cpp
//
//  Slider to set value in a predefined range.
//

#include "components/SliderComponent.h"

#include "resources/Font.h"

#define MOVE_REPEAT_DELAY 500
#define MOVE_REPEAT_RATE 40

SliderComponent::SliderComponent(
    Window* window, float min, float max, float increment, const std::string& suffix)
    : GuiComponent(window)
    , mMin(min)
    , mMax(max)
    , mSingleIncrement(increment)
    , mMoveRate(0)
    , mKnob(window)
    , mSuffix(suffix)
{
    assert((min - max) != 0);

    // Some sane default value.
    mValue = (max + min) / 2.0f;

    mKnob.setOrigin(0.5f, 0.5f);
    mKnob.setImage(":/graphics/slider_knob.svg");

    setSize(Renderer::getScreenWidth() * 0.15f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
}

bool SliderComponent::input(InputConfig* config, Input input)
{
    if (input.value != 0) {
        if (config->isMappedLike("left", input)) {
            if (input.value)
                setValue(mValue - mSingleIncrement);

            mMoveRate = input.value ? -mSingleIncrement : 0;
            mMoveAccumulator = -MOVE_REPEAT_DELAY;
            return true;
        }
        if (config->isMappedLike("right", input)) {
            if (input.value)
                setValue(mValue + mSingleIncrement);

            mMoveRate = input.value ? mSingleIncrement : 0;
            mMoveAccumulator = -MOVE_REPEAT_DELAY;
            return true;
        }
    }
    else {
        mMoveRate = 0;
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
    glm::mat4 trans{parentTrans * getTransform()};
    Renderer::setMatrix(trans);

    // Render suffix.
    if (mValueCache)
        mFont->renderTextCache(mValueCache.get());

    float width{mSize.x - mKnob.getSize().x -
                (mValueCache ?
                     mValueCache->metrics.size.x + (4.0f * Renderer::getScreenWidthModifier()) :
                     0.0f)};

    // Render line.
    const float lineWidth{2.0f * Renderer::getScreenHeightModifier()};
    Renderer::drawRect(mKnob.getSize().x / 2.0f, mSize.y / 2.0f - lineWidth / 2.0f, width,
                       lineWidth, 0x777777FF, 0x777777FF);

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

float SliderComponent::getValue() { return mValue; }

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
        const std::string val = ss.str();

        ss.str("");
        ss.clear();
        ss << std::fixed;
        ss.precision(0);
        ss << mMax;
        ss << mSuffix;
        const std::string max = ss.str();

        glm::vec2 textSize = mFont->sizeText(max);
        mValueCache = std::shared_ptr<TextCache>(mFont->buildTextCache(
            val, mSize.x - textSize.x, (mSize.y - textSize.y) / 2.0f, 0x777777FF));
        mValueCache->metrics.size.x = textSize.x; // Fudge the width.
    }

    // Update knob position/size.
    mKnob.setResize(0, mSize.y * 0.7f);
    float lineLength =
        mSize.x - mKnob.getSize().x -
        (mValueCache ? mValueCache->metrics.size.x + (4.0f * Renderer::getScreenWidthModifier()) :
                       0.0f);

    mKnob.setPosition(((mValue - mMin / 2.0f) / mMax) * lineLength + mKnob.getSize().x / 2.0f,
                      mSize.y / 2.0f);
}

std::vector<HelpPrompt> SliderComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("left/right", "change value"));
    return prompts;
}

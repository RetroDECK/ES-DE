//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  SliderComponent.cpp
//
//  Slider to set value in a predefined range.
//

#include "components/SliderComponent.h"

#include "Window.h"
#include "utils/LocalizationUtil.h"

#define MOVE_REPEAT_DELAY 500
#define MOVE_REPEAT_RATE 40

SliderComponent::SliderComponent(float min, float max, float increment, const std::string& suffix)
    : mRenderer {Renderer::getInstance()}
    , mMin {min}
    , mMax {max}
    , mValue {0.0f}
    , mSingleIncrement {increment}
    , mMoveRate {0.0f}
    , mBarLength {0.0f}
    , mBarHeight {0.0f}
    , mBarPosY {0.0f}
    , mMoveAccumulator {0}
    , mSliderTextSize {0.0f, 0.f}
    , mSuffix {suffix}
{
    assert((min - max) != 0.0f);

    mSliderText = std::make_unique<TextComponent>("", Font::get(FONT_SIZE_SMALL, FONT_PATH_LIGHT),
                                                  mMenuColorPrimary);
    setSize(mWindow->peekGui()->getSize().x * 0.26f,
            Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());

    // Some reasonable default value.
    mValue = (max + min) / 2.0f;

    mKnob.setResize(0.0f, std::round(mSize.y * 0.7f));
    mKnob.setOrigin(0.5f, 0.0f);
    mKnob.setImage(":/graphics/slider_knob.svg");
    mKnob.setColorShift(mMenuColorPrimary);

    mKnobDisabled.setResize(0.0f, std::round(mSize.y * 0.7f));
    mKnobDisabled.setOrigin(0.5f, 0.0f);
    mKnobDisabled.setImage(":/graphics/slider_knob.svg");
    mKnobDisabled.setColorShift(mMenuColorSliderKnobDisabled);
}

bool SliderComponent::input(InputConfig* config, Input input)
{
    // Ignore input if the component has been disabled.
    if (!mEnabled)
        return false;

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
        mSliderText->setDebugRendering(false);
        mRenderer->drawRect(mSize.x - mSliderTextSize.x, (mSize.y - mSliderTextSize.y) / 2.0f,
                            mSliderTextSize.x, mSliderTextSize.y, 0x0000FF33, 0x0000FF33);
        mRenderer->drawRect(mSize.x - mSliderTextSize.x, 0.0f, mSliderTextSize.x, mSize.y,
                            0x00000033, 0x00000033);
    }

    mSliderText->render(trans);
    mRenderer->setMatrix(trans);

    mRenderer->drawRect(
        mKnob.getSize().x / 2.0f, mBarPosY, mBarLength, mBarHeight,
        (mMenuColorPrimary & 0xFFFFFF00) | static_cast<unsigned int>(mOpacity * 255.0f),
        (mMenuColorPrimary & 0xFFFFFF00) | static_cast<unsigned int>(mOpacity * 255.0f));

    if (mOpacity > DISABLED_OPACITY)
        mKnob.render(trans);
    else
        mKnobDisabled.render(trans);

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
    mSliderText->setFont(Font::get(mSize.y, FONT_PATH_LIGHT));
    onValueChanged();
}

void SliderComponent::onValueChanged()
{
    {
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

        mSliderText->setText(val);
        mSliderTextSize = mSliderText->getFont()->sizeText(ss.str());
        mSliderText->setPosition(mSize.x - mSliderTextSize.x, (mSize.y - mSliderTextSize.y) / 2.0f);
    }

    mKnob.setResize(0.0f, std::round(mSize.y * 0.7f));

    if (mRenderer->getScreenWidth() > mRenderer->getScreenHeight())
        mBarHeight = std::round(2.0f * mRenderer->getScreenHeightModifier());
    else
        mBarHeight = std::round(2.0f * std::round(mRenderer->getScreenWidthModifier()));

    // For very low resolutions, make sure the bar height is not rounded to zero.
    if (mBarHeight < 1.0f)
        mBarHeight = 1.0f;

    // Always make both mSize and the knob odd or even for correct positioning.
    if (static_cast<int>(mSize.y) % 2 != static_cast<int>(mKnob.getSize().y) % 2) {
        mKnob.setResize(mKnob.getSize().x - 1.0f, mKnob.getSize().y - 1.0f);
        setSize(getSize().x, getSize().y - 1.0f);
    }

    mBarLength = mSize.x - mKnob.getSize().x -
                 (mSliderTextSize.x + (4.0f * mRenderer->getScreenWidthModifier()));

    if (static_cast<int>(mSize.y) % 2 != static_cast<int>(mBarHeight) % 2) {
        if (mBarHeight > 1.0f && mSize.y / mBarHeight < 5.0f)
            --mBarHeight;
        else
            ++mBarHeight;
    }

    const float posX {(mValue - mMin) / (mMax - mMin)};
    // For smooth outer boundaries.
    // const float posX {glm::smoothstep(mMin, mMax, mValue)};

    const float posY {(mSize.y - mKnob.getSize().y) / 2.0f};
    mKnob.setPosition(posX * mBarLength + mKnob.getSize().x / 2.0f, posY);

    mKnobDisabled.setResize(mKnob.getSize());
    mKnobDisabled.setPosition(mKnob.getPosition());

    mBarPosY = (mSize.y - mBarHeight) / 2.0f;

    if (mChangedValueCallback)
        mChangedValueCallback();
}

std::vector<HelpPrompt> SliderComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("left/right", _("change value")));
    return prompts;
}

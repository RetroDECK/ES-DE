//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SwitchComponent.cpp
//
//  Basic on/off switch used in menus.
//

#include "SwitchComponent.h"

#include "resources/Font.h"

SwitchComponent::SwitchComponent(
        Window* window,
        bool state)
        : GuiComponent(window),
        mImage(window),
        mState(state),
        mOriginalValue(state),
        mColorOriginalValue(DEFAULT_COLORSHIFT),
        mColorChangedValue(DEFAULT_COLORSHIFT)
{
    mImage.setImage(":/graphics/off.svg");
    mImage.setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
    mSize = mImage.getSize();
}

void SwitchComponent::onSizeChanged()
{
    mImage.setSize(mSize);
}

bool SwitchComponent::input(InputConfig* config, Input input)
{
    if (config->isMappedTo("a", input) && input.value) {
        // Ignore input if the component has been disabled.
        if (!mEnabled)
            return true;

        mState = !mState;
        onStateChanged();

        if (mToggleCallback)
            mToggleCallback();

        return true;
    }

    return false;
}

void SwitchComponent::render(const Transform4x4f& parentTrans)
{
    Transform4x4f trans = parentTrans * getTransform();
    mImage.render(trans);
    renderChildren(trans);
}

bool SwitchComponent::getState() const
{
    return mState;
}

void SwitchComponent::setState(bool state)
{
    mState = state;
    onStateChanged();
}

std::string SwitchComponent::getValue() const
{
    return mState ?  "true" : "false";
}

void SwitchComponent::setValue(const std::string& statestring)
{
    if (statestring == "true")
        mState = true;
    else
        mState = false;

    mOriginalValue = mState;
    onStateChanged();
}

unsigned char SwitchComponent::getOpacity() const
{
    return mImage.getOpacity();
}

void SwitchComponent::setOpacity(unsigned char opacity)
{
    mImage.setOpacity(opacity);
}

void SwitchComponent::setColorShift(unsigned int color)
{
    mImage.setColorShift(color);
}

unsigned int SwitchComponent::getColorShift() const
{
    return mImage.getColorShift();
}

void SwitchComponent::onStateChanged()
{
    mImage.setImage(mState ? ":/graphics/on.svg" : ":/graphics/off.svg");

    // Change the color of the switch to reflect the changes.
    if (mState == mOriginalValue)
        mImage.setColorShift(mColorOriginalValue);
    else
        mImage.setColorShift(mColorChangedValue);
}

std::vector<HelpPrompt> SwitchComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("a", "toggle"));
    return prompts;
}

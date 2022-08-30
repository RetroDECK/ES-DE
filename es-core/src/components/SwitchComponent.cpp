//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SwitchComponent.cpp
//
//  Basic on/off switch used in menus.
//

#include "SwitchComponent.h"

#include "resources/Font.h"

SwitchComponent::SwitchComponent(bool state)
    : mState {state}
    , mOriginalValue {state}
    , mColorOriginalValue {DEFAULT_COLORSHIFT}
    , mColorChangedValue {DEFAULT_COLORSHIFT}
{
    mImage.setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
    mImage.setImage(":/graphics/off.svg");
    mSize = mImage.getSize();
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

void SwitchComponent::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    mImage.render(trans);
    renderChildren(trans);
}

void SwitchComponent::setState(bool state)
{
    mState = state;
    onStateChanged();
}

std::string SwitchComponent::getValue() const { return mState ? "true" : "false"; }

void SwitchComponent::setValue(const std::string& statestring)
{
    if (statestring == "true")
        mState = true;
    else
        mState = false;

    mOriginalValue = mState;
    onStateChanged();
}

void SwitchComponent::onStateChanged()
{
    mImage.setResize(mSize);
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

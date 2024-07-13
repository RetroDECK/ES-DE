//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  SwitchComponent.cpp
//
//  Basic on/off switch used in menus.
//

#include "SwitchComponent.h"

#include "resources/Font.h"
#include "utils/LocalizationUtil.h"

SwitchComponent::SwitchComponent(bool state)
    : mState {state}
    , mOriginalValue {state}
    , mColorOriginalValue {mMenuColorPrimary}
    , mColorChangedValue {mMenuColorPrimary}
{
    mImage.setResize(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight());
    mImage.setImage(":/graphics/off.svg");
    mImage.setColorShift(mMenuColorPrimary);
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
    const float imageOpacity {mImage.getOpacity()};
    const float opacity {
        mState ? imageOpacity :
                 (mState == mOriginalValue ? 0.5f * imageOpacity : 0.7f * imageOpacity)};
    mImage.setOpacity(opacity);
    mImage.render(trans);
    mImage.setOpacity(imageOpacity);
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
    prompts.push_back(HelpPrompt("a", _("toggle")));
    return prompts;
}

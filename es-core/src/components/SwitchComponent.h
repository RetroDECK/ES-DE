//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SwitchComponent.h
//
//  Basic on/off switch used in menus.
//

#ifndef ES_CORE_COMPONENTS_SWITCH_COMPONENT_H
#define ES_CORE_COMPONENTS_SWITCH_COMPONENT_H

#include "components/ImageComponent.h"
#include "GuiComponent.h"

// A simple "on/off" switch.
class SwitchComponent : public GuiComponent
{
public:
    SwitchComponent(Window* window, bool state = false);

    bool input(InputConfig* config, Input input) override;
    void render(const Transform4x4f& parentTrans) override;
    void onSizeChanged() override;

    void setResize(float width, float height) override;

    bool getState() const;
    void setState(bool state);
    std::string getValue() const override;
    void setValue(const std::string& statestring) override;

    void setOriginalColor(unsigned int color) override { mColorOriginalValue = color; };
    void setChangedColor(unsigned int color) override { mColorChangedValue = color; };
    void setCallback(const std::function<void()>& callbackFunc) { mToggleCallback = callbackFunc; };

    unsigned char getOpacity() const override;
    void setOpacity(unsigned char opacity) override;
    // Multiply all pixels in the image by this color when rendering.
    void setColorShift(unsigned int color) override;

    unsigned int getColorShift() const override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void onStateChanged();

    ImageComponent mImage;
    bool mState;
    bool mOriginalValue;
    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;
    std::function<void()> mToggleCallback;
};

#endif // ES_CORE_COMPONENTS_SWITCH_COMPONENT_H

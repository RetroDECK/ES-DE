//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiDetectDevice.h
//
//  Detect input devices (keyboards, joysticks and gamepads).
//

#ifndef ES_CORE_GUIS_GUI_DETECT_DEVICE_H
#define ES_CORE_GUIS_GUI_DETECT_DEVICE_H

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"

class TextComponent;

class GuiDetectDevice : public GuiComponent
{
public:
    GuiDetectDevice(bool firstRun, bool forcedConfig, const std::function<void()>& doneCallback);

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void onSizeChanged() override;

private:
    bool mFirstRun;
    bool mForcedConfig;
    InputConfig* mHoldingConfig;
    int mHoldTime;

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mMsg1;
    std::shared_ptr<TextComponent> mMsg2;
    std::shared_ptr<TextComponent> mDeviceInfo;
    std::shared_ptr<TextComponent> mDeviceHeld;

    std::function<void()> mDoneCallback;
};

#endif // ES_CORE_GUIS_GUI_DETECT_DEVICE_H

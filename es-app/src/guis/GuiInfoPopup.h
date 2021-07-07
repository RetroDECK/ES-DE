//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiInfoPopup.h
//
//  Popup window used for user notifications.
//

#ifndef ES_APP_GUIS_GUI_INFO_POPUP_H
#define ES_APP_GUIS_GUI_INFO_POPUP_H

#include "GuiComponent.h"
#include "Window.h"

class ComponentGrid;
class NinePatchComponent;

class GuiInfoPopup : public GuiComponent, public Window::InfoPopup
{
public:
    GuiInfoPopup(Window* window, std::string message, int duration);
    ~GuiInfoPopup();

    void render(const Transform4x4f& parentTrans) override;
    void stop() override { mRunning = false; }

private:
    bool updateState();

    ComponentGrid* mGrid;
    NinePatchComponent* mFrame;

    std::string mMessage;
    int mDuration;
    int mAlpha;
    int mStartTime;
    bool mRunning;
};

#endif // ES_APP_GUIS_GUI_INFO_POPUP_H

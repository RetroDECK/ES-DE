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
#include "renderers/Renderer.h"

class ComponentGrid;
class NinePatchComponent;

class GuiInfoPopup : public GuiComponent
{
public:
    GuiInfoPopup(std::string message, int duration);
    ~GuiInfoPopup();

    void render(const glm::mat4& parentTrans);
    void stop() { mRunning = false; }
    bool isRunning() { return mRunning; }

private:
    bool updateState();

    Renderer* mRenderer;
    ComponentGrid* mGrid;
    NinePatchComponent* mFrame;

    std::string mMessage;
    int mDuration;
    float mAlpha;
    int mStartTime;
    bool mRunning;
};

#endif // ES_APP_GUIS_GUI_INFO_POPUP_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiLaunchScreen.h
//
//  Screen shown when launching a game.
//

#ifndef ES_APP_GUIS_GUI_LAUNCH_SCREEN_H
#define ES_APP_GUIS_GUI_LAUNCH_SCREEN_H

#include "GuiComponent.h"
#include "Window.h"
#include "components/ComponentGrid.h"
#include "components/ImageComponent.h"
#include "components/NinePatchComponent.h"
#include "components/TextComponent.h"

class FileData;

class GuiLaunchScreen : public Window::GuiLaunchScreen, GuiComponent
{
public:
    GuiLaunchScreen(Window* window);

    virtual ~GuiLaunchScreen();

    virtual void displayLaunchScreen(FileData* game) override;

    virtual void closeLaunchScreen() override;

    void onSizeChanged() override;

    virtual void update(int deltaTime) override;

    virtual void render(const glm::mat4& parentTrans) override;

private:
    Window* mWindow;
    NinePatchComponent mBackground;
    ComponentGrid* mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mGameName;
    std::shared_ptr<TextComponent> mSystemName;

    ImageComponent* mMarquee;
    std::string mImagePath;

    float mScaleUp;
};

#endif // ES_APP_GUIS_GUI_LAUNCH_SCREEN_H

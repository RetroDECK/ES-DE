//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiGeneralScreensaverOptions.h
//
//  User interface for the screensaver options.
//  Submenu to the GuiMenu main menu.
//

#ifndef ES_APP_GUIS_GUI_GENERAL_SCREENSAVER_OPTIONS_H
#define ES_APP_GUIS_GUI_GENERAL_SCREENSAVER_OPTIONS_H

#include "guis/GuiSettings.h"

class GuiGeneralScreensaverOptions : public GuiSettings
{
public:
    GuiGeneralScreensaverOptions(Window* window, const char* title);

private:
    void openSlideshowScreensaverOptions();
    void openVideoScreensaverOptions();
};

#endif // ES_APP_GUIS_GUI_GENERAL_SCREENSAVER_OPTIONS_H

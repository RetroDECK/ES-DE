//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScreensaverOptions.h
//
//  User interface for the screensaver options.
//  Submenu to the GuiMenu main menu.
//

#ifndef ES_APP_GUIS_GUI_SCREENSAVER_OPTIONS_H
#define ES_APP_GUIS_GUI_SCREENSAVER_OPTIONS_H

#include "guis/GuiSettings.h"

class GuiScreensaverOptions : public GuiSettings
{
public:
    GuiScreensaverOptions(Window* window, const char* title);

private:
    void openSlideshowScreensaverOptions();
    void openVideoScreensaverOptions();
};

#endif // ES_APP_GUIS_GUI_SCREENSAVER_OPTIONS_H

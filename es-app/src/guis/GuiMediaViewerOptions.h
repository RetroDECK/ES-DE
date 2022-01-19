//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMediaViewerOptions.h
//
//  User interface for the media viewer options.
//  Submenu to the GuiMenu main menu.
//

#ifndef ES_APP_GUIS_GUI_MEDIA_VIEWER_OPTIONS_H
#define ES_APP_GUIS_GUI_MEDIA_VIEWER_OPTIONS_H

#include "guis/GuiSettings.h"

class GuiMediaViewerOptions : public GuiSettings
{
public:
    GuiMediaViewerOptions(const std::string& title);
};

#endif // ES_APP_GUIS_GUI_MEDIA_VIEWER_OPTIONS_H

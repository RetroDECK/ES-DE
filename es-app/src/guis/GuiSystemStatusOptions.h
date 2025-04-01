//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiSystemStatusOptions.h
//
//  User interface for the system status options.
//  Submenu to the GuiMenu main menu.
//

#ifndef ES_APP_GUIS_GUI_SYSTEM_STATUS_OPTIONS_H
#define ES_APP_GUIS_GUI_SYSTEM_STATUS_OPTIONS_H

#include "guis/GuiSettings.h"

class GuiSystemStatusOptions : public GuiSettings
{
public:
    GuiSystemStatusOptions(const std::string& title);
};

#endif // ES_APP_GUIS_GUI_SYSTEM_STATUS_OPTIONS_H

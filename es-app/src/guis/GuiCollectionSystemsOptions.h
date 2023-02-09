//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiCollectionSystemsOptions.h
//
//  User interface for the game collection settings.
//  Submenu to the GuiMenu main menu.
//

#ifndef ES_APP_GUIS_GUI_COLLECTION_SYSTEM_OPTIONS_H
#define ES_APP_GUIS_GUI_COLLECTION_SYSTEM_OPTIONS_H

#include "GuiSettings.h"

template <typename T> class OptionListComponent;

class GuiCollectionSystemsOptions : public virtual GuiSettings
{
public:
    GuiCollectionSystemsOptions(std::string title);

private:
    void createCustomCollection(std::string inName);

    Renderer* mRenderer;
    std::shared_ptr<OptionListComponent<std::string>> mCollectionSystemsAuto;
    std::shared_ptr<OptionListComponent<std::string>> mCollectionSystemsCustom;

    bool mAddedCustomCollection;
    bool mDeletedCustomCollection;
};

#endif // ES_APP_GUIS_GUI_COLLECTION_SYSTEM_OPTIONS_H

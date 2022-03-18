//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiAlternativeEmulators.h
//
//  User interface to select between alternative emulators per system
//  based on configuration entries in es_systems.xml.
//

#ifndef ES_APP_GUIS_GUI_ALTERNATIVE_EMULATORS_H
#define ES_APP_GUIS_GUI_ALTERNATIVE_EMULATORS_H

#include "GuiComponent.h"
#include "guis/GuiSettings.h"

template <typename T> class OptionListComponent;

class GuiAlternativeEmulators : public GuiComponent
{
public:
    GuiAlternativeEmulators();

private:
    void updateMenu(const std::string& systemName, const std::string& label, bool defaultEmulator);
    void selectorWindow(SystemData* system);

    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

    MenuComponent mMenu;
    bool mHasSystems;

    std::map<std::string, std::shared_ptr<TextComponent>> mCommandRows;
    std::shared_ptr<OptionListComponent<std::string>> mCommandSelection;
};

#endif // ES_APP_GUIS_GUI_ALTERNATIVE_EMULATORS_H

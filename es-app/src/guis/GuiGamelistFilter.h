//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiGamelistFilter.h
//
//  User interface for the gamelist filters.
//  Triggered from the GuiGamelistOptions menu.
//  Actual filter logic is covered by FileFilterIndex.
//

#ifndef ES_APP_GUIS_GUI_GAME_LIST_FILTER_H
#define ES_APP_GUIS_GUI_GAME_LIST_FILTER_H

#include "FileFilterIndex.h"
#include "GuiComponent.h"
#include "components/MenuComponent.h"

template <typename T> class OptionListComponent;
class SystemData;

class GuiGamelistFilter : public GuiComponent
{
public:
    GuiGamelistFilter(Window* window,
                      SystemData* system,
                      std::function<void(bool)> filtersChangedCallback);

    ~GuiGamelistFilter();
    bool input(InputConfig* config, Input input) override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override;

private:
    void initializeMenu();
    void applyFilters();
    void resetAllFilters();
    void addFiltersToMenu();

    std::map<FilterIndexType, std::shared_ptr<OptionListComponent<std::string>>> mFilterOptions;
    std::vector<std::vector<std::string>> mInitialFilters;
    std::string mInitialTextFilter;

    MenuComponent mMenu;
    SystemData* mSystem;
    FileFilterIndex* mFilterIndex;
    std::shared_ptr<TextComponent> mTextFilterField;
    std::function<void(bool)> mFiltersChangedCallback;
    bool mFiltersChanged;
};

#endif // ES_APP_GUIS_GUI_GAME_LIST_FILTER_H

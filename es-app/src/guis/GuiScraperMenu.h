//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperMenu.h
//
//  Game media scraper, including settings as well as the scraping start button.
//  Submenu to the GuiMenu main menu.
//  Will call GuiScraperMulti to perform the actual scraping.
//

#ifndef ES_APP_GUIS_GUI_SCRAPER_MENU_H
#define ES_APP_GUIS_GUI_SCRAPER_MENU_H

#include "components/MenuComponent.h"
#include "guis/GuiSettings.h"
#include "scrapers/Scraper.h"
#include "views/ViewController.h"

class FileData;
class SwitchComponent;
class SystemData;

template <typename T> class OptionListComponent;
using GameFilterFunc = std::function<bool(SystemData*, FileData*)>;

class GuiScraperMenu : public GuiComponent
{
public:
    GuiScraperMenu(std::string title);
    ~GuiScraperMenu();

    bool input(InputConfig* config, Input input) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

private:
    void pressedStart();
    void start();

    void addEntry(const std::string&,
                  unsigned int color,
                  bool add_arrow,
                  const std::function<void()>& func);
    void openAccountOptions();
    void openContentOptions();
    void openMiximageOptions();
    void openOfflineGenerator(GuiSettings* settings);
    void openOtherOptions();

    std::queue<ScraperSearchParams> getSearches(std::vector<SystemData*> systems,
                                                GameFilterFunc selector);

    std::shared_ptr<OptionListComponent<std::string>> mScraper;
    std::shared_ptr<OptionListComponent<GameFilterFunc>> mFilters;
    std::shared_ptr<OptionListComponent<SystemData*>> mSystems;

    MenuComponent mMenu;
};

#endif // ES_APP_GUIS_GUI_SCRAPER_MENU_H

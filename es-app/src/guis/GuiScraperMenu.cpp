//
//  GuiScraperMenu.cpp
//
//  Game media scraper, including settings as well as the scraping start button.
//  Submenu to the GuiMenu main menu.
//  Will call GuiScraperMulti to perform the actual scraping.
//

#include "guis/GuiScraperMenu.h"

#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperMulti.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "SystemData.h"
#include "guis/GuiSettings.h"

GuiScraperMenu::GuiScraperMenu(Window* window) : GuiComponent(window),
        mMenu(window, "SCRAPER")
{
    // Scrape from.
    auto scraper_list = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "SCRAPE FROM", false);
    std::vector<std::string> scrapers = getScraperList();

    // Select either the first entry or the one read from the settings,
    // just in case the scraper from settings has vanished.
    for (auto it = scrapers.cbegin(); it != scrapers.cend(); it++)
        scraper_list->add(*it, *it, *it == Settings::getInstance()->getString("Scraper"));

    mMenu.addWithLabel("SCRAPE FROM", scraper_list);
    mMenu.addSaveFunc([scraper_list] { Settings::getInstance()->setString("Scraper",
            scraper_list->getSelected()); });

    // Search filters, getSearches() will generate a queue of games to scrape
    // based on the outcome of the checks below.
    mFilters = std::make_shared< OptionListComponent<GameFilterFunc>>
            (mWindow, getHelpStyle(), "SCRAPE THESE GAMES", false);
    mFilters->add("ALL GAMES", [](SystemData*, FileData*) -> bool { return true; }, true);
    mFilters->add("FAVORITE GAMES", [](SystemData*, FileData* g) -> bool {
        return g->getFavorite(); }, false);
    mFilters->add("NO METADATA", [](SystemData*, FileData* g) -> bool {
        return g->metadata.get("desc").empty(); }, false);
    mFilters->add("NO GAME IMAGE",
            [](SystemData*, FileData* g) -> bool {
        return g->getImagePath().empty(); }, false);
    mFilters->add("NO GAME VIDEO",
            [](SystemData*, FileData* g) -> bool {
        return g->getVideoPath().empty(); }, false);
    mMenu.addWithLabel("Filter", mFilters);

    // Add systems (all systems with an existing platform ID are listed).
    mSystems = std::make_shared< OptionListComponent<SystemData*>>
            (mWindow, getHelpStyle(), "SCRAPE THESE SYSTEMS", true);
    for (unsigned int i = 0; i < SystemData::sSystemVector.size(); i++) {
        if (!SystemData::sSystemVector[i]->hasPlatformId(PlatformIds::PLATFORM_IGNORE)) {
            mSystems->add(SystemData::sSystemVector[i]->getFullName(),
                    SystemData::sSystemVector[i],
                    !SystemData::sSystemVector[i]->getPlatformIds().empty());
            SystemData::sSystemVector[i]->getScrapeFlag() ?
                    mSystems->selectEntry(i) : mSystems->unselectEntry(i);
        }
    }
    mMenu.addWithLabel("Systems", mSystems);

    addEntry("CONTENT SETTINGS", 0x777777FF, true, [this] {
        // Always save the settings when entering this menu, so the options that are
        // not supported by TheGamesDB can be disabled.
        mMenu.save();
        openContentSettings();
    });
    addEntry("OTHER SETTINGS", 0x777777FF, true, [this] {
        // Always save the settings when entering this menu, so the options that are
        // not supported by TheGamesDB can be disabled.
        mMenu.save();
        openOtherSettings();
    });

    addChild(&mMenu);

    mMenu.addButton("START", "start", std::bind(&GuiScraperMenu::pressedStart, this));
    mMenu.addButton("BACK", "back", [&] { delete this; });

    setSize(mMenu.getSize());

    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2,
            Renderer::getScreenHeight() * 0.15f);
}

GuiScraperMenu::~GuiScraperMenu()
{
    // Save the scrape flags to the system settings so that they are
    // remembered throughout the program session.
    std::vector<SystemData*> sys = mSystems->getSelectedObjects();
    for (auto it = SystemData::sSystemVector.cbegin();
            it != SystemData::sSystemVector.cend(); it++) {
            (*it)->setScrapeFlag(false);
        for (auto it_sys = sys.cbegin(); it_sys != sys.cend(); it_sys++) {
            if ((*it)->getFullName() == (*it_sys)->getFullName())
                (*it)->setScrapeFlag(true);
        }
    }
}

void GuiScraperMenu::openContentSettings()
{
    auto s = new GuiSettings(mWindow, "SCRAPER CONTENT SETTINGS");

    // Scrape game names.
    auto scrape_gamename = std::make_shared<SwitchComponent>(mWindow);
    scrape_gamename->setState(Settings::getInstance()->getBool("ScrapeGameNames"));
    s->addWithLabel("SCRAPE GAME NAMES", scrape_gamename);
    s->addSaveFunc([scrape_gamename] { Settings::getInstance()->setBool("ScrapeGameNames",
            scrape_gamename->getState()); });

    // Scrape ratings.
    auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
    scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
    s->addWithLabel("SCRAPE RATINGS", scrape_ratings);
    s->addSaveFunc([scrape_ratings] { Settings::getInstance()->setBool("ScrapeRatings",
            scrape_ratings->getState()); });

    // Ratings are not supported by TheGamesDB, so disable the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape_ratings->setDisabled();
        scrape_ratings->setOpacity(DISABLED_OPACITY);
        // I'm sure there is a better way to find the text component...
        scrape_ratings->getParent()->getChild(
                scrape_ratings->getParent()->getChildCount()-2)->setOpacity(DISABLED_OPACITY);
    }

    // Scrape other metadata.
    auto scrape_metadata = std::make_shared<SwitchComponent>(mWindow);
    scrape_metadata->setState(Settings::getInstance()->getBool("ScrapeMetadata"));
    s->addWithLabel("SCRAPE OTHER METADATA", scrape_metadata);
    s->addSaveFunc([scrape_metadata] { Settings::getInstance()->setBool("ScrapeMetadata",
            scrape_metadata->getState()); });

    // Scrape videos.
    auto scrape_videos = std::make_shared<SwitchComponent>(mWindow);
    scrape_videos->setState(Settings::getInstance()->getBool("ScrapeVideos"));
    s->addWithLabel("SCRAPE VIDEOS", scrape_videos);
    s->addSaveFunc([scrape_videos] { Settings::getInstance()->setBool("ScrapeVideos",
            scrape_videos->getState()); });

    // Videos are not supported by TheGamesDB, so disable the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape_videos->setDisabled();
        scrape_videos->setOpacity(DISABLED_OPACITY);
        // I'm sure there is a better way to find the text component...
        scrape_videos->getParent()->getChild(
                scrape_videos->getParent()->getChildCount()-2)->setOpacity(DISABLED_OPACITY);
    }

    // Scrape screenshots images.
    auto scrape_screenshots = std::make_shared<SwitchComponent>(mWindow);
    scrape_screenshots->setState(Settings::getInstance()->getBool("ScrapeScreenshots"));
    s->addWithLabel("SCRAPE SCREENSHOT IMAGES", scrape_screenshots);
    s->addSaveFunc([scrape_screenshots] { Settings::getInstance()->setBool("ScrapeScreenshots",
            scrape_screenshots->getState()); });

    // Scrape cover images.
    auto scrape_covers = std::make_shared<SwitchComponent>(mWindow);
    scrape_covers->setState(Settings::getInstance()->getBool("ScrapeCovers"));
    s->addWithLabel("SCRAPE BOX COVER IMAGES", scrape_covers);
    s->addSaveFunc([scrape_covers] { Settings::getInstance()->setBool("ScrapeCovers",
            scrape_covers->getState()); });

    // Scrape marquee images.
    auto scrape_marquees = std::make_shared<SwitchComponent>(mWindow);
    scrape_marquees->setState(Settings::getInstance()->getBool("ScrapeMarquees"));
    s->addWithLabel("SCRAPE MARQUEE (WHEEL) IMAGES", scrape_marquees);
    s->addSaveFunc([scrape_marquees] { Settings::getInstance()->setBool("ScrapeMarquees",
            scrape_marquees->getState()); });

    // Scrape 3D box images.
    auto scrape_3dboxes = std::make_shared<SwitchComponent>(mWindow);
    scrape_3dboxes->setState(Settings::getInstance()->getBool("Scrape3DBoxes"));
    s->addWithLabel("SCRAPE 3D BOX IMAGES", scrape_3dboxes);
    s->addSaveFunc([scrape_3dboxes] { Settings::getInstance()->setBool("Scrape3DBoxes",
            scrape_3dboxes->getState()); });

    // 3D box images are not supported by TheGamesDB, so disable the option if this scraper
    // is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape_3dboxes->setDisabled();
        scrape_3dboxes->setOpacity(DISABLED_OPACITY);
        // I'm sure there is a better way to find the text component...
        scrape_3dboxes->getParent()->getChild(
                scrape_3dboxes->getParent()->getChildCount()-2)->setOpacity(DISABLED_OPACITY);
    }

    mWindow->pushGui(s);
}

void GuiScraperMenu::openOtherSettings()
{
    auto s = new GuiSettings(mWindow, "OTHER SCRAPER SETTINGS");

    // Scraper region.
    auto scraper_region = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "REGION", false);
    std::vector<std::string> transitions_rg;
    transitions_rg.push_back("eu");
    transitions_rg.push_back("jp");
    transitions_rg.push_back("us");
    transitions_rg.push_back("ss");
    transitions_rg.push_back("wor");

    if (Settings::getInstance()->getString("ScraperRegion") != "") {
        if (std::find(transitions_rg.begin(), transitions_rg.end(),
                Settings::getInstance()->getString("ScraperRegion")) == transitions_rg.end()) {
            transitions_rg.push_back(Settings::getInstance()->getString("ScraperRegion"));
        }
    }
    for (auto it = transitions_rg.cbegin(); it != transitions_rg.cend(); it++)
        scraper_region->add(*it, *it, Settings::getInstance()->getString("ScraperRegion") == *it);
    s->addWithLabel("REGION", scraper_region);
    s->addSaveFunc([scraper_region] {
        Settings::getInstance()->setString("ScraperRegion", scraper_region->getSelected());
    });

    // Regions are not supported by TheGamesDB, so disable the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraper_region->setDisabled();
        scraper_region->setOpacity(DISABLED_OPACITY);
        // I'm sure there is a better way to find the text component...
        scraper_region->getParent()->getChild(
                scraper_region->getParent()->getChildCount()-2)->setOpacity(DISABLED_OPACITY);
    }

    // Scraper language.
    auto scraper_language = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "LANGUAGE", false);
    std::vector<std::string> transitions_lg;
    transitions_lg.push_back("en");
    transitions_lg.push_back("wor");

    if (Settings::getInstance()->getString("ScraperLanguage") != "") {
        if (std::find(transitions_lg.begin(), transitions_lg.end(),
                Settings::getInstance()->getString("ScraperLanguage")) == transitions_lg.end()) {
            transitions_lg.push_back(Settings::getInstance()->getString("ScraperLanguage"));
        }
    }
    for (auto it = transitions_lg.cbegin(); it != transitions_lg.cend(); it++)
        scraper_language->add(*it, *it,
                Settings::getInstance()->getString("ScraperLanguage") == *it);
    s->addWithLabel("LANGUAGE", scraper_language);
    s->addSaveFunc([scraper_language] {
        Settings::getInstance()->setString("ScraperLanguage", scraper_language->getSelected());
    });

    // Languages are not supported by TheGamesDB, so disable the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraper_language->setDisabled();
        scraper_language->setOpacity(DISABLED_OPACITY);
        // I'm sure there is a better way to find the text component...
        scraper_language->getParent()->getChild(
                scraper_language->getParent()->getChildCount()-2)->setOpacity(DISABLED_OPACITY);
    }

    // Overwrite files and data.
    auto scrape_overwrite = std::make_shared<SwitchComponent>(mWindow);
    scrape_overwrite->setState(Settings::getInstance()->getBool("ScraperOverwriteData"));
    s->addWithLabel("OVERWRITE FILES AND DATA", scrape_overwrite);
    s->addSaveFunc([scrape_overwrite] { Settings::getInstance()->setBool("ScraperOverwriteData",
            scrape_overwrite->getState()); });

    // Interactive scraping.
    auto scraper_interactive = std::make_shared<SwitchComponent>(mWindow);
    scraper_interactive->setState(Settings::getInstance()->getBool("ScraperInteractive"));
    s->addWithLabel("INTERACTIVE MODE", scraper_interactive);
    s->addSaveFunc([scraper_interactive] { Settings::getInstance()->setBool("ScraperInteractive",
            scraper_interactive->getState()); });

    // Semi-automatic scraping.
    auto scraper_semiautomatic = std::make_shared<SwitchComponent>(mWindow);
    scraper_semiautomatic->setState(Settings::getInstance()->getBool("ScraperSemiautomatic"));
    s->addWithLabel("AUTO-ACCEPT SINGLE GAME MATCHES", scraper_semiautomatic);
    s->addSaveFunc([scraper_semiautomatic] {
            Settings::getInstance()->setBool("ScraperSemiautomatic",
            scraper_semiautomatic->getState()); });

    // Respect the per-file multi-scraper exclusion flag.
    auto scraper_respect_exclusions = std::make_shared<SwitchComponent>(mWindow);
    scraper_respect_exclusions->setState(
                Settings::getInstance()->getBool("ScraperRespectExclusions"));
    s->addWithLabel("RESPECT PER-FILE SCRAPER EXCLUSIONS", scraper_respect_exclusions);
    s->addSaveFunc([scraper_respect_exclusions] {
            Settings::getInstance()->setBool("ScraperRespectExclusions",
            scraper_respect_exclusions->getState()); });

    mWindow->pushGui(s);
}

void GuiScraperMenu::pressedStart()
{
    // Save any GUI settings that may have been modified.
    mMenu.save();

    std::vector<SystemData*> sys = mSystems->getSelectedObjects();
    for (auto it = sys.cbegin(); it != sys.cend(); it++) {
        if ((*it)->getPlatformIds().empty()) {
            mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                Utils::String::toUpper("Warning: some of your selected systems do not "
                "have a platform set. Results may be even more inaccurate than "
                "usual!\nContinue anyway?"),
                "YES", std::bind(&GuiScraperMenu::start, this),
                "NO", nullptr));
            return;
        }
    }
    start();
}

void GuiScraperMenu::start()
{
    std::queue<ScraperSearchParams> searches = getSearches(mSystems->getSelectedObjects(),
            mFilters->getSelected());

    if (searches.empty()) {
        mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
            "NO GAMES TO SCRAPE"));
    }
    else {
        GuiScraperMulti* gsm = new GuiScraperMulti(mWindow, searches,
                Settings::getInstance()->getBool("ScraperInteractive"));
        mWindow->pushGui(gsm);
        delete this;
    }
}

std::queue<ScraperSearchParams> GuiScraperMenu::getSearches(
        std::vector<SystemData*> systems, GameFilterFunc selector)
{
    std::queue<ScraperSearchParams> queue;
    for (auto sys = systems.cbegin(); sys != systems.cend(); sys++) {
        std::vector<FileData*> games = (*sys)->getRootFolder()->getFilesRecursive(GAME);
        for (auto game = games.cbegin(); game != games.cend(); game++) {
            // Skip scraping the game if the multi-scraper exclusion flag has been
            // set for the file, and the flag to respect this value is also enabled.
            bool skipGame = ((*game)->getExcludeFromScraper() &&
                Settings::getInstance()->getBool("ScraperRespectExclusions") == true);
            if (!skipGame && selector((*sys), (*game))) {
                ScraperSearchParams search;
                search.game = *game;
                search.system = *sys;

                queue.push(search);
            }
        }
    }
    return queue;
}

void GuiScraperMenu::addEntry(const char* name, unsigned int color,
        bool add_arrow, const std::function<void()>& func)
{
    std::shared_ptr<Font> font = Font::get(FONT_SIZE_MEDIUM);

    // Populate the list.
    ComponentListRow row;
    row.addElement(std::make_shared<TextComponent>(mWindow, name, font, color), true);

    if (add_arrow) {
        std::shared_ptr<ImageComponent> bracket = makeArrow(mWindow);
        row.addElement(bracket, false);
    }

    row.makeAcceptInputHandler(func);
    mMenu.addRow(row);
}

bool GuiScraperMenu::input(InputConfig* config, Input input)
{
    if (GuiComponent::input(config, input))
        return true;

    if (config->isMappedTo("b", input) &&
            input.value != 0) {
        delete this;
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiScraperMenu::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    return prompts;
}

HelpStyle GuiScraperMenu::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}

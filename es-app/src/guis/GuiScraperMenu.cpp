//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiScraperMenu.cpp
//
//  Game media scraper, including settings as well as the scraping start button.
//  Submenu to the GuiMenu main menu.
//  Will call GuiScraperMulti to perform the actual scraping.
//

#include "guis/GuiScraperMenu.h"

#include "FileData.h"
#include "FileSorts.h"
#include "SystemData.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiOfflineGenerator.h"
#include "guis/GuiScraperMulti.h"
#include "views/ViewController.h"

GuiScraperMenu::GuiScraperMenu(Window* window, std::string title)
    : GuiComponent(window)
    , mMenu(window, title)
{
    // Scraper service.
    mScraper = std::make_shared<OptionListComponent<std::string>>(mWindow, getHelpStyle(),
                                                                  "SCRAPE FROM", false);
    std::vector<std::string> scrapers = getScraperList();
    // Select either the first entry or the one read from the settings,
    // just in case the scraper from settings has vanished.
    for (auto it = scrapers.cbegin(); it != scrapers.cend(); ++it)
        mScraper->add(*it, *it, *it == Settings::getInstance()->getString("Scraper"));
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the scraper to "screenscraper" in this case.
    if (mScraper->getSelectedObjects().size() == 0)
        mScraper->selectEntry(0);

    mMenu.addWithLabel("SCRAPE FROM", mScraper);

    // Search filters, getSearches() will generate a queue of games to scrape
    // based on the outcome of the checks below.
    mFilters = std::make_shared<OptionListComponent<GameFilterFunc>>(mWindow, getHelpStyle(),
                                                                     "SCRAPE THESE GAMES", false);
    mFilters->add(
        "ALL GAMES",
        [](SystemData*, FileData*) -> bool {
            // All games.
            return true;
        },
        false);
    mFilters->add(
        "FAVORITE GAMES",
        [](SystemData*, FileData* g) -> bool {
            // Favorite games.
            return g->getFavorite();
        },
        false);
    mFilters->add(
        "NO METADATA",
        [](SystemData*, FileData* g) -> bool {
            // No metadata.
            return g->metadata.get("desc").empty();
        },
        false);
    mFilters->add(
        "NO GAME IMAGE",
        [](SystemData*, FileData* g) -> bool {
            // No game image.
            return g->getImagePath().empty();
        },
        false);
    mFilters->add(
        "NO GAME VIDEO",
        [](SystemData*, FileData* g) -> bool {
            // No game video.
            return g->getVideoPath().empty();
        },
        false);
    mFilters->add(
        "FOLDERS ONLY",
        [](SystemData*, FileData* g) -> bool {
            // Folders only.
            return g->getType() == FOLDER;
        },
        false);

    mFilters->selectEntry(Settings::getInstance()->getInt("ScraperFilter"));
    mMenu.addWithLabel("SCRAPE THESE GAMES", mFilters);

    mMenu.addSaveFunc([this] {
        if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper")) {
            Settings::getInstance()->setString("Scraper", mScraper->getSelected());
            mMenu.setNeedsSaving();
        }
        // The filter setting is only retained during the program session i.e. it's not saved
        // to es_settings.xml.
        if (mFilters->getSelectedId() !=
            static_cast<unsigned int>(Settings::getInstance()->getInt("ScraperFilter")))
            Settings::getInstance()->setInt("ScraperFilter", mFilters->getSelectedId());
    });

    // Add systems (all systems with an existing platform ID are listed).
    mSystems = std::make_shared<OptionListComponent<SystemData*>>(mWindow, getHelpStyle(),
                                                                  "SCRAPE THESE SYSTEMS", true);
    for (unsigned int i = 0; i < SystemData::sSystemVector.size(); ++i) {
        if (!SystemData::sSystemVector[i]->hasPlatformId(PlatformIds::PLATFORM_IGNORE)) {
            mSystems->add(SystemData::sSystemVector[i]->getFullName(), SystemData::sSystemVector[i],
                          !SystemData::sSystemVector[i]->getPlatformIds().empty());
            SystemData::sSystemVector[i]->getScrapeFlag() ? mSystems->selectEntry(i) :
                                                            mSystems->unselectEntry(i);
        }
    }
    mMenu.addWithLabel("SCRAPE THESE SYSTEMS", mSystems);

    addEntry("ACCOUNT SETTINGS", 0x777777FF, true, [this] {
        // Open the account options menu.
        openAccountOptions();
    });
    addEntry("CONTENT SETTINGS", 0x777777FF, true, [this] {
        // If the scraper service has been changed before entering this menu, then save the
        // settings so that the specific options supported by the respective scrapers
        // can be enabled or disabled.
        if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper"))
            mMenu.save();
        openContentOptions();
    });
    addEntry("MIXIMAGE SETTINGS", 0x777777FF, true, [this] {
        // Open the miximage options menu.
        openMiximageOptions();
    });
    addEntry("OTHER SETTINGS", 0x777777FF, true, [this] {
        // If the scraper service has been changed before entering this menu, then save the
        // settings so that the specific options supported by the respective scrapers
        // can be enabled or disabled.
        if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper"))
            mMenu.save();
        openOtherOptions();
    });

    addChild(&mMenu);

    mMenu.addButton("START", "start", std::bind(&GuiScraperMenu::pressedStart, this));
    mMenu.addButton("BACK", "back", [&] { delete this; });

    setSize(mMenu.getSize());

    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f, Renderer::getScreenHeight() * 0.13f);
}

GuiScraperMenu::~GuiScraperMenu()
{
    // Save the scrape flags to the system settings so that they are
    // remembered throughout the program session.
    std::vector<SystemData*> sys = mSystems->getSelectedObjects();
    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        (*it)->setScrapeFlag(false);
        for (auto it_sys = sys.cbegin(); it_sys != sys.cend(); ++it_sys) {
            if ((*it)->getFullName() == (*it_sys)->getFullName())
                (*it)->setScrapeFlag(true);
        }
    }
}

void GuiScraperMenu::openAccountOptions()
{
    auto s = new GuiSettings(mWindow, "ACCOUNT SETTINGS");

    // Whether to use the ScreenScraper account when scraping.
    auto scraper_use_account_screenscraper = std::make_shared<SwitchComponent>(mWindow);
    scraper_use_account_screenscraper->setState(
        Settings::getInstance()->getBool("ScraperUseAccountScreenScraper"));
    s->addWithLabel("USE THIS ACCOUNT FOR SCREENSCRAPER", scraper_use_account_screenscraper);
    s->addSaveFunc([scraper_use_account_screenscraper, s] {
        if (scraper_use_account_screenscraper->getState() !=
            Settings::getInstance()->getBool("ScraperUseAccountScreenScraper")) {
            Settings::getInstance()->setBool("ScraperUseAccountScreenScraper",
                                             scraper_use_account_screenscraper->getState());
            s->setNeedsSaving();
        }
    });

    // ScreenScraper username.
    auto scraper_username_screenscraper = std::make_shared<TextComponent>(
        mWindow, "", Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_RIGHT);
    s->addEditableTextComponent("SCREENSCRAPER USERNAME", scraper_username_screenscraper,
                                Settings::getInstance()->getString("ScraperUsernameScreenScraper"));
    s->addSaveFunc([scraper_username_screenscraper, s] {
        if (scraper_username_screenscraper->getValue() !=
            Settings::getInstance()->getString("ScraperUsernameScreenScraper")) {
            Settings::getInstance()->setString("ScraperUsernameScreenScraper",
                                               scraper_username_screenscraper->getValue());
            s->setNeedsSaving();
        }
    });

    // ScreenScraper password.
    auto scraper_password_screenscraper = std::make_shared<TextComponent>(
        mWindow, "", Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_RIGHT);
    std::string passwordMasked;
    if (Settings::getInstance()->getString("ScraperPasswordScreenScraper") != "") {
        passwordMasked = "********";
        scraper_password_screenscraper->setHiddenValue(
            Settings::getInstance()->getString("ScraperPasswordScreenScraper"));
    }
    s->addEditableTextComponent("SCREENSCRAPER PASSWORD", scraper_password_screenscraper,
                                passwordMasked, "", true);
    s->addSaveFunc([scraper_password_screenscraper, s] {
        if (scraper_password_screenscraper->getHiddenValue() !=
            Settings::getInstance()->getString("ScraperPasswordScreenScraper")) {
            Settings::getInstance()->setString("ScraperPasswordScreenScraper",
                                               scraper_password_screenscraper->getHiddenValue());
            s->setNeedsSaving();
        }
    });

    mWindow->pushGui(s);
}

void GuiScraperMenu::openContentOptions()
{
    auto s = new GuiSettings(mWindow, "CONTENT SETTINGS");

    // Scrape game names.
    auto scrape_game_names = std::make_shared<SwitchComponent>(mWindow);
    scrape_game_names->setState(Settings::getInstance()->getBool("ScrapeGameNames"));
    s->addWithLabel("GAME NAMES", scrape_game_names);
    s->addSaveFunc([scrape_game_names, s] {
        if (scrape_game_names->getState() != Settings::getInstance()->getBool("ScrapeGameNames")) {
            Settings::getInstance()->setBool("ScrapeGameNames", scrape_game_names->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape ratings.
    auto scrape_ratings = std::make_shared<SwitchComponent>(mWindow);
    scrape_ratings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
    s->addWithLabel("RATINGS", scrape_ratings);
    s->addSaveFunc([scrape_ratings, s] {
        if (scrape_ratings->getState() != Settings::getInstance()->getBool("ScrapeRatings")) {
            Settings::getInstance()->setBool("ScrapeRatings", scrape_ratings->getState());
            s->setNeedsSaving();
        }
    });

    // Ratings are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape_ratings->setEnabled(false);
        scrape_ratings->setOpacity(DISABLED_OPACITY);
        scrape_ratings->getParent()
            ->getChild(scrape_ratings->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape controllers (arcade systems only).
    auto scrapeControllers = std::make_shared<SwitchComponent>(mWindow);
    scrapeControllers->setState(Settings::getInstance()->getBool("ScrapeControllers"));
    s->addWithLabel("CONTROLLERS (ARCADE SYSTEMS ONLY)", scrapeControllers);
    s->addSaveFunc([scrapeControllers, s] {
        if (scrapeControllers->getState() !=
            Settings::getInstance()->getBool("ScrapeControllers")) {
            Settings::getInstance()->setBool("ScrapeControllers", scrapeControllers->getState());
            s->setNeedsSaving();
        }
    });

    // Controllers are not supported by TheGamesDB, so gray out the option if this scraper is
    // selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrapeControllers->setEnabled(false);
        scrapeControllers->setOpacity(DISABLED_OPACITY);
        scrapeControllers->getParent()
            ->getChild(scrapeControllers->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape other metadata.
    auto scrape_metadata = std::make_shared<SwitchComponent>(mWindow);
    scrape_metadata->setState(Settings::getInstance()->getBool("ScrapeMetadata"));
    s->addWithLabel("OTHER METADATA", scrape_metadata);
    s->addSaveFunc([scrape_metadata, s] {
        if (scrape_metadata->getState() != Settings::getInstance()->getBool("ScrapeMetadata")) {
            Settings::getInstance()->setBool("ScrapeMetadata", scrape_metadata->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape videos.
    auto scrape_videos = std::make_shared<SwitchComponent>(mWindow);
    scrape_videos->setState(Settings::getInstance()->getBool("ScrapeVideos"));
    s->addWithLabel("VIDEOS", scrape_videos);
    s->addSaveFunc([scrape_videos, s] {
        if (scrape_videos->getState() != Settings::getInstance()->getBool("ScrapeVideos")) {
            Settings::getInstance()->setBool("ScrapeVideos", scrape_videos->getState());
            s->setNeedsSaving();
        }
    });

    // Videos are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape_videos->setEnabled(false);
        scrape_videos->setOpacity(DISABLED_OPACITY);
        scrape_videos->getParent()
            ->getChild(scrape_videos->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape screenshots images.
    auto scrape_screenshots = std::make_shared<SwitchComponent>(mWindow);
    scrape_screenshots->setState(Settings::getInstance()->getBool("ScrapeScreenshots"));
    s->addWithLabel("SCREENSHOT IMAGES", scrape_screenshots);
    s->addSaveFunc([scrape_screenshots, s] {
        if (scrape_screenshots->getState() !=
            Settings::getInstance()->getBool("ScrapeScreenshots")) {
            Settings::getInstance()->setBool("ScrapeScreenshots", scrape_screenshots->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape title screen images.
    auto scrapeTitleScreens = std::make_shared<SwitchComponent>(mWindow);
    scrapeTitleScreens->setState(Settings::getInstance()->getBool("ScrapeTitleScreens"));
    s->addWithLabel("TITLE SCREEN IMAGES", scrapeTitleScreens);
    s->addSaveFunc([scrapeTitleScreens, s] {
        if (scrapeTitleScreens->getState() !=
            Settings::getInstance()->getBool("ScrapeTitleScreens")) {
            Settings::getInstance()->setBool("ScrapeTitleScreens", scrapeTitleScreens->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape box cover images.
    auto scrape_covers = std::make_shared<SwitchComponent>(mWindow);
    scrape_covers->setState(Settings::getInstance()->getBool("ScrapeCovers"));
    s->addWithLabel("BOX COVER IMAGES", scrape_covers);
    s->addSaveFunc([scrape_covers, s] {
        if (scrape_covers->getState() != Settings::getInstance()->getBool("ScrapeCovers")) {
            Settings::getInstance()->setBool("ScrapeCovers", scrape_covers->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape box back cover images.
    auto scrapeBackCovers = std::make_shared<SwitchComponent>(mWindow);
    scrapeBackCovers->setState(Settings::getInstance()->getBool("ScrapeBackCovers"));
    s->addWithLabel("BOX BACK COVER IMAGES", scrapeBackCovers);
    s->addSaveFunc([scrapeBackCovers, s] {
        if (scrapeBackCovers->getState() != Settings::getInstance()->getBool("ScrapeBackCovers")) {
            Settings::getInstance()->setBool("ScrapeBackCovers", scrapeBackCovers->getState());
            s->setNeedsSaving();
        }
    });

    // Box back cover images are not supported by TheGamesDB, so gray out the option if this
    // scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrapeBackCovers->setEnabled(false);
        scrapeBackCovers->setOpacity(DISABLED_OPACITY);
        scrapeBackCovers->getParent()
            ->getChild(scrapeBackCovers->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape marquee images.
    auto scrape_marquees = std::make_shared<SwitchComponent>(mWindow);
    scrape_marquees->setState(Settings::getInstance()->getBool("ScrapeMarquees"));
    s->addWithLabel("MARQUEE (WHEEL) IMAGES", scrape_marquees);
    s->addSaveFunc([scrape_marquees, s] {
        if (scrape_marquees->getState() != Settings::getInstance()->getBool("ScrapeMarquees")) {
            Settings::getInstance()->setBool("ScrapeMarquees", scrape_marquees->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape 3D box images.
    auto scrape_3dboxes = std::make_shared<SwitchComponent>(mWindow);
    scrape_3dboxes->setState(Settings::getInstance()->getBool("Scrape3DBoxes"));
    s->addWithLabel("3D BOX IMAGES", scrape_3dboxes);
    s->addSaveFunc([scrape_3dboxes, s] {
        if (scrape_3dboxes->getState() != Settings::getInstance()->getBool("Scrape3DBoxes")) {
            Settings::getInstance()->setBool("Scrape3DBoxes", scrape_3dboxes->getState());
            s->setNeedsSaving();
        }
    });

    // 3D box images are not supported by TheGamesDB, so gray out the option if this scraper
    // is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape_3dboxes->setEnabled(false);
        scrape_3dboxes->setOpacity(DISABLED_OPACITY);
        scrape_3dboxes->getParent()
            ->getChild(scrape_3dboxes->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape physical media images.
    auto scrapePhysicalMedia = std::make_shared<SwitchComponent>(mWindow);
    scrapePhysicalMedia->setState(Settings::getInstance()->getBool("ScrapePhysicalMedia"));
    s->addWithLabel("PHYSICAL MEDIA IMAGES", scrapePhysicalMedia);
    s->addSaveFunc([scrapePhysicalMedia, s] {
        if (scrapePhysicalMedia->getState() !=
            Settings::getInstance()->getBool("ScrapePhysicalMedia")) {
            Settings::getInstance()->setBool("ScrapePhysicalMedia",
                                             scrapePhysicalMedia->getState());
            s->setNeedsSaving();
        }
    });

    // Physical media images are not supported by TheGamesDB, so gray out the option if this
    // scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrapePhysicalMedia->setEnabled(false);
        scrapePhysicalMedia->setOpacity(DISABLED_OPACITY);
        scrapePhysicalMedia->getParent()
            ->getChild(scrapePhysicalMedia->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    mWindow->pushGui(s);
}

void GuiScraperMenu::openMiximageOptions()
{
    auto s = new GuiSettings(mWindow, "MIXIMAGE SETTINGS");

    // Miximage resolution.
    auto miximage_resolution = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "MIXIMAGE RESOLUTION", false);
    std::string selectedResolution = Settings::getInstance()->getString("MiximageResolution");
    miximage_resolution->add("1280x960", "1280x960", selectedResolution == "1280x960");
    miximage_resolution->add("1920x1440", "1920x1440", selectedResolution == "1920x1440");
    miximage_resolution->add("640x480", "640x480", selectedResolution == "640x480");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the resolution to "1280x960" in this case.
    if (miximage_resolution->getSelectedObjects().size() == 0)
        miximage_resolution->selectEntry(0);
    s->addWithLabel("MIXIMAGE RESOLUTION", miximage_resolution);
    s->addSaveFunc([miximage_resolution, s] {
        if (miximage_resolution->getSelected() !=
            Settings::getInstance()->getString("MiximageResolution")) {
            Settings::getInstance()->setString("MiximageResolution",
                                               miximage_resolution->getSelected());
            s->setNeedsSaving();
        }
    });

    // Screenshot scaling method.
    auto miximage_scaling = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "SCREENSHOT SCALING", false);
    std::string selectedScaling = Settings::getInstance()->getString("MiximageScreenshotScaling");
    miximage_scaling->add("sharp", "sharp", selectedScaling == "sharp");
    miximage_scaling->add("smooth", "smooth", selectedScaling == "smooth");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the scaling method to "sharp" in this case.
    if (miximage_scaling->getSelectedObjects().size() == 0)
        miximage_scaling->selectEntry(0);
    s->addWithLabel("SCREENSHOT SCALING METHOD", miximage_scaling);
    s->addSaveFunc([miximage_scaling, s] {
        if (miximage_scaling->getSelected() !=
            Settings::getInstance()->getString("MiximageScreenshotScaling")) {
            Settings::getInstance()->setString("MiximageScreenshotScaling",
                                               miximage_scaling->getSelected());
            s->setNeedsSaving();
        }
    });

    // Box/cover size.
    auto miximageBoxSize = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "BOX SIZE", false);
    std::string selectedBoxSize = Settings::getInstance()->getString("MiximageBoxSize");
    miximageBoxSize->add("small", "small", selectedBoxSize == "small");
    miximageBoxSize->add("medium", "medium", selectedBoxSize == "medium");
    miximageBoxSize->add("large", "large", selectedBoxSize == "large");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the box size to "medium" in this case.
    if (miximageBoxSize->getSelectedObjects().size() == 0)
        miximageBoxSize->selectEntry(0);
    s->addWithLabel("BOX SIZE", miximageBoxSize);
    s->addSaveFunc([miximageBoxSize, s] {
        if (miximageBoxSize->getSelected() !=
            Settings::getInstance()->getString("MiximageBoxSize")) {
            Settings::getInstance()->setString("MiximageBoxSize", miximageBoxSize->getSelected());
            s->setNeedsSaving();
        }
    });

    // Physical media size.
    auto miximagePhysicalMediaSize = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "PHYSICAL MEDIA SIZE", false);
    std::string selectedPhysicalMediaSize =
        Settings::getInstance()->getString("MiximagePhysicalMediaSize");
    miximagePhysicalMediaSize->add("small", "small", selectedPhysicalMediaSize == "small");
    miximagePhysicalMediaSize->add("medium", "medium", selectedPhysicalMediaSize == "medium");
    miximagePhysicalMediaSize->add("large", "large", selectedPhysicalMediaSize == "large");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the physical media size to "medium" in this case.
    if (miximagePhysicalMediaSize->getSelectedObjects().size() == 0)
        miximagePhysicalMediaSize->selectEntry(0);
    s->addWithLabel("PHYSICAL MEDIA SIZE", miximagePhysicalMediaSize);
    s->addSaveFunc([miximagePhysicalMediaSize, s] {
        if (miximagePhysicalMediaSize->getSelected() !=
            Settings::getInstance()->getString("MiximagePhysicalMediaSize")) {
            Settings::getInstance()->setString("MiximagePhysicalMediaSize",
                                               miximagePhysicalMediaSize->getSelected());
            s->setNeedsSaving();
        }
    });

    // Whether to generate miximages when scraping.
    auto miximage_generate = std::make_shared<SwitchComponent>(mWindow);
    miximage_generate->setState(Settings::getInstance()->getBool("MiximageGenerate"));
    s->addWithLabel("GENERATE MIXIMAGES WHEN SCRAPING", miximage_generate);
    s->addSaveFunc([miximage_generate, s] {
        if (miximage_generate->getState() != Settings::getInstance()->getBool("MiximageGenerate")) {
            Settings::getInstance()->setBool("MiximageGenerate", miximage_generate->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to overwrite miximages (both for the scraper and offline generator).
    auto miximage_overwrite = std::make_shared<SwitchComponent>(mWindow);
    miximage_overwrite->setState(Settings::getInstance()->getBool("MiximageOverwrite"));
    s->addWithLabel("OVERWRITE MIXIMAGES (SCRAPER/OFFLINE GENERATOR)", miximage_overwrite);
    s->addSaveFunc([miximage_overwrite, s] {
        if (miximage_overwrite->getState() !=
            Settings::getInstance()->getBool("MiximageOverwrite")) {
            Settings::getInstance()->setBool("MiximageOverwrite", miximage_overwrite->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to remove letterboxes from the screenshots.
    auto remove_letterboxes = std::make_shared<SwitchComponent>(mWindow);
    remove_letterboxes->setState(Settings::getInstance()->getBool("MiximageRemoveLetterboxes"));
    s->addWithLabel("REMOVE LETTERBOXES FROM SCREENSHOTS", remove_letterboxes);
    s->addSaveFunc([remove_letterboxes, s] {
        if (remove_letterboxes->getState() !=
            Settings::getInstance()->getBool("MiximageRemoveLetterboxes")) {
            Settings::getInstance()->setBool("MiximageRemoveLetterboxes",
                                             remove_letterboxes->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to remove pillarboxes from the screenshots.
    auto remove_pillarboxes = std::make_shared<SwitchComponent>(mWindow);
    remove_pillarboxes->setState(Settings::getInstance()->getBool("MiximageRemovePillarboxes"));
    s->addWithLabel("REMOVE PILLARBOXES FROM SCREENSHOTS", remove_pillarboxes);
    s->addSaveFunc([remove_pillarboxes, s] {
        if (remove_pillarboxes->getState() !=
            Settings::getInstance()->getBool("MiximageRemovePillarboxes")) {
            Settings::getInstance()->setBool("MiximageRemovePillarboxes",
                                             remove_pillarboxes->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to rotate horizontally oriented boxes.
    auto miximageRotateBoxes = std::make_shared<SwitchComponent>(mWindow);
    miximageRotateBoxes->setState(
        Settings::getInstance()->getBool("MiximageRotateHorizontalBoxes"));
    s->addWithLabel("ROTATE HORIZONTALLY ORIENTED BOXES", miximageRotateBoxes);
    s->addSaveFunc([miximageRotateBoxes, s] {
        if (miximageRotateBoxes->getState() !=
            Settings::getInstance()->getBool("MiximageRotateHorizontalBoxes")) {
            Settings::getInstance()->setBool("MiximageRotateHorizontalBoxes",
                                             miximageRotateBoxes->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to include marquee images.
    auto miximage_marquee = std::make_shared<SwitchComponent>(mWindow);
    miximage_marquee->setState(Settings::getInstance()->getBool("MiximageIncludeMarquee"));
    s->addWithLabel("INCLUDE MARQUEE IMAGE", miximage_marquee);
    s->addSaveFunc([miximage_marquee, s] {
        if (miximage_marquee->getState() !=
            Settings::getInstance()->getBool("MiximageIncludeMarquee")) {
            Settings::getInstance()->setBool("MiximageIncludeMarquee",
                                             miximage_marquee->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to include box images.
    auto miximage_box = std::make_shared<SwitchComponent>(mWindow);
    miximage_box->setState(Settings::getInstance()->getBool("MiximageIncludeBox"));
    s->addWithLabel("INCLUDE BOX IMAGE", miximage_box);
    s->addSaveFunc([miximage_box, s] {
        if (miximage_box->getState() != Settings::getInstance()->getBool("MiximageIncludeBox")) {
            Settings::getInstance()->setBool("MiximageIncludeBox", miximage_box->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to use cover image if there is no 3D box image.
    auto miximage_cover_fallback = std::make_shared<SwitchComponent>(mWindow);
    miximage_cover_fallback->setState(Settings::getInstance()->getBool("MiximageCoverFallback"));
    s->addWithLabel("USE COVER IMAGE IF 3D BOX IS MISSING", miximage_cover_fallback);
    s->addSaveFunc([miximage_cover_fallback, s] {
        if (miximage_cover_fallback->getState() !=
            Settings::getInstance()->getBool("MiximageCoverFallback")) {
            Settings::getInstance()->setBool("MiximageCoverFallback",
                                             miximage_cover_fallback->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to include physical media images.
    auto miximagePhysicalMedia = std::make_shared<SwitchComponent>(mWindow);
    miximagePhysicalMedia->setState(
        Settings::getInstance()->getBool("MiximageIncludePhysicalMedia"));
    s->addWithLabel("INCLUDE PHYSICAL MEDIA IMAGE", miximagePhysicalMedia);
    s->addSaveFunc([miximagePhysicalMedia, s] {
        if (miximagePhysicalMedia->getState() !=
            Settings::getInstance()->getBool("MiximageIncludePhysicalMedia")) {
            Settings::getInstance()->setBool("MiximageIncludePhysicalMedia",
                                             miximagePhysicalMedia->getState());
            s->setNeedsSaving();
        }
    });

    // Miximage offline generator.
    ComponentListRow offline_generator_row;
    offline_generator_row.elements.clear();
    offline_generator_row.addElement(std::make_shared<TextComponent>(mWindow, "OFFLINE GENERATOR",
                                                                     Font::get(FONT_SIZE_MEDIUM),
                                                                     0x777777FF),
                                     true);
    offline_generator_row.addElement(makeArrow(mWindow), false);
    offline_generator_row.makeAcceptInputHandler(
        std::bind(&GuiScraperMenu::openOfflineGenerator, this, s));
    s->addRow(offline_generator_row);

    mWindow->pushGui(s);
}

void GuiScraperMenu::openOfflineGenerator(GuiSettings* settings)
{
    if (mSystems->getSelectedObjects().empty()) {
        mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                                       "THE OFFLINE GENERATOR USES THE SAME SYSTEM\n"
                                       "SELECTIONS AS THE SCRAPER, SO PLEASE SELECT\n"
                                       "AT LEAST ONE SYSTEM TO GENERATE IMAGES FOR"));
        return;
    }

    // Always save the settings before starting the generator, in case any of the
    // miximage settings were modified.
    settings->save();
    // Also unset the save flag so that a double saving does not take place when closing
    // the miximage options menu later on.
    settings->setNeedsSaving(false);

    // Build the queue of games to process.
    std::queue<FileData*> gameQueue;
    std::vector<SystemData*> systems = mSystems->getSelectedObjects();

    for (auto sys = systems.cbegin(); sys != systems.cend(); ++sys) {
        std::vector<FileData*> games = (*sys)->getRootFolder()->getChildrenRecursive();

        // Sort the games by "filename, ascending".
        std::stable_sort(games.begin(), games.end(), FileSorts::SortTypes.at(0).comparisonFunction);

        for (FileData* game : games)
            gameQueue.push(game);
    }

    mWindow->pushGui(new GuiOfflineGenerator(mWindow, gameQueue));
}

void GuiScraperMenu::openOtherOptions()
{
    auto s = new GuiSettings(mWindow, "OTHER SETTINGS");

    // Scraper region.
    auto scraper_region = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "REGION", false);
    std::string selectedScraperRegion = Settings::getInstance()->getString("ScraperRegion");
    // clang-format off
    scraper_region->add("Europe", "eu",  selectedScraperRegion == "eu");
    scraper_region->add("Japan",  "jp",  selectedScraperRegion == "jp");
    scraper_region->add("USA",    "us",  selectedScraperRegion == "us");
    scraper_region->add("World",  "wor", selectedScraperRegion == "wor");
    // clang-format on
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the region to "Europe" in this case.
    if (scraper_region->getSelectedObjects().size() == 0)
        scraper_region->selectEntry(0);
    s->addWithLabel("REGION", scraper_region);
    s->addSaveFunc([scraper_region, s] {
        if (scraper_region->getSelected() != Settings::getInstance()->getString("ScraperRegion")) {
            Settings::getInstance()->setString("ScraperRegion", scraper_region->getSelected());
            s->setNeedsSaving();
        }
    });

    // Regions are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraper_region->setEnabled(false);
        scraper_region->setOpacity(DISABLED_OPACITY);
        scraper_region->getParent()
            ->getChild(scraper_region->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scraper language.
    auto scraper_language = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "PREFERRED LANGUAGE", false);
    std::string selectedScraperLanguage = Settings::getInstance()->getString("ScraperLanguage");
    // clang-format off
    scraper_language->add("English",    "en", selectedScraperLanguage == "en");
    scraper_language->add("Español",    "es", selectedScraperLanguage == "es");
    scraper_language->add("Português",  "pt", selectedScraperLanguage == "pt");
    scraper_language->add("Français",   "fr", selectedScraperLanguage == "fr");
    scraper_language->add("Deutsch",    "de", selectedScraperLanguage == "de");
    scraper_language->add("Italiano",   "it", selectedScraperLanguage == "it");
    scraper_language->add("Nederlands", "nl", selectedScraperLanguage == "nl");
    scraper_language->add("日本語",      "ja", selectedScraperLanguage == "ja");
    scraper_language->add("简体中文",    "zh", selectedScraperLanguage == "zh");
    scraper_language->add("한국어",      "ko", selectedScraperLanguage == "ko");
    scraper_language->add("Русский",    "ru", selectedScraperLanguage == "ru");
    scraper_language->add("Dansk",      "da", selectedScraperLanguage == "da");
    scraper_language->add("Suomi",      "fi", selectedScraperLanguage == "fi");
    scraper_language->add("Svenska",    "sv", selectedScraperLanguage == "sv");
    scraper_language->add("Magyar",     "hu", selectedScraperLanguage == "hu");
    scraper_language->add("Norsk",      "no", selectedScraperLanguage == "no");
    scraper_language->add("Polski",     "pl", selectedScraperLanguage == "pl");
    scraper_language->add("Čeština",    "cz", selectedScraperLanguage == "cz");
    scraper_language->add("Slovenčina", "sk", selectedScraperLanguage == "sk");
    scraper_language->add("Türkçe",     "tr", selectedScraperLanguage == "tr");
    // clang-format on
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the language to "English" in this case.
    if (scraper_language->getSelectedObjects().size() == 0)
        scraper_language->selectEntry(0);
    s->addWithLabel("PREFERRED LANGUAGE", scraper_language);
    s->addSaveFunc([scraper_language, s] {
        if (scraper_language->getSelected() !=
            Settings::getInstance()->getString("ScraperLanguage")) {
            Settings::getInstance()->setString("ScraperLanguage", scraper_language->getSelected());
            s->setNeedsSaving();
        }
    });

    // Languages are not supported by TheGamesDB, so gray out the option if this scraper is
    // selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraper_language->setEnabled(false);
        scraper_language->setOpacity(DISABLED_OPACITY);
        scraper_language->getParent()
            ->getChild(scraper_language->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Overwrite files and data.
    auto scraper_overwrite_data = std::make_shared<SwitchComponent>(mWindow);
    scraper_overwrite_data->setState(Settings::getInstance()->getBool("ScraperOverwriteData"));
    s->addWithLabel("OVERWRITE FILES AND DATA", scraper_overwrite_data);
    s->addSaveFunc([scraper_overwrite_data, s] {
        if (scraper_overwrite_data->getState() !=
            Settings::getInstance()->getBool("ScraperOverwriteData")) {
            Settings::getInstance()->setBool("ScraperOverwriteData",
                                             scraper_overwrite_data->getState());
            s->setNeedsSaving();
        }
    });

    // Halt scraping on invalid media files.
    auto scraper_halt_on_invalid_media = std::make_shared<SwitchComponent>(mWindow);
    scraper_halt_on_invalid_media->setState(
        Settings::getInstance()->getBool("ScraperHaltOnInvalidMedia"));
    s->addWithLabel("HALT ON INVALID MEDIA FILES", scraper_halt_on_invalid_media);
    s->addSaveFunc([scraper_halt_on_invalid_media, s] {
        if (scraper_halt_on_invalid_media->getState() !=
            Settings::getInstance()->getBool("ScraperHaltOnInvalidMedia")) {
            Settings::getInstance()->setBool("ScraperHaltOnInvalidMedia",
                                             scraper_halt_on_invalid_media->getState());
            s->setNeedsSaving();
        }
    });

    // Search using metadata names.
    auto scraper_search_metadata_name = std::make_shared<SwitchComponent>(mWindow);
    scraper_search_metadata_name->setState(
        Settings::getInstance()->getBool("ScraperSearchMetadataName"));
    s->addWithLabel("SEARCH USING METADATA NAMES", scraper_search_metadata_name);
    s->addSaveFunc([scraper_search_metadata_name, s] {
        if (scraper_search_metadata_name->getState() !=
            Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
            Settings::getInstance()->setBool("ScraperSearchMetadataName",
                                             scraper_search_metadata_name->getState());
            s->setNeedsSaving();
        }
    });

    // Interactive scraping.
    auto scraper_interactive = std::make_shared<SwitchComponent>(mWindow);
    scraper_interactive->setState(Settings::getInstance()->getBool("ScraperInteractive"));
    s->addWithLabel("INTERACTIVE MODE", scraper_interactive);
    s->addSaveFunc([scraper_interactive, s] {
        if (scraper_interactive->getState() !=
            Settings::getInstance()->getBool("ScraperInteractive")) {
            Settings::getInstance()->setBool("ScraperInteractive", scraper_interactive->getState());
            s->setNeedsSaving();
        }
    });

    // Semi-automatic scraping.
    auto scraper_semiautomatic = std::make_shared<SwitchComponent>(mWindow);
    scraper_semiautomatic->setState(Settings::getInstance()->getBool("ScraperSemiautomatic"));
    s->addWithLabel("AUTO-ACCEPT SINGLE GAME MATCHES", scraper_semiautomatic);
    s->addSaveFunc([scraper_semiautomatic, s] {
        if (scraper_semiautomatic->getState() !=
            Settings::getInstance()->getBool("ScraperSemiautomatic")) {
            Settings::getInstance()->setBool("ScraperSemiautomatic",
                                             scraper_semiautomatic->getState());
            s->setNeedsSaving();
        }
    });

    // If interactive mode is set to off, then gray out this option.
    if (!Settings::getInstance()->getBool("ScraperInteractive")) {
        scraper_semiautomatic->setEnabled(false);
        scraper_semiautomatic->setOpacity(DISABLED_OPACITY);
        scraper_semiautomatic->getParent()
            ->getChild(scraper_semiautomatic->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Respect the per-file multi-scraper exclusion flag.
    auto scraper_respect_exclusions = std::make_shared<SwitchComponent>(mWindow);
    scraper_respect_exclusions->setState(
        Settings::getInstance()->getBool("ScraperRespectExclusions"));
    s->addWithLabel("RESPECT PER-FILE SCRAPER EXCLUSIONS", scraper_respect_exclusions);
    s->addSaveFunc([scraper_respect_exclusions, s] {
        if (scraper_respect_exclusions->getState() !=
            Settings::getInstance()->getBool("ScraperRespectExclusions")) {
            Settings::getInstance()->setBool("ScraperRespectExclusions",
                                             scraper_respect_exclusions->getState());
            s->setNeedsSaving();
        }
    });

    // Exclude files recursively for excluded folders.
    auto scraper_exclude_recursively = std::make_shared<SwitchComponent>(mWindow);
    scraper_exclude_recursively->setState(
        Settings::getInstance()->getBool("ScraperExcludeRecursively"));
    s->addWithLabel("EXCLUDE FOLDERS RECURSIVELY", scraper_exclude_recursively);
    s->addSaveFunc([scraper_exclude_recursively, s] {
        if (scraper_exclude_recursively->getState() !=
            Settings::getInstance()->getBool("ScraperExcludeRecursively")) {
            Settings::getInstance()->setBool("ScraperExcludeRecursively",
                                             scraper_exclude_recursively->getState());
            s->setNeedsSaving();
        }
    });

    // If respecting excluded files is set to off, then gray out this option.
    if (!Settings::getInstance()->getBool("ScraperRespectExclusions")) {
        scraper_exclude_recursively->setEnabled(false);
        scraper_exclude_recursively->setOpacity(DISABLED_OPACITY);
        scraper_exclude_recursively->getParent()
            ->getChild(scraper_exclude_recursively->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Include actual folders when scraping.
    auto scraper_include_folders = std::make_shared<SwitchComponent>(mWindow);
    scraper_include_folders->setState(Settings::getInstance()->getBool("ScraperIncludeFolders"));
    s->addWithLabel("SCRAPE ACTUAL FOLDERS", scraper_include_folders);
    s->addSaveFunc([scraper_include_folders, s] {
        if (scraper_include_folders->getState() !=
            Settings::getInstance()->getBool("ScraperIncludeFolders")) {
            Settings::getInstance()->setBool("ScraperIncludeFolders",
                                             scraper_include_folders->getState());
            s->setNeedsSaving();
        }
    });

    // Retry search on peer verification errors (TLS/certificate issues).
    auto retry_peer_verification = std::make_shared<SwitchComponent>(mWindow);
    retry_peer_verification->setState(
        Settings::getInstance()->getBool("ScraperRetryPeerVerification"));
    s->addWithLabel("AUTO-RETRY ON PEER VERIFICATION ERRORS", retry_peer_verification);
    s->addSaveFunc([retry_peer_verification, s] {
        if (retry_peer_verification->getState() !=
            Settings::getInstance()->getBool("ScraperRetryPeerVerification")) {
            Settings::getInstance()->setBool("ScraperRetryPeerVerification",
                                             retry_peer_verification->getState());
            s->setNeedsSaving();
        }
    });

    // The TLS/certificate issue is not present for TheGamesDB, so gray out the option if this
    // scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        retry_peer_verification->setEnabled(false);
        retry_peer_verification->setOpacity(DISABLED_OPACITY);
        retry_peer_verification->getParent()
            ->getChild(retry_peer_verification->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Switch callbacks.
    auto interactiveToggleFunc = [scraper_semiautomatic]() {
        if (scraper_semiautomatic->getEnabled()) {
            scraper_semiautomatic->setEnabled(false);
            scraper_semiautomatic->setOpacity(DISABLED_OPACITY);
            scraper_semiautomatic->getParent()
                ->getChild(scraper_semiautomatic->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            scraper_semiautomatic->setEnabled(true);
            scraper_semiautomatic->setOpacity(255);
            scraper_semiautomatic->getParent()
                ->getChild(scraper_semiautomatic->getChildIndex() - 1)
                ->setOpacity(255);
        }
    };

    auto excludeRecursivelyToggleFunc = [scraper_exclude_recursively]() {
        if (scraper_exclude_recursively->getEnabled()) {
            scraper_exclude_recursively->setEnabled(false);
            scraper_exclude_recursively->setOpacity(DISABLED_OPACITY);
            scraper_exclude_recursively->getParent()
                ->getChild(scraper_exclude_recursively->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            scraper_exclude_recursively->setEnabled(true);
            scraper_exclude_recursively->setOpacity(255);
            scraper_exclude_recursively->getParent()
                ->getChild(scraper_exclude_recursively->getChildIndex() - 1)
                ->setOpacity(255);
        }
    };

    scraper_interactive->setCallback(interactiveToggleFunc);
    scraper_respect_exclusions->setCallback(excludeRecursivelyToggleFunc);

    mWindow->pushGui(s);
}

void GuiScraperMenu::pressedStart()
{
    // If the scraper service has been changed, then save the settings as otherwise the
    // wrong scraper would be used.
    if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper"))
        mMenu.save();

    std::vector<SystemData*> sys = mSystems->getSelectedObjects();
    for (auto it = sys.cbegin(); it != sys.cend(); ++it) {
        if ((*it)->getPlatformIds().empty()) {
            std::string warningString;
            if (sys.size() == 1) {
                warningString = "The selected system does not have a\n"
                                "platform set, results may be inaccurate\n"
                                "Continue anyway?";
            }
            else {
                warningString = "At least one of your selected\n"
                                "systems does not have a platform\n"
                                "set, results may be inaccurate\n"
                                "Continue anyway?";
            }
            mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                                           Utils::String::toUpper(warningString), "YES",
                                           std::bind(&GuiScraperMenu::start, this), "NO", nullptr));
            return;
        }
    }
    start();
}

void GuiScraperMenu::start()
{
    if (mSystems->getSelectedObjects().empty()) {
        mWindow->pushGui(
            new GuiMsgBox(mWindow, getHelpStyle(), "PLEASE SELECT AT LEAST ONE SYSTEM TO SCRAPE"));
        return;
    }

    bool contentToScrape = false;
    std::string scraperService = Settings::getInstance()->getString("Scraper");

    // Check if there is actually any type of content selected for scraping.
    do {
        if (Settings::getInstance()->getBool("ScrapeGameNames")) {
            contentToScrape = true;
            break;
        }
        if (scraperService == "screenscraper" &&
            Settings::getInstance()->getBool("ScrapeRatings")) {
            contentToScrape = true;
            break;
        }
        if (scraperService == "screenscraper" &&
            Settings::getInstance()->getBool("ScrapeControllers")) {
            contentToScrape = true;
            break;
        }
        if (Settings::getInstance()->getBool("ScrapeMetadata")) {
            contentToScrape = true;
            break;
        }
        if (scraperService == "screenscraper" && Settings::getInstance()->getBool("ScrapeVideos")) {
            contentToScrape = true;
            break;
        }
        if (Settings::getInstance()->getBool("ScrapeScreenshots")) {
            contentToScrape = true;
            break;
        }
        if (Settings::getInstance()->getBool("ScrapeTitleScreens")) {
            contentToScrape = true;
            break;
        }
        if (Settings::getInstance()->getBool("ScrapeCovers")) {
            contentToScrape = true;
            break;
        }
        if (scraperService == "screenscraper" &&
            Settings::getInstance()->getBool("ScrapeBackCovers")) {
            contentToScrape = true;
            break;
        }
        if (Settings::getInstance()->getBool("ScrapeMarquees")) {
            contentToScrape = true;
            break;
        }
        if (scraperService == "screenscraper" &&
            Settings::getInstance()->getBool("Scrape3DBoxes")) {
            contentToScrape = true;
            break;
        }
        if (scraperService == "screenscraper" &&
            Settings::getInstance()->getBool("ScrapePhysicalMedia")) {
            contentToScrape = true;
            break;
        }
    } while (0);

    if (!contentToScrape) {
        mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                                       "PLEASE SELECT AT LEAST ONE CONTENT TYPE TO SCRAPE"));
        return;
    }

    std::queue<ScraperSearchParams> searches =
        getSearches(mSystems->getSelectedObjects(), mFilters->getSelected());

    if (searches.empty()) {
        mWindow->pushGui(
            new GuiMsgBox(mWindow, getHelpStyle(), "ALL GAMES WERE FILTERED, NOTHING TO SCRAPE"));
    }
    else {
        GuiScraperMulti* gsm = new GuiScraperMulti(
            mWindow, searches, Settings::getInstance()->getBool("ScraperInteractive"));
        mWindow->pushGui(gsm);
        mMenu.setCursorToList();
        mMenu.setCursorToFirstListEntry();
    }
}

std::queue<ScraperSearchParams> GuiScraperMenu::getSearches(std::vector<SystemData*> systems,
                                                            GameFilterFunc selector)
{
    std::queue<ScraperSearchParams> queue;
    for (auto sys = systems.cbegin(); sys != systems.cend(); ++sys) {
        std::vector<FileData*> games = (*sys)->getRootFolder()->getScrapeFilesRecursive(
            Settings::getInstance()->getBool("ScraperIncludeFolders"),
            Settings::getInstance()->getBool("ScraperExcludeRecursively"),
            Settings::getInstance()->getBool("ScraperRespectExclusions"));
        for (auto game = games.cbegin(); game != games.cend(); ++game) {
            if (selector((*sys), (*game))) {
                ScraperSearchParams search;
                search.game = *game;
                search.system = *sys;

                queue.push(search);
            }
        }
    }
    return queue;
}

void GuiScraperMenu::addEntry(const std::string& name,
                              unsigned int color,
                              bool add_arrow,
                              const std::function<void()>& func)
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

    if (config->isMappedTo("y", input) && input.value != 0)
        pressedStart();

    if (config->isMappedTo("b", input) && input.value != 0) {
        delete this;
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiScraperMenu::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts = mMenu.getHelpPrompts();
    prompts.push_back(HelpPrompt("b", "back"));
    prompts.push_back(HelpPrompt("y", "start"));
    return prompts;
}

HelpStyle GuiScraperMenu::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}

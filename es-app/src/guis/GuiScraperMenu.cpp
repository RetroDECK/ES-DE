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

GuiScraperMenu::GuiScraperMenu(std::string title)
    : mMenu {title}
{
    // Scraper service.
    mScraper =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "SCRAPE FROM", false);
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
    mFilters = std::make_shared<OptionListComponent<GameFilterFunc>>(getHelpStyle(),
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
    mSystems = std::make_shared<OptionListComponent<SystemData*>>(getHelpStyle(),
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
    std::vector<SystemData*> sys {mSystems->getSelectedObjects()};
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
    auto s = new GuiSettings("ACCOUNT SETTINGS");

    // ScreenScraper username.
    auto scraperUsernameScreenScraper =
        std::make_shared<TextComponent>("", Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_RIGHT);
    s->addEditableTextComponent("SCREENSCRAPER USERNAME", scraperUsernameScreenScraper,
                                Settings::getInstance()->getString("ScraperUsernameScreenScraper"));
    s->addSaveFunc([scraperUsernameScreenScraper, s] {
        if (scraperUsernameScreenScraper->getValue() !=
            Settings::getInstance()->getString("ScraperUsernameScreenScraper")) {
            Settings::getInstance()->setString("ScraperUsernameScreenScraper",
                                               scraperUsernameScreenScraper->getValue());
            s->setNeedsSaving();
        }
    });

    // ScreenScraper password.
    auto scraperPasswordScreenScraper =
        std::make_shared<TextComponent>("", Font::get(FONT_SIZE_MEDIUM), 0x777777FF, ALIGN_RIGHT);
    std::string passwordMasked;
    if (Settings::getInstance()->getString("ScraperPasswordScreenScraper") != "") {
        passwordMasked = "********";
        scraperPasswordScreenScraper->setHiddenValue(
            Settings::getInstance()->getString("ScraperPasswordScreenScraper"));
    }
    s->addEditableTextComponent("SCREENSCRAPER PASSWORD", scraperPasswordScreenScraper,
                                passwordMasked, "", true);
    s->addSaveFunc([scraperPasswordScreenScraper, s] {
        if (scraperPasswordScreenScraper->getHiddenValue() !=
            Settings::getInstance()->getString("ScraperPasswordScreenScraper")) {
            Settings::getInstance()->setString("ScraperPasswordScreenScraper",
                                               scraperPasswordScreenScraper->getHiddenValue());
            s->setNeedsSaving();
        }
    });

    // Whether to use the ScreenScraper account when scraping.
    auto scraperUseAccountScreenScraper = std::make_shared<SwitchComponent>();
    scraperUseAccountScreenScraper->setState(
        Settings::getInstance()->getBool("ScraperUseAccountScreenScraper"));
    s->addWithLabel("USE THIS ACCOUNT FOR SCREENSCRAPER", scraperUseAccountScreenScraper);
    s->addSaveFunc([scraperUseAccountScreenScraper, s] {
        if (scraperUseAccountScreenScraper->getState() !=
            Settings::getInstance()->getBool("ScraperUseAccountScreenScraper")) {
            Settings::getInstance()->setBool("ScraperUseAccountScreenScraper",
                                             scraperUseAccountScreenScraper->getState());
            s->setNeedsSaving();
        }
    });

    mWindow->pushGui(s);
}

void GuiScraperMenu::openContentOptions()
{
    auto s = new GuiSettings("CONTENT SETTINGS");

    // Scrape game names.
    auto scrapeGameNames = std::make_shared<SwitchComponent>();
    scrapeGameNames->setState(Settings::getInstance()->getBool("ScrapeGameNames"));
    s->addWithLabel("GAME NAMES", scrapeGameNames);
    s->addSaveFunc([scrapeGameNames, s] {
        if (scrapeGameNames->getState() != Settings::getInstance()->getBool("ScrapeGameNames")) {
            Settings::getInstance()->setBool("ScrapeGameNames", scrapeGameNames->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape ratings.
    auto scrapeRatings = std::make_shared<SwitchComponent>();
    scrapeRatings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
    s->addWithLabel("RATINGS", scrapeRatings);
    s->addSaveFunc([scrapeRatings, s] {
        if (scrapeRatings->getState() != Settings::getInstance()->getBool("ScrapeRatings")) {
            Settings::getInstance()->setBool("ScrapeRatings", scrapeRatings->getState());
            s->setNeedsSaving();
        }
    });

    // Ratings are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrapeRatings->setEnabled(false);
        scrapeRatings->setOpacity(DISABLED_OPACITY);
        scrapeRatings->getParent()
            ->getChild(scrapeRatings->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape controllers (arcade systems only).
    auto scrapeControllers = std::make_shared<SwitchComponent>();
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
    auto scrapeMetadata = std::make_shared<SwitchComponent>();
    scrapeMetadata->setState(Settings::getInstance()->getBool("ScrapeMetadata"));
    s->addWithLabel("OTHER METADATA", scrapeMetadata);
    s->addSaveFunc([scrapeMetadata, s] {
        if (scrapeMetadata->getState() != Settings::getInstance()->getBool("ScrapeMetadata")) {
            Settings::getInstance()->setBool("ScrapeMetadata", scrapeMetadata->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape videos.
    auto scrapeVideos = std::make_shared<SwitchComponent>();
    scrapeVideos->setState(Settings::getInstance()->getBool("ScrapeVideos"));
    s->addWithLabel("VIDEOS", scrapeVideos);
    s->addSaveFunc([scrapeVideos, s] {
        if (scrapeVideos->getState() != Settings::getInstance()->getBool("ScrapeVideos")) {
            Settings::getInstance()->setBool("ScrapeVideos", scrapeVideos->getState());
            s->setNeedsSaving();
        }
    });

    // Videos are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrapeVideos->setEnabled(false);
        scrapeVideos->setOpacity(DISABLED_OPACITY);
        scrapeVideos->getParent()
            ->getChild(scrapeVideos->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape screenshots images.
    auto scrapeScreenshots = std::make_shared<SwitchComponent>();
    scrapeScreenshots->setState(Settings::getInstance()->getBool("ScrapeScreenshots"));
    s->addWithLabel("SCREENSHOT IMAGES", scrapeScreenshots);
    s->addSaveFunc([scrapeScreenshots, s] {
        if (scrapeScreenshots->getState() !=
            Settings::getInstance()->getBool("ScrapeScreenshots")) {
            Settings::getInstance()->setBool("ScrapeScreenshots", scrapeScreenshots->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape title screen images.
    auto scrapeTitleScreens = std::make_shared<SwitchComponent>();
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
    auto scrapeCovers = std::make_shared<SwitchComponent>();
    scrapeCovers->setState(Settings::getInstance()->getBool("ScrapeCovers"));
    s->addWithLabel("BOX COVER IMAGES", scrapeCovers);
    s->addSaveFunc([scrapeCovers, s] {
        if (scrapeCovers->getState() != Settings::getInstance()->getBool("ScrapeCovers")) {
            Settings::getInstance()->setBool("ScrapeCovers", scrapeCovers->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape box back cover images.
    auto scrapeBackCovers = std::make_shared<SwitchComponent>();
    scrapeBackCovers->setState(Settings::getInstance()->getBool("ScrapeBackCovers"));
    s->addWithLabel("BOX BACK COVER IMAGES", scrapeBackCovers);
    s->addSaveFunc([scrapeBackCovers, s] {
        if (scrapeBackCovers->getState() != Settings::getInstance()->getBool("ScrapeBackCovers")) {
            Settings::getInstance()->setBool("ScrapeBackCovers", scrapeBackCovers->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape marquee images.
    auto scrapeMarquees = std::make_shared<SwitchComponent>();
    scrapeMarquees->setState(Settings::getInstance()->getBool("ScrapeMarquees"));
    s->addWithLabel("MARQUEE (WHEEL) IMAGES", scrapeMarquees);
    s->addSaveFunc([scrapeMarquees, s] {
        if (scrapeMarquees->getState() != Settings::getInstance()->getBool("ScrapeMarquees")) {
            Settings::getInstance()->setBool("ScrapeMarquees", scrapeMarquees->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape 3D box images.
    auto scrape3dBoxes = std::make_shared<SwitchComponent>();
    scrape3dBoxes->setState(Settings::getInstance()->getBool("Scrape3DBoxes"));
    s->addWithLabel("3D BOX IMAGES", scrape3dBoxes);
    s->addSaveFunc([scrape3dBoxes, s] {
        if (scrape3dBoxes->getState() != Settings::getInstance()->getBool("Scrape3DBoxes")) {
            Settings::getInstance()->setBool("Scrape3DBoxes", scrape3dBoxes->getState());
            s->setNeedsSaving();
        }
    });

    // 3D box images are not supported by TheGamesDB, so gray out the option if this scraper
    // is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrape3dBoxes->setEnabled(false);
        scrape3dBoxes->setOpacity(DISABLED_OPACITY);
        scrape3dBoxes->getParent()
            ->getChild(scrape3dBoxes->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scrape physical media images.
    auto scrapePhysicalMedia = std::make_shared<SwitchComponent>();
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

    // Scrape fan art images.
    auto scrapeFanArt = std::make_shared<SwitchComponent>();
    scrapeFanArt->setState(Settings::getInstance()->getBool("ScrapeFanArt"));
    s->addWithLabel("FAN ART IMAGES", scrapeFanArt);
    s->addSaveFunc([scrapeFanArt, s] {
        if (scrapeFanArt->getState() != Settings::getInstance()->getBool("ScrapeFanArt")) {
            Settings::getInstance()->setBool("ScrapeFanArt", scrapeFanArt->getState());
            s->setNeedsSaving();
        }
    });

    mWindow->pushGui(s);
}

void GuiScraperMenu::openMiximageOptions()
{
    auto s = new GuiSettings("MIXIMAGE SETTINGS");

    // Miximage resolution.
    auto miximageResolution = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "MIXIMAGE RESOLUTION", false);
    std::string selectedResolution {Settings::getInstance()->getString("MiximageResolution")};
    miximageResolution->add("1280x960", "1280x960", selectedResolution == "1280x960");
    miximageResolution->add("1920x1440", "1920x1440", selectedResolution == "1920x1440");
    miximageResolution->add("640x480", "640x480", selectedResolution == "640x480");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the resolution to "1280x960" in this case.
    if (miximageResolution->getSelectedObjects().size() == 0)
        miximageResolution->selectEntry(0);
    s->addWithLabel("MIXIMAGE RESOLUTION", miximageResolution);
    s->addSaveFunc([miximageResolution, s] {
        if (miximageResolution->getSelected() !=
            Settings::getInstance()->getString("MiximageResolution")) {
            Settings::getInstance()->setString("MiximageResolution",
                                               miximageResolution->getSelected());
            s->setNeedsSaving();
        }
    });

    // Screenshot scaling method.
    auto miximageScaling = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "SCREENSHOT SCALING", false);
    std::string selectedScaling {Settings::getInstance()->getString("MiximageScreenshotScaling")};
    miximageScaling->add("sharp", "sharp", selectedScaling == "sharp");
    miximageScaling->add("smooth", "smooth", selectedScaling == "smooth");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the scaling method to "sharp" in this case.
    if (miximageScaling->getSelectedObjects().size() == 0)
        miximageScaling->selectEntry(0);
    s->addWithLabel("SCREENSHOT SCALING METHOD", miximageScaling);
    s->addSaveFunc([miximageScaling, s] {
        if (miximageScaling->getSelected() !=
            Settings::getInstance()->getString("MiximageScreenshotScaling")) {
            Settings::getInstance()->setString("MiximageScreenshotScaling",
                                               miximageScaling->getSelected());
            s->setNeedsSaving();
        }
    });

    // Box/cover size.
    auto miximageBoxSize =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "BOX SIZE", false);
    std::string selectedBoxSize {Settings::getInstance()->getString("MiximageBoxSize")};
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
        getHelpStyle(), "PHYSICAL MEDIA SIZE", false);
    std::string selectedPhysicalMediaSize {
        Settings::getInstance()->getString("MiximagePhysicalMediaSize")};
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
    auto miximageGenerate = std::make_shared<SwitchComponent>();
    miximageGenerate->setState(Settings::getInstance()->getBool("MiximageGenerate"));
    s->addWithLabel("GENERATE MIXIMAGES WHEN SCRAPING", miximageGenerate);
    s->addSaveFunc([miximageGenerate, s] {
        if (miximageGenerate->getState() != Settings::getInstance()->getBool("MiximageGenerate")) {
            Settings::getInstance()->setBool("MiximageGenerate", miximageGenerate->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to overwrite miximages (both for the scraper and offline generator).
    auto miximageOverwrite = std::make_shared<SwitchComponent>();
    miximageOverwrite->setState(Settings::getInstance()->getBool("MiximageOverwrite"));
    s->addWithLabel("OVERWRITE MIXIMAGES (SCRAPER/OFFLINE GENERATOR)", miximageOverwrite);
    s->addSaveFunc([miximageOverwrite, s] {
        if (miximageOverwrite->getState() !=
            Settings::getInstance()->getBool("MiximageOverwrite")) {
            Settings::getInstance()->setBool("MiximageOverwrite", miximageOverwrite->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to remove letterboxes from the screenshots.
    auto miximageRemoveLetterboxes = std::make_shared<SwitchComponent>();
    miximageRemoveLetterboxes->setState(
        Settings::getInstance()->getBool("MiximageRemoveLetterboxes"));
    s->addWithLabel("REMOVE LETTERBOXES FROM SCREENSHOTS", miximageRemoveLetterboxes);
    s->addSaveFunc([miximageRemoveLetterboxes, s] {
        if (miximageRemoveLetterboxes->getState() !=
            Settings::getInstance()->getBool("MiximageRemoveLetterboxes")) {
            Settings::getInstance()->setBool("MiximageRemoveLetterboxes",
                                             miximageRemoveLetterboxes->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to remove pillarboxes from the screenshots.
    auto miximageRemovePillarboxes = std::make_shared<SwitchComponent>();
    miximageRemovePillarboxes->setState(
        Settings::getInstance()->getBool("MiximageRemovePillarboxes"));
    s->addWithLabel("REMOVE PILLARBOXES FROM SCREENSHOTS", miximageRemovePillarboxes);
    s->addSaveFunc([miximageRemovePillarboxes, s] {
        if (miximageRemovePillarboxes->getState() !=
            Settings::getInstance()->getBool("MiximageRemovePillarboxes")) {
            Settings::getInstance()->setBool("MiximageRemovePillarboxes",
                                             miximageRemovePillarboxes->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to rotate horizontally oriented boxes.
    auto miximageRotateBoxes = std::make_shared<SwitchComponent>();
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
    auto miximageIncludeMarquee = std::make_shared<SwitchComponent>();
    miximageIncludeMarquee->setState(Settings::getInstance()->getBool("MiximageIncludeMarquee"));
    s->addWithLabel("INCLUDE MARQUEE IMAGE", miximageIncludeMarquee);
    s->addSaveFunc([miximageIncludeMarquee, s] {
        if (miximageIncludeMarquee->getState() !=
            Settings::getInstance()->getBool("MiximageIncludeMarquee")) {
            Settings::getInstance()->setBool("MiximageIncludeMarquee",
                                             miximageIncludeMarquee->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to include box images.
    auto miximageIncludeBox = std::make_shared<SwitchComponent>();
    miximageIncludeBox->setState(Settings::getInstance()->getBool("MiximageIncludeBox"));
    s->addWithLabel("INCLUDE BOX IMAGE", miximageIncludeBox);
    s->addSaveFunc([miximageIncludeBox, s] {
        if (miximageIncludeBox->getState() !=
            Settings::getInstance()->getBool("MiximageIncludeBox")) {
            Settings::getInstance()->setBool("MiximageIncludeBox", miximageIncludeBox->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to use cover image if there is no 3D box image.
    auto miximageCoverFallback = std::make_shared<SwitchComponent>();
    miximageCoverFallback->setState(Settings::getInstance()->getBool("MiximageCoverFallback"));
    s->addWithLabel("USE COVER IMAGE IF 3D BOX IS MISSING", miximageCoverFallback);
    s->addSaveFunc([miximageCoverFallback, s] {
        if (miximageCoverFallback->getState() !=
            Settings::getInstance()->getBool("MiximageCoverFallback")) {
            Settings::getInstance()->setBool("MiximageCoverFallback",
                                             miximageCoverFallback->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to include physical media images.
    auto miximageIncludePhysicalMedia = std::make_shared<SwitchComponent>();
    miximageIncludePhysicalMedia->setState(
        Settings::getInstance()->getBool("MiximageIncludePhysicalMedia"));
    s->addWithLabel("INCLUDE PHYSICAL MEDIA IMAGE", miximageIncludePhysicalMedia);
    s->addSaveFunc([miximageIncludePhysicalMedia, s] {
        if (miximageIncludePhysicalMedia->getState() !=
            Settings::getInstance()->getBool("MiximageIncludePhysicalMedia")) {
            Settings::getInstance()->setBool("MiximageIncludePhysicalMedia",
                                             miximageIncludePhysicalMedia->getState());
            s->setNeedsSaving();
        }
    });

    // Miximage offline generator.
    ComponentListRow offlineGeneratorRow;
    offlineGeneratorRow.elements.clear();
    offlineGeneratorRow.addElement(std::make_shared<TextComponent>("OFFLINE GENERATOR",
                                                                   Font::get(FONT_SIZE_MEDIUM),
                                                                   0x777777FF),
                                   true);
    offlineGeneratorRow.addElement(makeArrow(), false);
    offlineGeneratorRow.makeAcceptInputHandler(
        std::bind(&GuiScraperMenu::openOfflineGenerator, this, s));
    s->addRow(offlineGeneratorRow);

    mWindow->pushGui(s);
}

void GuiScraperMenu::openOfflineGenerator(GuiSettings* settings)
{
    if (mSystems->getSelectedObjects().empty()) {
        mWindow->pushGui(new GuiMsgBox(getHelpStyle(),
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
    std::vector<SystemData*> systems {mSystems->getSelectedObjects()};

    for (auto sys = systems.cbegin(); sys != systems.cend(); ++sys) {
        std::vector<FileData*> games {(*sys)->getRootFolder()->getChildrenRecursive()};

        // Sort the games by "filename, ascending".
        std::stable_sort(games.begin(), games.end(), FileSorts::SortTypes.at(0).comparisonFunction);

        for (FileData* game : games)
            gameQueue.push(game);
    }

    mWindow->pushGui(new GuiOfflineGenerator(gameQueue));
}

void GuiScraperMenu::openOtherOptions()
{
    auto s = new GuiSettings("OTHER SETTINGS");

    // Scraper region.
    auto scraperRegion =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "REGION", false);
    std::string selectedScraperRegion {Settings::getInstance()->getString("ScraperRegion")};
    // clang-format off
    scraperRegion->add("Europe", "eu",  selectedScraperRegion == "eu");
    scraperRegion->add("Japan",  "jp",  selectedScraperRegion == "jp");
    scraperRegion->add("USA",    "us",  selectedScraperRegion == "us");
    scraperRegion->add("World",  "wor", selectedScraperRegion == "wor");
    // clang-format on
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the region to "Europe" in this case.
    if (scraperRegion->getSelectedObjects().size() == 0)
        scraperRegion->selectEntry(0);
    s->addWithLabel("REGION", scraperRegion);
    s->addSaveFunc([scraperRegion, s] {
        if (scraperRegion->getSelected() != Settings::getInstance()->getString("ScraperRegion")) {
            Settings::getInstance()->setString("ScraperRegion", scraperRegion->getSelected());
            s->setNeedsSaving();
        }
    });

    // Regions are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraperRegion->setEnabled(false);
        scraperRegion->setOpacity(DISABLED_OPACITY);
        scraperRegion->getParent()
            ->getChild(scraperRegion->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Scraper language.
    auto scraperLanguage = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "PREFERRED LANGUAGE", false);
    std::string selectedScraperLanguage {Settings::getInstance()->getString("ScraperLanguage")};
    // clang-format off
    scraperLanguage->add("English",    "en", selectedScraperLanguage == "en");
    scraperLanguage->add("Español",    "es", selectedScraperLanguage == "es");
    scraperLanguage->add("Português",  "pt", selectedScraperLanguage == "pt");
    scraperLanguage->add("Français",   "fr", selectedScraperLanguage == "fr");
    scraperLanguage->add("Deutsch",    "de", selectedScraperLanguage == "de");
    scraperLanguage->add("Italiano",   "it", selectedScraperLanguage == "it");
    scraperLanguage->add("Nederlands", "nl", selectedScraperLanguage == "nl");
    scraperLanguage->add("日本語",      "ja", selectedScraperLanguage == "ja");
    scraperLanguage->add("简体中文",    "zh", selectedScraperLanguage == "zh");
    scraperLanguage->add("한국어",      "ko", selectedScraperLanguage == "ko");
    scraperLanguage->add("Русский",    "ru", selectedScraperLanguage == "ru");
    scraperLanguage->add("Dansk",      "da", selectedScraperLanguage == "da");
    scraperLanguage->add("Suomi",      "fi", selectedScraperLanguage == "fi");
    scraperLanguage->add("Svenska",    "sv", selectedScraperLanguage == "sv");
    scraperLanguage->add("Magyar",     "hu", selectedScraperLanguage == "hu");
    scraperLanguage->add("Norsk",      "no", selectedScraperLanguage == "no");
    scraperLanguage->add("Polski",     "pl", selectedScraperLanguage == "pl");
    scraperLanguage->add("Čeština",    "cz", selectedScraperLanguage == "cz");
    scraperLanguage->add("Slovenčina", "sk", selectedScraperLanguage == "sk");
    scraperLanguage->add("Türkçe",     "tr", selectedScraperLanguage == "tr");
    // clang-format on
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the language to "English" in this case.
    if (scraperLanguage->getSelectedObjects().size() == 0)
        scraperLanguage->selectEntry(0);
    s->addWithLabel("PREFERRED LANGUAGE", scraperLanguage);
    s->addSaveFunc([scraperLanguage, s] {
        if (scraperLanguage->getSelected() !=
            Settings::getInstance()->getString("ScraperLanguage")) {
            Settings::getInstance()->setString("ScraperLanguage", scraperLanguage->getSelected());
            s->setNeedsSaving();
        }
    });

    // Languages are not supported by TheGamesDB, so gray out the option if this scraper is
    // selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraperLanguage->setEnabled(false);
        scraperLanguage->setOpacity(DISABLED_OPACITY);
        scraperLanguage->getParent()
            ->getChild(scraperLanguage->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Overwrite files and data.
    auto scraperOverwriteData = std::make_shared<SwitchComponent>();
    scraperOverwriteData->setState(Settings::getInstance()->getBool("ScraperOverwriteData"));
    s->addWithLabel("OVERWRITE FILES AND DATA", scraperOverwriteData);
    s->addSaveFunc([scraperOverwriteData, s] {
        if (scraperOverwriteData->getState() !=
            Settings::getInstance()->getBool("ScraperOverwriteData")) {
            Settings::getInstance()->setBool("ScraperOverwriteData",
                                             scraperOverwriteData->getState());
            s->setNeedsSaving();
        }
    });

    // Halt scraping on invalid media files.
    auto scraperHaltOnInvalidMedia = std::make_shared<SwitchComponent>();
    scraperHaltOnInvalidMedia->setState(
        Settings::getInstance()->getBool("ScraperHaltOnInvalidMedia"));
    s->addWithLabel("HALT ON INVALID MEDIA FILES", scraperHaltOnInvalidMedia);
    s->addSaveFunc([scraperHaltOnInvalidMedia, s] {
        if (scraperHaltOnInvalidMedia->getState() !=
            Settings::getInstance()->getBool("ScraperHaltOnInvalidMedia")) {
            Settings::getInstance()->setBool("ScraperHaltOnInvalidMedia",
                                             scraperHaltOnInvalidMedia->getState());
            s->setNeedsSaving();
        }
    });

    // Search using metadata names.
    auto scraperSearchMetadataName = std::make_shared<SwitchComponent>();
    scraperSearchMetadataName->setState(
        Settings::getInstance()->getBool("ScraperSearchMetadataName"));
    s->addWithLabel("SEARCH USING METADATA NAMES", scraperSearchMetadataName);
    s->addSaveFunc([scraperSearchMetadataName, s] {
        if (scraperSearchMetadataName->getState() !=
            Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
            Settings::getInstance()->setBool("ScraperSearchMetadataName",
                                             scraperSearchMetadataName->getState());
            s->setNeedsSaving();
        }
    });

    // Include actual folders when scraping.
    auto scraperIncludeFolders = std::make_shared<SwitchComponent>();
    scraperIncludeFolders->setState(Settings::getInstance()->getBool("ScraperIncludeFolders"));
    s->addWithLabel("SCRAPE ACTUAL FOLDERS", scraperIncludeFolders);
    s->addSaveFunc([scraperIncludeFolders, s] {
        if (scraperIncludeFolders->getState() !=
            Settings::getInstance()->getBool("ScraperIncludeFolders")) {
            Settings::getInstance()->setBool("ScraperIncludeFolders",
                                             scraperIncludeFolders->getState());
            s->setNeedsSaving();
        }
    });

    // Interactive scraping.
    auto scraperInteractive = std::make_shared<SwitchComponent>();
    scraperInteractive->setState(Settings::getInstance()->getBool("ScraperInteractive"));
    s->addWithLabel("INTERACTIVE MODE", scraperInteractive);
    s->addSaveFunc([scraperInteractive, s] {
        if (scraperInteractive->getState() !=
            Settings::getInstance()->getBool("ScraperInteractive")) {
            Settings::getInstance()->setBool("ScraperInteractive", scraperInteractive->getState());
            s->setNeedsSaving();
        }
    });

    // Semi-automatic scraping.
    auto scraperSemiautomatic = std::make_shared<SwitchComponent>();
    scraperSemiautomatic->setState(Settings::getInstance()->getBool("ScraperSemiautomatic"));
    s->addWithLabel("AUTO-ACCEPT SINGLE GAME MATCHES", scraperSemiautomatic);
    s->addSaveFunc([scraperSemiautomatic, s] {
        if (scraperSemiautomatic->getState() !=
            Settings::getInstance()->getBool("ScraperSemiautomatic")) {
            Settings::getInstance()->setBool("ScraperSemiautomatic",
                                             scraperSemiautomatic->getState());
            s->setNeedsSaving();
        }
    });

    // If interactive mode is set to off, then gray out this option.
    if (!Settings::getInstance()->getBool("ScraperInteractive")) {
        scraperSemiautomatic->setEnabled(false);
        scraperSemiautomatic->setOpacity(DISABLED_OPACITY);
        scraperSemiautomatic->getParent()
            ->getChild(scraperSemiautomatic->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Respect the per-file multi-scraper exclusion flag.
    auto scraperRespectExclusions = std::make_shared<SwitchComponent>();
    scraperRespectExclusions->setState(
        Settings::getInstance()->getBool("ScraperRespectExclusions"));
    s->addWithLabel("RESPECT PER-FILE SCRAPER EXCLUSIONS", scraperRespectExclusions);
    s->addSaveFunc([scraperRespectExclusions, s] {
        if (scraperRespectExclusions->getState() !=
            Settings::getInstance()->getBool("ScraperRespectExclusions")) {
            Settings::getInstance()->setBool("ScraperRespectExclusions",
                                             scraperRespectExclusions->getState());
            s->setNeedsSaving();
        }
    });

    // Exclude files recursively for excluded folders.
    auto scraperExcludeRecursively = std::make_shared<SwitchComponent>();
    scraperExcludeRecursively->setState(
        Settings::getInstance()->getBool("ScraperExcludeRecursively"));
    s->addWithLabel("EXCLUDE FOLDERS RECURSIVELY", scraperExcludeRecursively);
    s->addSaveFunc([scraperExcludeRecursively, s] {
        if (scraperExcludeRecursively->getState() !=
            Settings::getInstance()->getBool("ScraperExcludeRecursively")) {
            Settings::getInstance()->setBool("ScraperExcludeRecursively",
                                             scraperExcludeRecursively->getState());
            s->setNeedsSaving();
        }
    });

    // If respecting excluded files is set to off, then gray out this option.
    if (!Settings::getInstance()->getBool("ScraperRespectExclusions")) {
        scraperExcludeRecursively->setEnabled(false);
        scraperExcludeRecursively->setOpacity(DISABLED_OPACITY);
        scraperExcludeRecursively->getParent()
            ->getChild(scraperExcludeRecursively->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Convert underscores to spaces when searching.
    auto scraperConvertUnderscores = std::make_shared<SwitchComponent>();
    scraperConvertUnderscores->setState(
        Settings::getInstance()->getBool("ScraperConvertUnderscores"));
    s->addWithLabel("CONVERT UNDERSCORES TO SPACES WHEN SEARCHING", scraperConvertUnderscores);
    s->addSaveFunc([scraperConvertUnderscores, s] {
        if (scraperConvertUnderscores->getState() !=
            Settings::getInstance()->getBool("ScraperConvertUnderscores")) {
            Settings::getInstance()->setBool("ScraperConvertUnderscores",
                                             scraperConvertUnderscores->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to fallback to additional regions.
    auto scraperRegionFallback = std::make_shared<SwitchComponent>(mWindow);
    scraperRegionFallback->setState(Settings::getInstance()->getBool("ScraperRegionFallback"));
    s->addWithLabel("ENABLE FALLBACK TO ADDITIONAL REGIONS", scraperRegionFallback);
    s->addSaveFunc([scraperRegionFallback, s] {
        if (scraperRegionFallback->getState() !=
            Settings::getInstance()->getBool("ScraperRegionFallback")) {
            Settings::getInstance()->setBool("ScraperRegionFallback",
                                             scraperRegionFallback->getState());
            s->setNeedsSaving();
        }
    });

    // Regions are not supported by TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraperRegionFallback->setEnabled(false);
        scraperRegionFallback->setOpacity(DISABLED_OPACITY);
        scraperRegionFallback->getParent()
            ->getChild(scraperRegionFallback->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Retry search on peer verification errors (TLS/certificate issues).
    auto scraperRetryPeerVerification = std::make_shared<SwitchComponent>();
    scraperRetryPeerVerification->setState(
        Settings::getInstance()->getBool("ScraperRetryPeerVerification"));
    s->addWithLabel("AUTO-RETRY ON PEER VERIFICATION ERRORS", scraperRetryPeerVerification);
    s->addSaveFunc([scraperRetryPeerVerification, s] {
        if (scraperRetryPeerVerification->getState() !=
            Settings::getInstance()->getBool("ScraperRetryPeerVerification")) {
            Settings::getInstance()->setBool("ScraperRetryPeerVerification",
                                             scraperRetryPeerVerification->getState());
            s->setNeedsSaving();
        }
    });

    // The TLS/certificate issue is not present for TheGamesDB, so gray out the option if this
    // scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraperRetryPeerVerification->setEnabled(false);
        scraperRetryPeerVerification->setOpacity(DISABLED_OPACITY);
        scraperRetryPeerVerification->getParent()
            ->getChild(scraperRetryPeerVerification->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Switch callbacks.
    auto interactiveToggleFunc = [scraperSemiautomatic]() {
        if (scraperSemiautomatic->getEnabled()) {
            scraperSemiautomatic->setEnabled(false);
            scraperSemiautomatic->setOpacity(DISABLED_OPACITY);
            scraperSemiautomatic->getParent()
                ->getChild(scraperSemiautomatic->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            scraperSemiautomatic->setEnabled(true);
            scraperSemiautomatic->setOpacity(1.0f);
            scraperSemiautomatic->getParent()
                ->getChild(scraperSemiautomatic->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
    };

    auto excludeRecursivelyToggleFunc = [scraperExcludeRecursively]() {
        if (scraperExcludeRecursively->getEnabled()) {
            scraperExcludeRecursively->setEnabled(false);
            scraperExcludeRecursively->setOpacity(DISABLED_OPACITY);
            scraperExcludeRecursively->getParent()
                ->getChild(scraperExcludeRecursively->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            scraperExcludeRecursively->setEnabled(true);
            scraperExcludeRecursively->setOpacity(1.0f);
            scraperExcludeRecursively->getParent()
                ->getChild(scraperExcludeRecursively->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
    };

    scraperInteractive->setCallback(interactiveToggleFunc);
    scraperRespectExclusions->setCallback(excludeRecursivelyToggleFunc);

    mWindow->pushGui(s);
}

void GuiScraperMenu::pressedStart()
{
    // If the scraper service has been changed, then save the settings as otherwise the
    // wrong scraper would be used.
    if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper"))
        mMenu.save();

    std::vector<SystemData*> sys {mSystems->getSelectedObjects()};
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
            mWindow->pushGui(new GuiMsgBox(getHelpStyle(), Utils::String::toUpper(warningString),
                                           "YES", std::bind(&GuiScraperMenu::start, this), "NO",
                                           nullptr));
            return;
        }
    }
    start();
}

void GuiScraperMenu::start()
{
    if (mSystems->getSelectedObjects().empty()) {
        mWindow->pushGui(
            new GuiMsgBox(getHelpStyle(), "PLEASE SELECT AT LEAST ONE SYSTEM TO SCRAPE"));
        return;
    }

    bool contentToScrape {false};
    std::string scraperService {Settings::getInstance()->getString("Scraper")};

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
        if (Settings::getInstance()->getBool("ScrapeBackCovers")) {
            contentToScrape = true;
            break;
        }
        if (Settings::getInstance()->getBool("ScrapeFanArt")) {
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
        mWindow->pushGui(
            new GuiMsgBox(getHelpStyle(), "PLEASE SELECT AT LEAST ONE CONTENT TYPE TO SCRAPE"));
        return;
    }

    std::queue<ScraperSearchParams> searches =
        getSearches(mSystems->getSelectedObjects(), mFilters->getSelected());

    if (searches.empty()) {
        mWindow->pushGui(
            new GuiMsgBox(getHelpStyle(), "ALL GAMES WERE FILTERED, NOTHING TO SCRAPE"));
    }
    else {
        GuiScraperMulti* gsm {
            new GuiScraperMulti(searches, Settings::getInstance()->getBool("ScraperInteractive"))};
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
        std::vector<FileData*> games {(*sys)->getRootFolder()->getScrapeFilesRecursive(
            Settings::getInstance()->getBool("ScraperIncludeFolders"),
            Settings::getInstance()->getBool("ScraperExcludeRecursively"),
            Settings::getInstance()->getBool("ScraperRespectExclusions"))};
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
    std::shared_ptr<Font> font {Font::get(FONT_SIZE_MEDIUM)};

    // Populate the list.
    ComponentListRow row;
    row.addElement(std::make_shared<TextComponent>(name, font, color), true);

    if (add_arrow) {
        std::shared_ptr<ImageComponent> bracket {makeArrow()};
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
    std::vector<HelpPrompt> prompts {mMenu.getHelpPrompts()};
    prompts.push_back(HelpPrompt("b", "back"));
    prompts.push_back(HelpPrompt("y", "start"));
    return prompts;
}

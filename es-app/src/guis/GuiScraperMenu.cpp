//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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
#include "utils/LocalizationUtil.h"

GuiScraperMenu::GuiScraperMenu(std::string title)
    : mRenderer {Renderer::getInstance()}
    , mMenu {title}
{
    // Scraper service.
    mScraper =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), _("SCRAPE FROM"), false);
    std::vector<std::string> scrapers = getScraperList();
    // Select either the first entry or the one read from the settings,
    // just in case the scraper from settings has vanished.
    for (auto it = scrapers.cbegin(); it != scrapers.cend(); ++it)
        mScraper->add(Utils::String::toUpper(*it), *it,
                      *it == Settings::getInstance()->getString("Scraper"));
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the scraper to "screenscraper" in this case.
    if (mScraper->getSelectedObjects().size() == 0)
        mScraper->selectEntry(0);

    mMenu.addWithLabel(_("SCRAPE FROM"), mScraper);

    // Search filters, getSearches() will generate a queue of games to scrape
    // based on the outcome of the checks below.
    mFilters = std::make_shared<OptionListComponent<GameFilterFunc>>(
        getHelpStyle(), _("SCRAPE THESE GAMES"), false);
    mFilters->add(
        _("ALL GAMES"),
        [](SystemData*, FileData*) -> bool {
            // All games.
            return true;
        },
        false);
    mFilters->add(
        _("FAVORITE GAMES"),
        [](SystemData*, FileData* g) -> bool {
            // Favorite games.
            return g->getFavorite();
        },
        false);
    mFilters->add(
        _("NO METADATA"),
        [](SystemData*, FileData* g) -> bool {
            // No metadata.
            return g->metadata.get("desc").empty();
        },
        false);
    mFilters->add(
        _("NO GAME IMAGE"),
        [](SystemData*, FileData* g) -> bool {
            // No game image.
            return g->getImagePath().empty();
        },
        false);
    mFilters->add(
        _("NO GAME VIDEO"),
        [](SystemData*, FileData* g) -> bool {
            // No game video.
            return g->getVideoPath().empty();
        },
        false);
    mFilters->add(
        _("FOLDERS ONLY"),
        [](SystemData*, FileData* g) -> bool {
            // Folders only.
            return g->getType() == FOLDER;
        },
        false);

    mFilters->selectEntry(Settings::getInstance()->getInt("ScraperFilter"));
    mMenu.addWithLabel(_("SCRAPE THESE GAMES"), mFilters);

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
                                                                  _("SCRAPE THESE SYSTEMS"), true);
    for (unsigned int i {0}; i < SystemData::sSystemVector.size(); ++i) {
        if (!SystemData::sSystemVector[i]->hasPlatformId(PlatformIds::PLATFORM_IGNORE)) {
            mSystems->add(Utils::String::toUpper(SystemData::sSystemVector[i]->getFullName()),
                          SystemData::sSystemVector[i],
                          !SystemData::sSystemVector[i]->getPlatformIds().empty());
            SystemData::sSystemVector[i]->getScrapeFlag() ? mSystems->selectEntry(i) :
                                                            mSystems->unselectEntry(i);
        }
    }
    mMenu.addWithLabel(_("SCRAPE THESE SYSTEMS"), mSystems);

    addEntry(_("ACCOUNT SETTINGS"), mMenuColorPrimary, true, [this] {
        // Open the account options menu.
        openAccountOptions();
    });
    addEntry(_("CONTENT SETTINGS"), mMenuColorPrimary, true, [this] {
        // If the scraper service has been changed before entering this menu, then save the
        // settings so that the specific options supported by the respective scrapers
        // can be enabled or disabled.
        if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper"))
            mMenu.save();
        openContentOptions();
    });
    addEntry(_("MIXIMAGE SETTINGS"), mMenuColorPrimary, true, [this] {
        // Open the miximage options menu.
        openMiximageOptions();
    });
    addEntry(_("OTHER SETTINGS"), mMenuColorPrimary, true, [this] {
        // If the scraper service has been changed before entering this menu, then save the
        // settings so that the specific options supported by the respective scrapers
        // can be enabled or disabled.
        if (mScraper->getSelected() != Settings::getInstance()->getString("Scraper"))
            mMenu.save();
        openOtherOptions();
    });

    addChild(&mMenu);

    mMenu.addButton(_("START"), _("start scraper"), std::bind(&GuiScraperMenu::pressedStart, this));
    mMenu.addButton(_("BACK"), _("back"), [&] { delete this; });

    setSize(mMenu.getSize());

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                mRenderer->getScreenHeight() * 0.13f);

    // Make sure that the hash searching max file size is within the allowed range.
    if (Settings::getInstance()->getInt("ScraperSearchFileHashMaxSize") < 32)
        Settings::getInstance()->setInt("ScraperSearchFileHashMaxSize", 32);
    else if (Settings::getInstance()->getInt("ScraperSearchFileHashMaxSize") > 800)
        Settings::getInstance()->setInt("ScraperSearchFileHashMaxSize", 800);
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
    auto s = new GuiSettings(_("ACCOUNT SETTINGS"));

    // ScreenScraper username.
    auto scraperUsernameScreenScraper = std::make_shared<TextComponent>(
        "", Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary, ALIGN_RIGHT);
    s->addEditableTextComponent(_("SCREENSCRAPER USERNAME"), scraperUsernameScreenScraper,
                                Settings::getInstance()->getString("ScraperUsernameScreenScraper"));
    scraperUsernameScreenScraper->setSize(0.0f,
                                          scraperUsernameScreenScraper->getFont()->getHeight());
    s->addSaveFunc([scraperUsernameScreenScraper, s] {
        if (scraperUsernameScreenScraper->getValue() !=
            Settings::getInstance()->getString("ScraperUsernameScreenScraper")) {
            Settings::getInstance()->setString("ScraperUsernameScreenScraper",
                                               scraperUsernameScreenScraper->getValue());
            s->setNeedsSaving();
        }
    });

    // ScreenScraper password.
    auto scraperPasswordScreenScraper = std::make_shared<TextComponent>(
        "", Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary, ALIGN_RIGHT);
    std::string passwordMasked;
    if (Settings::getInstance()->getString("ScraperPasswordScreenScraper") != "") {
        passwordMasked = "********";
        scraperPasswordScreenScraper->setHiddenValue(
            Settings::getInstance()->getString("ScraperPasswordScreenScraper"));
    }
    s->addEditableTextComponent(_("SCREENSCRAPER PASSWORD"), scraperPasswordScreenScraper,
                                passwordMasked, "", true);
    scraperPasswordScreenScraper->setSize(0.0f,
                                          scraperPasswordScreenScraper->getFont()->getHeight());
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
    s->addWithLabel(_("USE THIS ACCOUNT FOR SCREENSCRAPER"), scraperUseAccountScreenScraper);
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
    auto s = new GuiSettings(_("CONTENT SETTINGS"));

    // Scrape game names.
    auto scrapeGameNames = std::make_shared<SwitchComponent>();
    scrapeGameNames->setState(Settings::getInstance()->getBool("ScrapeGameNames"));
    s->addWithLabel(_("GAME NAMES"), scrapeGameNames);
    s->addSaveFunc([scrapeGameNames, s] {
        if (scrapeGameNames->getState() != Settings::getInstance()->getBool("ScrapeGameNames")) {
            Settings::getInstance()->setBool("ScrapeGameNames", scrapeGameNames->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape ratings.
    auto scrapeRatings = std::make_shared<SwitchComponent>();
    scrapeRatings->setState(Settings::getInstance()->getBool("ScrapeRatings"));
    s->addWithLabel(_("RATINGS"), scrapeRatings);
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

    // ScreenScraper controller scraping is currently broken, it's unclear if they will fix it.
    // // Scrape controllers (arcade systems only).
    // auto scrapeControllers = std::make_shared<SwitchComponent>();
    // scrapeControllers->setState(Settings::getInstance()->getBool("ScrapeControllers"));
    // s->addWithLabel("CONTROLLERS (ARCADE SYSTEMS ONLY)", scrapeControllers);
    // s->addSaveFunc([scrapeControllers, s] {
    //     if (scrapeControllers->getState() !=
    //         Settings::getInstance()->getBool("ScrapeControllers")) {
    //         Settings::getInstance()->setBool("ScrapeControllers", scrapeControllers->getState());
    //         s->setNeedsSaving();
    //     }
    // });

    // // Controllers are not supported by TheGamesDB, so gray out the option if this scraper is
    // // selected.
    // if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
    //     scrapeControllers->setEnabled(false);
    //     scrapeControllers->setOpacity(DISABLED_OPACITY);
    //     scrapeControllers->getParent()
    //         ->getChild(scrapeControllers->getChildIndex() - 1)
    //         ->setOpacity(DISABLED_OPACITY);
    // }

    // Scrape other metadata.
    auto scrapeMetadata = std::make_shared<SwitchComponent>();
    scrapeMetadata->setState(Settings::getInstance()->getBool("ScrapeMetadata"));
    s->addWithLabel(_("OTHER METADATA"), scrapeMetadata);
    s->addSaveFunc([scrapeMetadata, s] {
        if (scrapeMetadata->getState() != Settings::getInstance()->getBool("ScrapeMetadata")) {
            Settings::getInstance()->setBool("ScrapeMetadata", scrapeMetadata->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape videos.
    auto scrapeVideos = std::make_shared<SwitchComponent>();
    scrapeVideos->setState(Settings::getInstance()->getBool("ScrapeVideos"));
    s->addWithLabel(_("VIDEOS"), scrapeVideos);
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
    s->addWithLabel(_("SCREENSHOT IMAGES"), scrapeScreenshots);
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
    s->addWithLabel(_("TITLE SCREEN IMAGES"), scrapeTitleScreens);
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
    s->addWithLabel(_("BOX COVER IMAGES"), scrapeCovers);
    s->addSaveFunc([scrapeCovers, s] {
        if (scrapeCovers->getState() != Settings::getInstance()->getBool("ScrapeCovers")) {
            Settings::getInstance()->setBool("ScrapeCovers", scrapeCovers->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape box back cover images.
    auto scrapeBackCovers = std::make_shared<SwitchComponent>();
    scrapeBackCovers->setState(Settings::getInstance()->getBool("ScrapeBackCovers"));
    s->addWithLabel(_("BOX BACK COVER IMAGES"), scrapeBackCovers);
    s->addSaveFunc([scrapeBackCovers, s] {
        if (scrapeBackCovers->getState() != Settings::getInstance()->getBool("ScrapeBackCovers")) {
            Settings::getInstance()->setBool("ScrapeBackCovers", scrapeBackCovers->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape marquee images.
    auto scrapeMarquees = std::make_shared<SwitchComponent>();
    scrapeMarquees->setState(Settings::getInstance()->getBool("ScrapeMarquees"));
    s->addWithLabel(_("MARQUEE (WHEEL) IMAGES"), scrapeMarquees);
    s->addSaveFunc([scrapeMarquees, s] {
        if (scrapeMarquees->getState() != Settings::getInstance()->getBool("ScrapeMarquees")) {
            Settings::getInstance()->setBool("ScrapeMarquees", scrapeMarquees->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape 3D box images.
    auto scrape3dBoxes = std::make_shared<SwitchComponent>();
    scrape3dBoxes->setState(Settings::getInstance()->getBool("Scrape3DBoxes"));
    s->addWithLabel(_("3D BOX IMAGES"), scrape3dBoxes);
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
    s->addWithLabel(_("PHYSICAL MEDIA IMAGES"), scrapePhysicalMedia);
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
    s->addWithLabel(_("FAN ART IMAGES"), scrapeFanArt);
    s->addSaveFunc([scrapeFanArt, s] {
        if (scrapeFanArt->getState() != Settings::getInstance()->getBool("ScrapeFanArt")) {
            Settings::getInstance()->setBool("ScrapeFanArt", scrapeFanArt->getState());
            s->setNeedsSaving();
        }
    });

    // Scrape game manuals.
    auto scrapeManuals = std::make_shared<SwitchComponent>();
    scrapeManuals->setState(Settings::getInstance()->getBool("ScrapeManuals"));
    s->addWithLabel(_("GAME MANUALS"), scrapeManuals);
    s->addSaveFunc([scrapeManuals, s] {
        if (scrapeManuals->getState() != Settings::getInstance()->getBool("ScrapeManuals")) {
            Settings::getInstance()->setBool("ScrapeManuals", scrapeManuals->getState());
            s->setNeedsSaving();
        }
    });

    // Game manuals are not supported by TheGamesDB, so gray out the option if this scraper
    // is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scrapeManuals->setEnabled(false);
        scrapeManuals->setOpacity(DISABLED_OPACITY);
        scrapeManuals->getParent()
            ->getChild(scrapeManuals->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    mWindow->pushGui(s);
}

void GuiScraperMenu::openMiximageOptions()
{
    auto s = new GuiSettings(_("MIXIMAGE SETTINGS"));

    // Miximage resolution.
    auto miximageResolution = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _("MIXIMAGE RESOLUTION"), false);
    std::string selectedResolution {Settings::getInstance()->getString("MiximageResolution")};
    miximageResolution->add("1280X960", "1280x960", selectedResolution == "1280x960");
    miximageResolution->add("1920X1440", "1920x1440", selectedResolution == "1920x1440");
    miximageResolution->add("640X480", "640x480", selectedResolution == "640x480");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the resolution to "1280x960" in this case.
    if (miximageResolution->getSelectedObjects().size() == 0)
        miximageResolution->selectEntry(0);
    s->addWithLabel(_("MIXIMAGE RESOLUTION"), miximageResolution);
    s->addSaveFunc([miximageResolution, s] {
        if (miximageResolution->getSelected() !=
            Settings::getInstance()->getString("MiximageResolution")) {
            Settings::getInstance()->setString("MiximageResolution",
                                               miximageResolution->getSelected());
            s->setNeedsSaving();
        }
    });

    // Horizontally oriented screenshots fit.
    auto miximageHorizontalFit = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _p("short", "HORIZONTAL SCREENSHOT FIT"), false);
    const std::string selectedHorizontalFit {
        Settings::getInstance()->getString("MiximageScreenshotHorizontalFit")};
    miximageHorizontalFit->add(_("CONTAIN"), "contain", selectedHorizontalFit == "contain");
    miximageHorizontalFit->add(_("CROP"), "crop", selectedHorizontalFit == "crop");
    miximageHorizontalFit->add(_("STRETCH"), "stretch", selectedHorizontalFit == "stretch");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the horizontal screenshot fit to "crop" in this case.
    if (miximageHorizontalFit->getSelectedObjects().size() == 0)
        miximageHorizontalFit->selectEntry(1);
    s->addWithLabel(_("HORIZONTAL SCREENSHOT FIT"), miximageHorizontalFit);
    s->addSaveFunc([miximageHorizontalFit, s] {
        if (miximageHorizontalFit->getSelected() !=
            Settings::getInstance()->getString("MiximageScreenshotHorizontalFit")) {
            Settings::getInstance()->setString("MiximageScreenshotHorizontalFit",
                                               miximageHorizontalFit->getSelected());
            s->setNeedsSaving();
        }
    });

    // Vertically oriented screenshots fit.
    auto miximageVerticalFit = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _p("short", "VERTICAL SCREENSHOT FIT"), false);
    const std::string selectedVerticalFit {
        Settings::getInstance()->getString("MiximageScreenshotVerticalFit")};
    miximageVerticalFit->add(_("CONTAIN"), "contain", selectedVerticalFit == "contain");
    miximageVerticalFit->add(_("CROP"), "crop", selectedVerticalFit == "crop");
    miximageVerticalFit->add(_("STRETCH"), "stretch", selectedVerticalFit == "stretch");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the vertical screenshot fit to "contain" in this case.
    if (miximageVerticalFit->getSelectedObjects().size() == 0)
        miximageVerticalFit->selectEntry(0);
    s->addWithLabel(_("VERTICAL SCREENSHOT FIT"), miximageVerticalFit);
    s->addSaveFunc([miximageVerticalFit, s] {
        if (miximageVerticalFit->getSelected() !=
            Settings::getInstance()->getString("MiximageScreenshotVerticalFit")) {
            Settings::getInstance()->setString("MiximageScreenshotVerticalFit",
                                               miximageVerticalFit->getSelected());
            s->setNeedsSaving();
        }
    });

    // Screenshots aspect ratio threshold.
    auto miximageAspectThreshold = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _p("short", "SCREENSHOT ASPECT RATIO THRESHOLD"), false);
    const std::string selectedAspectThreshold {
        Settings::getInstance()->getString("MiximageScreenshotAspectThreshold")};
    miximageAspectThreshold->add(_("HIGH"), "high", selectedAspectThreshold == "high");
    miximageAspectThreshold->add(_("LOW"), "low", selectedAspectThreshold == "low");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the screenshot aspect threshold to "high" in this case.
    if (miximageAspectThreshold->getSelectedObjects().size() == 0)
        miximageAspectThreshold->selectEntry(0);
    s->addWithLabel(_("SCREENSHOT ASPECT RATIO THRESHOLD"), miximageAspectThreshold);
    s->addSaveFunc([miximageAspectThreshold, s] {
        if (miximageAspectThreshold->getSelected() !=
            Settings::getInstance()->getString("MiximageScreenshotAspectThreshold")) {
            Settings::getInstance()->setString("MiximageScreenshotAspectThreshold",
                                               miximageAspectThreshold->getSelected());
            s->setNeedsSaving();
        }
    });

    // Blank areas fill color.
    auto miximageBlankAreasColor = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _("BLANK AREAS FILL COLOR"), false);
    const std::string selectedBlankAreasColor {
        Settings::getInstance()->getString("MiximageScreenshotBlankAreasColor")};
    miximageBlankAreasColor->add(_("BLACK"), "black", selectedBlankAreasColor == "black");
    miximageBlankAreasColor->add(_("FRAME"), "frame", selectedBlankAreasColor == "frame");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the blank area fill color to "black" in this case.
    if (miximageBlankAreasColor->getSelectedObjects().size() == 0)
        miximageBlankAreasColor->selectEntry(0);
    s->addWithLabel(_("BLANK AREAS FILL COLOR"), miximageBlankAreasColor);
    s->addSaveFunc([miximageBlankAreasColor, s] {
        if (miximageBlankAreasColor->getSelected() !=
            Settings::getInstance()->getString("MiximageScreenshotBlankAreasColor")) {
            Settings::getInstance()->setString("MiximageScreenshotBlankAreasColor",
                                               miximageBlankAreasColor->getSelected());
            s->setNeedsSaving();
        }
    });

    // Screenshot scaling method.
    auto miximageScaling = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _p("short", "SCREENSHOT SCALING METHOD"), false);
    std::string selectedScaling {Settings::getInstance()->getString("MiximageScreenshotScaling")};
    miximageScaling->add(_("SHARP"), "sharp", selectedScaling == "sharp");
    miximageScaling->add(_("SMOOTH"), "smooth", selectedScaling == "smooth");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the scaling method to "sharp" in this case.
    if (miximageScaling->getSelectedObjects().size() == 0)
        miximageScaling->selectEntry(0);
    s->addWithLabel(_("SCREENSHOT SCALING METHOD"), miximageScaling);
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
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), _("BOX SIZE"), false);
    std::string selectedBoxSize {Settings::getInstance()->getString("MiximageBoxSize")};
    miximageBoxSize->add(_("SMALL"), "small", selectedBoxSize == "small");
    miximageBoxSize->add(_("MEDIUM"), "medium", selectedBoxSize == "medium");
    miximageBoxSize->add(_("LARGE"), "large", selectedBoxSize == "large");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the box size to "medium" in this case.
    if (miximageBoxSize->getSelectedObjects().size() == 0)
        miximageBoxSize->selectEntry(0);
    s->addWithLabel(_("BOX SIZE"), miximageBoxSize);
    s->addSaveFunc([miximageBoxSize, s] {
        if (miximageBoxSize->getSelected() !=
            Settings::getInstance()->getString("MiximageBoxSize")) {
            Settings::getInstance()->setString("MiximageBoxSize", miximageBoxSize->getSelected());
            s->setNeedsSaving();
        }
    });

    // Physical media size.
    auto miximagePhysicalMediaSize = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _("PHYSICAL MEDIA SIZE"), false);
    std::string selectedPhysicalMediaSize {
        Settings::getInstance()->getString("MiximagePhysicalMediaSize")};
    miximagePhysicalMediaSize->add(_("SMALL"), "small", selectedPhysicalMediaSize == "small");
    miximagePhysicalMediaSize->add(_("MEDIUM"), "medium", selectedPhysicalMediaSize == "medium");
    miximagePhysicalMediaSize->add(_("LARGE"), "large", selectedPhysicalMediaSize == "large");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the physical media size to "medium" in this case.
    if (miximagePhysicalMediaSize->getSelectedObjects().size() == 0)
        miximagePhysicalMediaSize->selectEntry(0);
    s->addWithLabel(_("PHYSICAL MEDIA SIZE"), miximagePhysicalMediaSize);
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
    s->addWithLabel(_("GENERATE MIXIMAGES WHEN SCRAPING"), miximageGenerate);
    s->addSaveFunc([miximageGenerate, s] {
        if (miximageGenerate->getState() != Settings::getInstance()->getBool("MiximageGenerate")) {
            Settings::getInstance()->setBool("MiximageGenerate", miximageGenerate->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to overwrite miximages (both for the scraper and offline generator).
    auto miximageOverwrite = std::make_shared<SwitchComponent>();
    miximageOverwrite->setState(Settings::getInstance()->getBool("MiximageOverwrite"));
    s->addWithLabel(_("OVERWRITE MIXIMAGES (SCRAPER/OFFLINE GENERATOR)"), miximageOverwrite);
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
    s->addWithLabel(_("REMOVE LETTERBOXES FROM SCREENSHOTS"), miximageRemoveLetterboxes);
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
    s->addWithLabel(_("REMOVE PILLARBOXES FROM SCREENSHOTS"), miximageRemovePillarboxes);
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
    s->addWithLabel(_("ROTATE HORIZONTALLY ORIENTED BOXES"), miximageRotateBoxes);
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
    s->addWithLabel(_("INCLUDE MARQUEE IMAGE"), miximageIncludeMarquee);
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
    s->addWithLabel(_("INCLUDE BOX IMAGE"), miximageIncludeBox);
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
    s->addWithLabel(_("USE COVER IMAGE IF 3D BOX IS MISSING"), miximageCoverFallback);
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
    s->addWithLabel(_("INCLUDE PHYSICAL MEDIA IMAGE"), miximageIncludePhysicalMedia);
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
    offlineGeneratorRow.addElement(std::make_shared<TextComponent>(_("OFFLINE GENERATOR"),
                                                                   Font::get(FONT_SIZE_MEDIUM),
                                                                   mMenuColorPrimary),
                                   true);
    offlineGeneratorRow.addElement(mMenu.makeArrow(), false);
    offlineGeneratorRow.makeAcceptInputHandler(
        std::bind(&GuiScraperMenu::openOfflineGenerator, this, s));
    s->addRow(offlineGeneratorRow);

    mWindow->pushGui(s);
}

void GuiScraperMenu::openOfflineGenerator(GuiSettings* settings)
{
    if (mSystems->getSelectedObjects().empty()) {
        mWindow->pushGui(new GuiMsgBox(getHelpStyle(),
                                       _("THE OFFLINE GENERATOR USES THE SAME SYSTEM "
                                         "SELECTIONS AS THE SCRAPER, SO PLEASE SELECT "
                                         "AT LEAST ONE SYSTEM TO GENERATE IMAGES FOR"),
                                       _("OK"), nullptr, "", nullptr, "", nullptr, nullptr, false,
                                       true,
                                       (mRenderer->getIsVerticalOrientation() ?
                                            0.80f :
                                            0.50f * (1.778f / mRenderer->getScreenAspectRatio()))));

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

        // Sort the games by "name, ascending".
        std::stable_sort(games.begin(), games.end(), FileSorts::SortTypes.at(0).comparisonFunction);

        for (FileData* game : games)
            gameQueue.push(game);
    }

    mWindow->pushGui(new GuiOfflineGenerator(gameQueue));
}

void GuiScraperMenu::openOtherOptions()
{
    auto s = new GuiSettings(_("OTHER SETTINGS"));

    // Scraper region.
    auto scraperRegion =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), _("REGION"), false);
    std::string selectedScraperRegion {Settings::getInstance()->getString("ScraperRegion")};
    // clang-format off
    scraperRegion->add(_("EUROPE"), "eu",  selectedScraperRegion == "eu");
    scraperRegion->add(_("JAPAN"),  "jp",  selectedScraperRegion == "jp");
    scraperRegion->add(_("USA"),    "us",  selectedScraperRegion == "us");
    scraperRegion->add(_("WORLD"),  "wor", selectedScraperRegion == "wor");
    // clang-format on
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the region to "Europe" in this case.
    if (scraperRegion->getSelectedObjects().size() == 0)
        scraperRegion->selectEntry(0);
    s->addWithLabel(_("REGION"), scraperRegion);
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
        getHelpStyle(), _("PREFERRED LANGUAGE"), false);
    std::string selectedScraperLanguage {Settings::getInstance()->getString("ScraperLanguage")};
    // clang-format off
    scraperLanguage->add("ENGLISH",    "en", selectedScraperLanguage == "en");
    scraperLanguage->add("ČEŠTINA",    "cz", selectedScraperLanguage == "cz");
    scraperLanguage->add("DANSK",      "da", selectedScraperLanguage == "da");
    scraperLanguage->add("DEUTSCH",    "de", selectedScraperLanguage == "de");
    scraperLanguage->add("ESPAÑOL",    "es", selectedScraperLanguage == "es");
    scraperLanguage->add("FRANÇAIS",   "fr", selectedScraperLanguage == "fr");
    scraperLanguage->add("ITALIANO",   "it", selectedScraperLanguage == "it");
    scraperLanguage->add("MAGYAR",     "hu", selectedScraperLanguage == "hu");
    scraperLanguage->add("NEDERLANDS", "nl", selectedScraperLanguage == "nl");
    scraperLanguage->add("NORSK",      "no", selectedScraperLanguage == "no");
    scraperLanguage->add("POLSKI",     "pl", selectedScraperLanguage == "pl");
    scraperLanguage->add("PORTUGUÊS",  "pt", selectedScraperLanguage == "pt");
    scraperLanguage->add("РУССКИЙ",    "ru", selectedScraperLanguage == "ru");
    scraperLanguage->add("SLOVENČINA", "sk", selectedScraperLanguage == "sk");
    scraperLanguage->add("SUOMI",      "fi", selectedScraperLanguage == "fi");
    scraperLanguage->add("SVENSKA",    "sv", selectedScraperLanguage == "sv");
    scraperLanguage->add("TÜRKÇE",     "tr", selectedScraperLanguage == "tr");
    scraperLanguage->add("日本語",      "ja", selectedScraperLanguage == "ja");
    scraperLanguage->add("한국어",      "ko", selectedScraperLanguage == "ko");
    scraperLanguage->add("简体中文",    "zh", selectedScraperLanguage == "zh");
    // clang-format on
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the language to "English" in this case.
    if (scraperLanguage->getSelectedObjects().size() == 0)
        scraperLanguage->selectEntry(0);
    s->addWithLabel(_("PREFERRED LANGUAGE"), scraperLanguage);
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

    // Automatic retries on error.
    mScraperRetryOnErrorCount = std::make_shared<SliderComponent>(0.0f, 10.0f, 1.0f);
    mScraperRetryOnErrorCount->setValue(
        static_cast<float>(Settings::getInstance()->getInt("ScraperRetryOnErrorCount")));
    s->addWithLabel(_("AUTOMATIC RETRIES ON ERROR"), mScraperRetryOnErrorCount);
    s->addSaveFunc([this, s] {
        if (mScraperRetryOnErrorCount->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScraperRetryOnErrorCount"))) {
            Settings::getInstance()->setInt(
                "ScraperRetryOnErrorCount",
                static_cast<int>(mScraperRetryOnErrorCount->getValue()));
            s->setNeedsSaving();
        }
    });

    // Retry attempt timer.
    auto scraperRetryOnErrorTimer = std::make_shared<SliderComponent>(1.0f, 30.0f, 1.0f, "s");
    scraperRetryOnErrorTimer->setValue(
        static_cast<float>(Settings::getInstance()->getInt("ScraperRetryOnErrorTimer")));
    s->addWithLabel(_("RETRY ATTEMPT TIMER"), scraperRetryOnErrorTimer);
    s->addSaveFunc([scraperRetryOnErrorTimer, s] {
        if (scraperRetryOnErrorTimer->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScraperRetryOnErrorTimer"))) {
            Settings::getInstance()->setInt("ScraperRetryOnErrorTimer",
                                            static_cast<int>(scraperRetryOnErrorTimer->getValue()));
            s->setNeedsSaving();
        }
    });

    if (mScraperRetryOnErrorCount->getValue() == 0.0f) {
        scraperRetryOnErrorTimer->setEnabled(false);
        scraperRetryOnErrorTimer->setOpacity(DISABLED_OPACITY);
        scraperRetryOnErrorTimer->getParent()
            ->getChild(scraperRetryOnErrorTimer->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Maximum file size for non-interactive mode file hash searching.
    auto scraperSearchFileHashMaxSize =
        std::make_shared<SliderComponent>(32.0f, 800.0f, 32.0f, "MiB");
    scraperSearchFileHashMaxSize->setValue(
        static_cast<float>(Settings::getInstance()->getInt("ScraperSearchFileHashMaxSize")));
    s->addWithLabel(_("HASH SEARCHES MAX FILE SIZE"), scraperSearchFileHashMaxSize);
    s->addSaveFunc([scraperSearchFileHashMaxSize, s] {
        if (scraperSearchFileHashMaxSize->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScraperSearchFileHashMaxSize"))) {
            Settings::getInstance()->setInt(
                "ScraperSearchFileHashMaxSize",
                static_cast<int>(scraperSearchFileHashMaxSize->getValue()));
            s->setNeedsSaving();
        }
    });

    // File hash searching is not supported by TheGamesDB, so gray out the option if this scraper
    // is selected. Also gray it out for ScreenScraper if file hash searching has been disabled.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb" ||
        !Settings::getInstance()->getBool("ScraperSearchFileHash")) {
        scraperSearchFileHashMaxSize->setEnabled(false);
        scraperSearchFileHashMaxSize->setOpacity(DISABLED_OPACITY);
        scraperSearchFileHashMaxSize->getParent()
            ->getChild(scraperSearchFileHashMaxSize->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Overwrite files and data.
    auto scraperOverwriteData = std::make_shared<SwitchComponent>();
    scraperOverwriteData->setState(Settings::getInstance()->getBool("ScraperOverwriteData"));
    s->addWithLabel(_("OVERWRITE FILES AND DATA"), scraperOverwriteData);
    s->addSaveFunc([scraperOverwriteData, s] {
        if (scraperOverwriteData->getState() !=
            Settings::getInstance()->getBool("ScraperOverwriteData")) {
            Settings::getInstance()->setBool("ScraperOverwriteData",
                                             scraperOverwriteData->getState());
            s->setNeedsSaving();
        }
    });

    // Search using file hashes for non-interactive mode.
    auto scraperSearchFileHash = std::make_shared<SwitchComponent>();
    scraperSearchFileHash->setState(Settings::getInstance()->getBool("ScraperSearchFileHash"));
    s->addWithLabel(_("SEARCH USING FILE HASHES (NON-INTERACTIVE MODE)"), scraperSearchFileHash);
    s->addSaveFunc([scraperSearchFileHash, s] {
        if (scraperSearchFileHash->getState() !=
            Settings::getInstance()->getBool("ScraperSearchFileHash")) {
            Settings::getInstance()->setBool("ScraperSearchFileHash",
                                             scraperSearchFileHash->getState());
            s->setNeedsSaving();
        }
    });

    // File hash searching is not supported by TheGamesDB, so gray out the option if this scraper
    // is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraperSearchFileHash->setEnabled(false);
        scraperSearchFileHash->setOpacity(DISABLED_OPACITY);
        scraperSearchFileHash->getParent()
            ->getChild(scraperSearchFileHash->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Search using metadata names.
    auto scraperSearchMetadataName = std::make_shared<SwitchComponent>();
    scraperSearchMetadataName->setState(
        Settings::getInstance()->getBool("ScraperSearchMetadataName"));
    s->addWithLabel(_("SEARCH USING METADATA NAMES"), scraperSearchMetadataName);
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
    s->addWithLabel(_("SCRAPE ACTUAL FOLDERS"), scraperIncludeFolders);
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
    s->addWithLabel(_("INTERACTIVE MODE"), scraperInteractive);
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
    s->addWithLabel(_("AUTO-ACCEPT SINGLE GAME MATCHES"), scraperSemiautomatic);
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
    s->addWithLabel(_("RESPECT PER-FILE SCRAPER EXCLUSIONS"), scraperRespectExclusions);
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
    s->addWithLabel(_("EXCLUDE FOLDERS RECURSIVELY"), scraperExcludeRecursively);
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
    s->addWithLabel(_("CONVERT UNDERSCORES TO SPACES WHEN SEARCHING"), scraperConvertUnderscores);
    s->addSaveFunc([scraperConvertUnderscores, s] {
        if (scraperConvertUnderscores->getState() !=
            Settings::getInstance()->getBool("ScraperConvertUnderscores")) {
            Settings::getInstance()->setBool("ScraperConvertUnderscores",
                                             scraperConvertUnderscores->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to remove dots from game names when searching using the automatic scraper.
    auto scraperAutomaticRemoveDots = std::make_shared<SwitchComponent>();
    scraperAutomaticRemoveDots->setState(
        Settings::getInstance()->getBool("ScraperAutomaticRemoveDots"));
    s->addWithLabel(_("REMOVE DOTS FROM SEARCHES WHEN AUTO-SCRAPING"), scraperAutomaticRemoveDots);
    s->addSaveFunc([scraperAutomaticRemoveDots, s] {
        if (scraperAutomaticRemoveDots->getState() !=
            Settings::getInstance()->getBool("ScraperAutomaticRemoveDots")) {
            Settings::getInstance()->setBool("ScraperAutomaticRemoveDots",
                                             scraperAutomaticRemoveDots->getState());
            s->setNeedsSaving();
        }
    });

    // This is not needed for TheGamesDB, so gray out the option if this scraper is selected.
    if (Settings::getInstance()->getString("Scraper") == "thegamesdb") {
        scraperAutomaticRemoveDots->setEnabled(false);
        scraperAutomaticRemoveDots->setOpacity(DISABLED_OPACITY);
        scraperAutomaticRemoveDots->getParent()
            ->getChild(scraperAutomaticRemoveDots->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Whether to fallback to additional regions.
    auto scraperRegionFallback = std::make_shared<SwitchComponent>(mWindow);
    scraperRegionFallback->setState(Settings::getInstance()->getBool("ScraperRegionFallback"));
    s->addWithLabel(_("ENABLE FALLBACK TO ADDITIONAL REGIONS"), scraperRegionFallback);
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

    // Slider callback.
    auto scraperRetryCountFunc = [this, scraperRetryOnErrorTimer]() {
        if (mScraperRetryOnErrorCount->getValue() == 0.0f) {
            scraperRetryOnErrorTimer->setEnabled(false);
            scraperRetryOnErrorTimer->setOpacity(DISABLED_OPACITY);
            scraperRetryOnErrorTimer->getParent()
                ->getChild(scraperRetryOnErrorTimer->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            scraperRetryOnErrorTimer->setEnabled(true);
            scraperRetryOnErrorTimer->setOpacity(1.0f);
            scraperRetryOnErrorTimer->getParent()
                ->getChild(scraperRetryOnErrorTimer->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
    };

    // Switch callbacks.
    auto hashSearchToggleFunc = [scraperSearchFileHashMaxSize]() {
        if (scraperSearchFileHashMaxSize->getEnabled()) {
            scraperSearchFileHashMaxSize->setEnabled(false);
            scraperSearchFileHashMaxSize->setOpacity(DISABLED_OPACITY);
            scraperSearchFileHashMaxSize->getParent()
                ->getChild(scraperSearchFileHashMaxSize->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            scraperSearchFileHashMaxSize->setEnabled(true);
            scraperSearchFileHashMaxSize->setOpacity(1.0f);
            scraperSearchFileHashMaxSize->getParent()
                ->getChild(scraperSearchFileHashMaxSize->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
    };

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

    mScraperRetryOnErrorCount->setCallback(scraperRetryCountFunc);
    scraperSearchFileHash->setCallback(hashSearchToggleFunc);
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
                warningString =
                    _("THE SELECTED SYSTEM DOES NOT HAVE A PLATFORM SET, RESULTS MAY BE "
                      "INACCURATE");
            }
            else {
                warningString = _("AT LEAST ONE OF YOUR SELECTED SYSTEMS DOES NOT HAVE A PLATFORM "
                                  "SET, RESULTS MAY BE INACCURATE");
            }
            mWindow->pushGui(
                new GuiMsgBox(getHelpStyle(), Utils::String::toUpper(warningString), _("PROCEED"),
                              std::bind(&GuiScraperMenu::start, this), _("CANCEL"), nullptr, "",
                              nullptr, nullptr, false, true,
                              (mRenderer->getIsVerticalOrientation() ?
                                   0.80f :
                                   0.50f * (1.778f / mRenderer->getScreenAspectRatio()))));
            return;
        }
    }
    start();
}

void GuiScraperMenu::start()
{
    if (mSystems->getSelectedObjects().empty()) {
        mWindow->pushGui(
            new GuiMsgBox(getHelpStyle(), _("PLEASE SELECT AT LEAST ONE SYSTEM TO SCRAPE")));
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
        // ScreenScraper controller scraping is currently broken, it's unclear if they will fix it.
        // if (scraperService == "screenscraper" &&
        //    Settings::getInstance()->getBool("ScrapeControllers")) {
        //    contentToScrape = true;
        //    break;
        // }
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
        if (scraperService == "screenscraper" &&
            Settings::getInstance()->getBool("ScrapeManuals")) {
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
            new GuiMsgBox(getHelpStyle(), _("PLEASE SELECT AT LEAST ONE CONTENT TYPE TO SCRAPE")));
        return;
    }

    auto searches = getSearches(mSystems->getSelectedObjects(), mFilters->getSelected());

    if (searches.first.empty()) {
        mWindow->pushGui(
            new GuiMsgBox(getHelpStyle(), _("ALL GAMES WERE FILTERED, NOTHING TO SCRAPE")));
    }
    else {
        GuiScraperMulti* gsm {
            new GuiScraperMulti(searches, Settings::getInstance()->getBool("ScraperInteractive"))};
        mWindow->pushGui(gsm);
        mMenu.setCursorToList();
        mMenu.setCursorToFirstListEntry();
    }
}

std::pair<std::queue<ScraperSearchParams>, std::map<SystemData*, int>> GuiScraperMenu::getSearches(
    std::vector<SystemData*> systems, GameFilterFunc selector)
{
    std::pair<std::queue<ScraperSearchParams>, std::map<SystemData*, int>> queue;

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

                ++queue.second[*sys];
                queue.first.push(search);
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
        std::shared_ptr<ImageComponent> bracket {mMenu.makeArrow()};
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
    prompts.push_back(HelpPrompt("b", _("back")));
    prompts.push_back(HelpPrompt("y", _("start scraper")));
    return prompts;
}

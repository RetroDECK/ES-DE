//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMenu.cpp
//
//  Main menu.
//  Some submenus are covered in separate source files.
//

#include "guis/GuiMenu.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiComplexTextEditPopup.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiScreensaverOptions.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperMenu.h"
#include "guis/GuiSettings.h"
#include "views/gamelist/IGameListView.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "EmulationStation.h"
#include "FileSorts.h"
#include "Platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"

#include <algorithm>
#include <SDL2/SDL_events.h>

GuiMenu::GuiMenu(Window* window) : GuiComponent(window),
        mMenu(window, "MAIN MENU"), mVersion(window)
{
    bool isFullUI = UIModeController::getInstance()->isUIModeFull();

    if (isFullUI)
        addEntry("SCRAPER", 0x777777FF, true, [this] { openScraperSettings(); });

    if (isFullUI)
        addEntry("UI SETTINGS", 0x777777FF, true, [this] { openUISettings(); });

    addEntry("SOUND SETTINGS", 0x777777FF, true, [this] { openSoundSettings(); });

    if (isFullUI)
        addEntry("GAME COLLECTION SETTINGS", 0x777777FF, true, [this] {
                openCollectionSystemSettings(); });

    if (isFullUI)
        addEntry("OTHER SETTINGS", 0x777777FF, true, [this] { openOtherSettings(); });

    if (isFullUI)
        addEntry("CONFIGURE INPUT", 0x777777FF, true, [this] { openConfigInput(); });

    addEntry("QUIT", 0x777777FF, true, [this] {openQuitMenu(); });

    addChild(&mMenu);
    addVersionInfo();
    setSize(mMenu.getSize());
    setPosition((Renderer::getScreenWidth() - mSize.x()) / 2,
            Renderer::getScreenHeight() * 0.15f);
}

GuiMenu::~GuiMenu()
{
    // This is required for the situation where scrolling started just before the menu
    // was openened. Without this, the scrolling would run until manually stopped after
    // the menu has been closed.
    ViewController::get()->stopScrolling();
}

void GuiMenu::openScraperSettings()
{
    // Open the scrape menu.
    mWindow->pushGui(new GuiScraperMenu(mWindow));
}

void GuiMenu::openUISettings()
{
    auto s = new GuiSettings(mWindow, "UI SETTINGS");

    // Optionally start in selected system/gamelist.
    auto startup_system = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "GAMELIST ON STARTUP", false);
    startup_system->add("NONE", "", Settings::getInstance()->getString("StartupSystem") == "");
    for (auto it = SystemData::sSystemVector.cbegin();
            it != SystemData::sSystemVector.cend(); it++) {
        if ("retropie" != (*it)->getName()) {
            startup_system->add((*it)->getName(), (*it)->getName(),
                    Settings::getInstance()->getString("StartupSystem") == (*it)->getName());
        }
    }
    s->addWithLabel("GAMELIST TO SHOW ON STARTUP", startup_system);
    s->addSaveFunc([startup_system, s] {
        if (startup_system->getSelected() != Settings::getInstance()->getString("StartupSystem")) {
            Settings::getInstance()->setString("StartupSystem", startup_system->getSelected());
            s->setNeedsSaving();
        }
    });

    // GameList view style.
    auto gamelist_view_style = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "GAMELIST VIEW STYLE", false);
    std::vector<std::string> styles;
    styles.push_back("automatic");
    styles.push_back("basic");
    styles.push_back("detailed");
    styles.push_back("video");
    styles.push_back("grid");
    for (auto it = styles.cbegin(); it != styles.cend(); it++)
        gamelist_view_style->add(*it, *it, Settings::getInstance()->
                getString("GamelistViewStyle") == *it);
    s->addWithLabel("GAMELIST VIEW STYLE", gamelist_view_style);
    s->addSaveFunc([gamelist_view_style, s] {
        if (gamelist_view_style->getSelected() !=
                Settings::getInstance()->getString("GamelistViewStyle")) {
            Settings::getInstance()->setString("GamelistViewStyle",
                    gamelist_view_style->getSelected());
            s->setNeedsSaving();
            s->setNeedsReloading();
        }
    });

    // Transition style.
    auto transition_style = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "TRANSITION STYLE", false);
    std::vector<std::string> transitions;
    transitions.push_back("fade");
    transitions.push_back("slide");
    transitions.push_back("instant");
    for (auto it = transitions.cbegin(); it != transitions.cend(); it++)
        transition_style->add(*it, *it, Settings::getInstance()->
                getString("TransitionStyle") == *it);
    s->addWithLabel("TRANSITION STYLE", transition_style);
    s->addSaveFunc([transition_style, s] {
        if (transition_style->getSelected() !=
                Settings::getInstance()->getString("TransitionStyle")) {
            Settings::getInstance()->setString("TransitionStyle", transition_style->getSelected());
            s->setNeedsSaving();
        }
    });

    // Theme selection.
    auto themeSets = ThemeData::getThemeSets();
    if (!themeSets.empty()) {
        std::map<std::string, ThemeSet>::const_iterator selectedSet =
                themeSets.find(Settings::getInstance()->getString("ThemeSet"));
        if (selectedSet == themeSets.cend())
            selectedSet = themeSets.cbegin();
        auto theme_set = std::make_shared<OptionListComponent<std::string>>
                (mWindow, getHelpStyle(), "THEME SET", false);
        for (auto it = themeSets.cbegin(); it != themeSets.cend(); it++)
            theme_set->add(it->first, it->first, it == selectedSet);
        s->addWithLabel("THEME SET", theme_set);
        s->addSaveFunc([theme_set, s] {
            if (theme_set->getSelected() != Settings::getInstance()->getString("ThemeSet")) {
                Scripting::fireEvent("theme-changed", theme_set->getSelected(),
                        Settings::getInstance()->getString("ThemeSet"));
                CollectionSystemManager::get()->updateSystemsList();
                Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());
                s->setNeedsSaving();
                s->setNeedsGoToStart();
                s->setNeedsReloading();
            }
        });
    }

    // UI mode.
    auto ui_mode = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "UI MODE", false);
    std::vector<std::string> uiModes;
    uiModes.push_back("full");
    uiModes.push_back("kiosk");
    uiModes.push_back("kid");
    for (auto it = uiModes.cbegin(); it != uiModes.cend(); it++)
        ui_mode->add(*it, *it, Settings::getInstance()->getString("UIMode") == *it);
    s->addWithLabel("UI MODE", ui_mode);
    s->addSaveFunc([ui_mode, this, s] {
        std::string selectedMode = ui_mode->getSelected();
        if (selectedMode != Settings::getInstance()->getString("UIMode") &&
                selectedMode != "full") {
            std::string msg = "YOU ARE CHANGING THE UI TO A RESTRICTED MODE:\n'" +
                    Utils::String::toUpper(selectedMode) + "'\n";
            msg += "THIS WILL HIDE MOST MENU OPTIONS TO PREVENT CHANGES TO THE SYSTEM.\n";
            msg += "TO UNLOCK AND RETURN TO THE FULL UI, ENTER THIS CODE: \n";
            msg += "\"" + UIModeController::getInstance()->getFormattedPassKeyStr() + "\"\n\n";
            msg += "DO YOU WANT TO PROCEED?";
            mWindow->pushGui(new GuiMsgBox(mWindow, this->getHelpStyle(), msg,
                    "YES", [selectedMode, s] {
                LOG(LogDebug) << "GuiMenu::openUISettings(): Setting UI mode to '"
                    << selectedMode << "'.";
                Settings::getInstance()->setString("UIMode", selectedMode);
                Settings::getInstance()->saveFile();
            }, "NO", nullptr));
        }
        else if (ui_mode->getSelected() != Settings::getInstance()->getString("UIMode") &&
                selectedMode == "full") {
            LOG(LogDebug) << "GuiMenu::openUISettings(): Setting UI mode to '" <<
                    selectedMode << "'.";
            Settings::getInstance()->setString("UIMode", ui_mode->getSelected());
            s->setNeedsSaving();
        }
    });

    // Default gamelist sort order.
    typedef OptionListComponent<const FileData::SortType*> SortList;
    std::string sortOrder;
    auto default_sort_order = std::make_shared<SortList>
            (mWindow, getHelpStyle(), "DEFAULT SORT ORDER", false);
    for (auto it = FileSorts::SortTypes.cbegin(); it != FileSorts::SortTypes.cend(); it++) {
        if (it->description == Settings::getInstance()->getString("DefaultSortOrder")) {
            sortOrder = it->description;
            break;
        }
    }
    // If an invalid sort order was defined in es_settings.cfg, then apply the default
    // sort order 'filename, ascending'.
    if (sortOrder == "")
        sortOrder = "filename, ascending";
    for (auto it = FileSorts::SortTypes.cbegin(); it != FileSorts::SortTypes.cend(); it++) {
        const FileData::SortType& sort = *it;
        if (sort.description == sortOrder)
            default_sort_order->add(sort.description, &sort, true);
        else
            default_sort_order->add(sort.description, &sort, false);
    }
    s->addWithLabel("DEFAULT SORT ORDER", default_sort_order);
    s->addSaveFunc([default_sort_order, sortOrder, s] {
        std::string selectedSortOrder = default_sort_order.get()->getSelected()->description;
        if (selectedSortOrder != sortOrder) {
            Settings::getInstance()->setString("DefaultSortOrder", selectedSortOrder);
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setNeedsSortingCollections();
        }
    });

    #if defined(USE_OPENGL_21)
    // Open menu effect.
    auto menu_opening_effect = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "MENU OPENING EFFECT", false);
    std::vector<std::string> menu_effects;
    menu_effects.push_back("scale-up");
    menu_effects.push_back("fade-in");
    menu_effects.push_back("none");
    for (auto it = menu_effects.cbegin(); it != menu_effects.cend(); it++)
        menu_opening_effect->add(*it, *it, Settings::getInstance()->
                getString("MenuOpeningEffect") == *it);
    s->addWithLabel("MENU OPENING EFFECT", menu_opening_effect);
    s->addSaveFunc([menu_opening_effect, s] {
        if (menu_opening_effect->getSelected() !=
                Settings::getInstance()->getString("MenuOpeningEffect")) {
            Settings::getInstance()->setString("MenuOpeningEffect",
                    menu_opening_effect->getSelected());
            s->setNeedsSaving();
        }
    });
    #endif

    // Carousel transitions.
    auto carousel_transitions = std::make_shared<SwitchComponent>(mWindow);
    carousel_transitions->setState(Settings::getInstance()->getBool("CarouselTransitions"));
    s->addWithLabel("DISPLAY CAROUSEL TRANSITIONS", carousel_transitions);
    s->addSaveFunc([carousel_transitions, s] {
        if (carousel_transitions->getState() !=
                Settings::getInstance()->getBool("CarouselTransitions")) {
            Settings::getInstance()->setBool("CarouselTransitions",
                    carousel_transitions->getState());
            s->setNeedsSaving();
        }
    });

    #if defined(USE_OPENGL_21)
    // Render scanlines for videos in the gamelists.
    auto gamelist_video_scanlines = std::make_shared<SwitchComponent>(mWindow);
    gamelist_video_scanlines->setState(Settings::getInstance()->getBool("GamelistVideoScanlines"));
    s->addWithLabel("RENDER SCANLINES FOR GAMELIST VIDEOS", gamelist_video_scanlines);
    s->addSaveFunc([gamelist_video_scanlines, s] {
        if (gamelist_video_scanlines->getState() !=
                Settings::getInstance()->getBool("GamelistVideoScanlines")) {
            Settings::getInstance()->setBool("GamelistVideoScanlines",
                    gamelist_video_scanlines->getState());
            s->setNeedsSaving();
        }
    });
    #endif

    // Sort folders on top of the gamelists.
    auto folders_on_top = std::make_shared<SwitchComponent>(mWindow);
    folders_on_top->setState(Settings::getInstance()->getBool("FoldersOnTop"));
    s->addWithLabel("SORT FOLDERS ON TOP OF GAMELISTS", folders_on_top);
    s->addSaveFunc([folders_on_top, s] {
        if (folders_on_top->getState() !=
                Settings::getInstance()->getBool("FoldersOnTop")) {
            Settings::getInstance()->setBool("FoldersOnTop", folders_on_top->getState());
            s->setNeedsSaving();
            s->setNeedsSorting();
        }
    });

    // Sort favorites on top of non-favorites in the gamelists.
    auto favorites_first = std::make_shared<SwitchComponent>(mWindow);
    favorites_first->setState(Settings::getInstance()->getBool("FavoritesFirst"));
    s->addWithLabel("SORT FAVORITE GAMES ABOVE NON-FAVORITES", favorites_first);
    s->addSaveFunc([favorites_first,s ] {
        if (favorites_first->getState() !=
                Settings::getInstance()->getBool("FavoritesFirst")) {
            Settings::getInstance()->setBool("FavoritesFirst", favorites_first->getState());
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setNeedsSortingCollections();
        }
    });

    // Enable gamelist star markings for favorite games.
    auto favorites_star = std::make_shared<SwitchComponent>(mWindow);
    favorites_star->setState(Settings::getInstance()->getBool("FavoritesStar"));
    s->addWithLabel("ADD STAR MARKINGS TO FAVORITE GAMES", favorites_star);
    s->addSaveFunc([favorites_star, s] {
        if (favorites_star->getState() != Settings::getInstance()->getBool("FavoritesStar")) {
            Settings::getInstance()->setBool("FavoritesStar", favorites_star->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
        }
    });

    // Enable the 'Y' button for tagging games as favorites.
    auto favorites_add_button = std::make_shared<SwitchComponent>(mWindow);
    favorites_add_button->setState(Settings::getInstance()->getBool("FavoritesAddButton"));
    s->addWithLabel("ENABLE BUTTON SHORTCUT TO TOGGLE FAVORITES", favorites_add_button);
    s->addSaveFunc([favorites_add_button, s] {
        if (Settings::getInstance()->getBool("FavoritesAddButton") !=
                favorites_add_button->getState()) {
            Settings::getInstance()->setBool("FavoritesAddButton",
                    favorites_add_button->getState());
            s->setNeedsSaving();
        }
    });

    // Gamelist filters.
    auto gamelist_filters = std::make_shared<SwitchComponent>(mWindow);
    gamelist_filters->setState(Settings::getInstance()->getBool("GamelistFilters"));
    s->addWithLabel("ENABLE GAMELIST FILTERS", gamelist_filters);
    s->addSaveFunc([gamelist_filters, s] {
        if (Settings::getInstance()->getBool("GamelistFilters") !=
                gamelist_filters->getState()) {
            Settings::getInstance()->setBool("GamelistFilters", gamelist_filters->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
        }
    });

    // Quick system select (navigate left/right in gamelist view).
    auto quick_system_select = std::make_shared<SwitchComponent>(mWindow);
    quick_system_select->setState(Settings::getInstance()->getBool("QuickSystemSelect"));
    s->addWithLabel("ENABLE QUICK SYSTEM SELECT", quick_system_select);
    s->addSaveFunc([quick_system_select, s] {
        if (Settings::getInstance()->getBool("QuickSystemSelect") !=
                quick_system_select->getState()) {
            Settings::getInstance()->setBool("QuickSystemSelect", quick_system_select->getState());
            s->setNeedsSaving();
        }
    });

    // On-screen help prompts.
    auto show_help_prompts = std::make_shared<SwitchComponent>(mWindow);
    show_help_prompts->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
    s->addWithLabel("DISPLAY ON-SCREEN HELP", show_help_prompts);
    s->addSaveFunc([show_help_prompts, s] {
        if (Settings::getInstance()->getBool("ShowHelpPrompts") != show_help_prompts->getState()) {
            Settings::getInstance()->setBool("ShowHelpPrompts", show_help_prompts->getState());
            s->setNeedsSaving();
        }
    });

    // Play videos immediately (overrides theme setting).
    auto play_videos_immediately = std::make_shared<SwitchComponent>(mWindow);
    play_videos_immediately->setState(Settings::getInstance()->getBool("PlayVideosImmediately"));
    s->addWithLabel("PLAY VIDEOS IMMEDIATELY (OVERRIDE THEME)", play_videos_immediately);
    s->addSaveFunc([play_videos_immediately, s] {
        if (Settings::getInstance()->getBool("PlayVideosImmediately") !=
                play_videos_immediately->getState()) {
            Settings::getInstance()->setBool("PlayVideosImmediately",
                    play_videos_immediately->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to show start menu in Kid mode.
    auto show_kid_start_menu = std::make_shared<SwitchComponent>(mWindow);
    show_kid_start_menu->setState(Settings::getInstance()->getBool("ShowKidStartMenu"));
    s->addWithLabel("SHOW START MENU IN KID MODE", show_kid_start_menu);
    s->addSaveFunc([show_kid_start_menu, s] {
        if (Settings::getInstance()->getBool("ShowKidStartMenu") !=
                show_kid_start_menu->getState()) {
            Settings::getInstance()->setBool("ShowKidStartMenu", show_kid_start_menu->getState());
            s->setNeedsSaving();
        }
    });

    // Screensaver.
    ComponentListRow screensaver_row;
    screensaver_row.elements.clear();
    screensaver_row.addElement(std::make_shared<TextComponent>
            (mWindow, "SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
    screensaver_row.addElement(makeArrow(mWindow), false);
    screensaver_row.makeAcceptInputHandler(std::bind(&GuiMenu::openScreensaverOptions, this));
    s->addRow(screensaver_row);

    mWindow->pushGui(s);
}

void GuiMenu::openSoundSettings()
{
    auto s = new GuiSettings(mWindow, "SOUND SETTINGS");

    // TEMPORARY - Hide the volume slider on macOS until the volume control logic
    // has been implemented for this operating system.
    #if !defined(__APPLE__)
    // System volume.
    auto system_volume = std::make_shared<SliderComponent>(mWindow, 0.f, 100.f, 1.f, "%");
    system_volume->setValue(static_cast<float>(VolumeControl::getInstance()->getVolume()));
    s->addWithLabel("SYSTEM VOLUME", system_volume);
    s->addSaveFunc([system_volume] {
        VolumeControl::getInstance()->
                setVolume(static_cast<int>(Math::round(system_volume->getValue())));
    });
    #endif

    if (UIModeController::getInstance()->isUIModeFull()) {
    // The ALSA Audio Card and Audio Device selection code is disabled at the moment.
    // As PulseAudio controls the sound devices for the desktop environment, it doesn't
    // make much sense to be able to select ALSA devices directly. Normally (always?)
    // the selection doesn't make any difference at all. But maybe some PulseAudio
    // settings could be added later on, if needed.
    // The code is still active for Raspberry Pi though as I'm not sure if this is
    // useful for that device.
    //    #if defined(__linux__)
        #if defined(_RPI_)
        // Audio card.
        auto audio_card = std::make_shared<OptionListComponent<std::string>>
                (mWindow, getHelpStyle(), "AUDIO CARD", false);
        std::vector<std::string> audio_cards;
        #if defined(_RPI_)
        // RPi Specific Audio Cards.
        audio_cards.push_back("local");
        audio_cards.push_back("hdmi");
        audio_cards.push_back("both");
        #endif
        audio_cards.push_back("default");
        audio_cards.push_back("sysdefault");
        audio_cards.push_back("dmix");
        audio_cards.push_back("hw");
        audio_cards.push_back("plughw");
        audio_cards.push_back("null");
        if (Settings::getInstance()->getString("AudioCard") != "") {
            if (std::find(audio_cards.begin(), audio_cards.end(),
                    Settings::getInstance()->getString("AudioCard")) == audio_cards.end()) {
                audio_cards.push_back(Settings::getInstance()->getString("AudioCard"));
            }
        }
        for (auto ac = audio_cards.cbegin(); ac != audio_cards.cend(); ac++)
            audio_card->add(*ac, *ac, Settings::getInstance()->getString("AudioCard") == *ac);
        s->addWithLabel("AUDIO CARD", audio_card);
        s->addSaveFunc([audio_card, s] {
            if (audio_card->getSelected() != Settings::getInstance()->getString("AudioCard")) {
                Settings::getInstance()->setString("AudioCard", audio_card->getSelected());
                VolumeControl::getInstance()->deinit();
                VolumeControl::getInstance()->init();
                s->setNeedsSaving();
            }
        });

        // Volume control device.
        auto vol_dev = std::make_shared<OptionListComponent<std::string>>
                (mWindow, getHelpStyle(), "AUDIO DEVICE", false);
        std::vector<std::string> transitions;
        transitions.push_back("PCM");
        transitions.push_back("Speaker");
        transitions.push_back("Master");
        transitions.push_back("Digital");
        transitions.push_back("Analogue");
        if (Settings::getInstance()->getString("AudioDevice") != "") {
            if (std::find(transitions.begin(), transitions.end(),
                    Settings::getInstance()->getString("AudioDevice")) == transitions.end()) {
                transitions.push_back(Settings::getInstance()->getString("AudioDevice"));
            }
        }
        for (auto it = transitions.cbegin(); it != transitions.cend(); it++)
            vol_dev->add(*it, *it, Settings::getInstance()->getString("AudioDevice") == *it);
        s->addWithLabel("AUDIO DEVICE", vol_dev);
        s->addSaveFunc([vol_dev, s] {
            if (vol_dev->getSelected() != Settings::getInstance()->getString("AudioDevice")) {
                Settings::getInstance()->setString("AudioDevice", vol_dev->getSelected());
                VolumeControl::getInstance()->deinit();
                VolumeControl::getInstance()->init();
                s->setNeedsSaving();
            }
        });
        #endif

        #if defined(_RPI_)
        // OMX player Audio Device
        auto omx_audio_dev = std::make_shared<OptionListComponent<std::string>>
                (mWindow, getHelpStyle(), "OMX PLAYER AUDIO DEVICE", false);
        std::vector<std::string> omx_cards;
        // RPi Specific  Audio Cards
        omx_cards.push_back("local");
        omx_cards.push_back("hdmi");
        omx_cards.push_back("both");
        omx_cards.push_back("alsa:hw:0,0");
        omx_cards.push_back("alsa:hw:1,0");
        if (Settings::getInstance()->getString("OMXAudioDev") != "") {
            if (std::find(omx_cards.begin(), omx_cards.end(),
                    Settings::getInstance()->getString("OMXAudioDev")) == omx_cards.end()) {
                omx_cards.push_back(Settings::getInstance()->getString("OMXAudioDev"));
            }
        }
        for (auto it = omx_cards.cbegin(); it != omx_cards.cend(); it++)
            omx_audio_dev->add(*it, *it, Settings::getInstance()->getString("OMXAudioDev") == *it);
        s->addWithLabel("OMX PLAYER AUDIO DEVICE", omx_audio_dev);
        s->addSaveFunc([omx_audio_dev, s] {
            if (omx_audio_dev->getSelected() !=
                    Settings::getInstance()->getString("OMXAudioDev")) {
                Settings::getInstance()->setString("OMXAudioDev", omx_audio_dev->getSelected());
                s->setNeedsSaving();
            }
        });
        #endif

        // Video audio.
        auto gamelist_video_audio = std::make_shared<SwitchComponent>(mWindow);
        gamelist_video_audio->setState(Settings::getInstance()->getBool("GamelistVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR VIDEO FILES IN GAMELIST VIEWS", gamelist_video_audio);
        s->addSaveFunc([gamelist_video_audio, s] {
            if (gamelist_video_audio->getState() !=
                    Settings::getInstance()->getBool("GamelistVideoAudio")) {
                Settings::getInstance()->setBool("GamelistVideoAudio",
                        gamelist_video_audio->getState());
                s->setNeedsSaving();
            }
        });

        // Navigation sounds.
        auto navigation_sounds = std::make_shared<SwitchComponent>(mWindow);
        navigation_sounds->setState(Settings::getInstance()->
                getBool("NavigationSounds"));
        s->addWithLabel("ENABLE NAVIGATION SOUNDS", navigation_sounds);
        s->addSaveFunc([navigation_sounds, s] {
            if (navigation_sounds->getState() !=
                    Settings::getInstance()->getBool("NavigationSounds")) {
                Settings::getInstance()->setBool("NavigationSounds",
                        navigation_sounds->getState());
                s->setNeedsSaving();
            }
        });
    }

    mWindow->pushGui(s);
}

void GuiMenu::openOtherSettings()
{
    auto s = new GuiSettings(mWindow, "OTHER SETTINGS");

    // Maximum VRAM.
    auto max_vram = std::make_shared<SliderComponent>(mWindow, 80.f, 1024.f, 8.f, "MiB");
    max_vram->setValue(static_cast<float>(Settings::getInstance()->getInt("MaxVRAM")));
    s->addWithLabel("VRAM LIMIT", max_vram);
    s->addSaveFunc([max_vram, s] {
        if (max_vram->getValue() != Settings::getInstance()->getInt("MaxVRAM")) {
            Settings::getInstance()->setInt("MaxVRAM",
                    static_cast<int>(Math::round(max_vram->getValue())));
            s->setNeedsSaving();
        }
    });

    #if defined(__unix__)
    // Fullscreen mode.
    auto fullscreen_mode = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "FULLSCREEN MODE", false);
    std::vector<std::string> screenmode;
    screenmode.push_back("normal");
    screenmode.push_back("borderless");
    for (auto it = screenmode.cbegin(); it != screenmode.cend(); it++)
        fullscreen_mode->add(*it, *it, Settings::getInstance()->getString("FullscreenMode") == *it);
    s->addWithLabel("FULLSCREEN MODE (REQUIRES RESTART)", fullscreen_mode);
    s->addSaveFunc([fullscreen_mode, s] {
        if (fullscreen_mode->getSelected() !=
                Settings::getInstance()->getString("FullscreenMode")) {
            Settings::getInstance()->setString("FullscreenMode", fullscreen_mode->getSelected());
            s->setNeedsSaving();
        }
    });
    #endif

    // When to save game metadata.
    auto save_gamelist_mode = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "WHEN TO SAVE METADATA", false);
    std::vector<std::string> saveModes;
    saveModes.push_back("on exit");
    saveModes.push_back("always");
    saveModes.push_back("never");
    for (auto it = saveModes.cbegin(); it != saveModes.cend(); it++) {
        save_gamelist_mode->add(*it, *it, Settings::getInstance()->
                getString("SaveGamelistsMode") == *it);
    }
    s->addWithLabel("WHEN TO SAVE GAME METADATA", save_gamelist_mode);
    s->addSaveFunc([save_gamelist_mode, s] {
        if (save_gamelist_mode->getSelected() !=
                Settings::getInstance()->getString("SaveGamelistsMode")) {
            Settings::getInstance()->setString("SaveGamelistsMode",
                    save_gamelist_mode->getSelected());
            // Always save the gamelist.xml files if switching to 'always' as there may
            // be changes that will otherwise be lost.
            if (Settings::getInstance()->getString("SaveGamelistsMode") == "always") {
                for (auto it = SystemData::sSystemVector.cbegin();
                        it != SystemData::sSystemVector.cend(); it++)
                    (*it)->writeMetaData();
            }
            s->setNeedsSaving();
        }
    });

    // Game media directory.
    ComponentListRow row;
    auto media_directory = std::make_shared<TextComponent>(mWindow, "GAME MEDIA DIRECTORY",
            Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
    auto bracket = std::make_shared<ImageComponent>(mWindow);
    bracket->setImage(":/graphics/arrow.svg");
    bracket->setResize(Vector2f(0, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()));
    row.addElement(media_directory, true);
    row.addElement(bracket, false);
    std::string title = "ENTER GAME MEDIA DIRECTORY";
    std::string mediaDirectoryStaticText = "Default directory:";
    std::string defaultDirectoryText = "~/.emulationstation/downloaded_media/";
    std::string initValue = Settings::getInstance()->getString("MediaDirectory");
    bool multiLine = false;
    auto updateVal = [this](const std::string& newVal) {
        Settings::getInstance()->setString("MediaDirectory", newVal);
        Settings::getInstance()->saveFile();
        ViewController::get()->reloadAll();
        mWindow->invalidateCachedBackground();
    };
    row.makeAcceptInputHandler([this, title, mediaDirectoryStaticText,
            defaultDirectoryText, initValue, updateVal, multiLine] {
                mWindow->pushGui(new GuiComplexTextEditPopup(mWindow, getHelpStyle(),
                title, mediaDirectoryStaticText, defaultDirectoryText,
                Settings::getInstance()->getString("MediaDirectory"),
                updateVal, multiLine, "SAVE", "SAVE CHANGES?"));
    });
    s->addRow(row);

    #if defined(_RPI_)
    // Video Player - VideoOmxPlayer.
    auto video_omx_player = std::make_shared<SwitchComponent>(mWindow);
    video_omx_player->setState(Settings::getInstance()->getBool("VideoOmxPlayer"));
    s->addWithLabel("USE OMX PLAYER (HW ACCELERATED)", video_omx_player);
    s->addSaveFunc([video_omx_player, s] {
        if (video_omx_player->getState() !=
                Settings::getInstance()->getBool("VideoOmxPlayer")) {
            Settings::getInstance()->setBool("VideoOmxPlayer", video_omx_player->getState());
            s->setNeedsSaving();
            // Need to reload all views to re-create the right video components.
            s->setNeedsReloading();
        }
    });
    #endif

    #if defined(_WIN64)
    // Hide taskbar during the ES program session.
    auto hide_taskbar = std::make_shared<SwitchComponent>(mWindow);
    hide_taskbar->setState(Settings::getInstance()->getBool("HideTaskbar"));
    s->addWithLabel("HIDE TASKBAR (REQUIRES RESTART)", hide_taskbar);
    s->addSaveFunc([hide_taskbar, s] {
        if (hide_taskbar->getState() !=
                Settings::getInstance()->getBool("HideTaskbar")) {
            Settings::getInstance()-> setBool("HideTaskbar", hide_taskbar->getState());
            s->setNeedsSaving();
        }
    });

    // Run ES in the background when a game has been launched.
    auto run_in_background = std::make_shared<SwitchComponent>(mWindow);
    run_in_background->setState(Settings::getInstance()->getBool("RunInBackground"));
    s->addWithLabel("RUN IN BACKGROUND (WHILE GAME IS LAUNCHED)", run_in_background);
    s->addSaveFunc([run_in_background,s] {
        if (run_in_background->getState() != Settings::getInstance()->getBool("RunInBackground")) {
            Settings::getInstance()->setBool("RunInBackground", run_in_background->getState());
            s->setNeedsSaving();
        }
    });
    #endif

    // Allow overriding of the launch command per game (the option to disable this is
    // intended primarily for testing purposes).
    auto launchcommand_override = std::make_shared<SwitchComponent>(mWindow);
    launchcommand_override->setState(Settings::getInstance()->getBool("LaunchCommandOverride"));
    s->addWithLabel("PER GAME LAUNCH COMMAND OVERRIDE", launchcommand_override);
    s->addSaveFunc([launchcommand_override, s] {
        if (launchcommand_override->getState() !=
                Settings::getInstance()->getBool("LaunchCommandOverride")) {
            Settings::getInstance()->
                    setBool("LaunchCommandOverride", launchcommand_override->getState());
            s->setNeedsSaving();
        }
    });

    // Show hidden files.
    auto show_hidden_files = std::make_shared<SwitchComponent>(mWindow);
    show_hidden_files->setState(Settings::getInstance()->getBool("ShowHiddenFiles"));
    s->addWithLabel("SHOW HIDDEN FILES AND FOLDERS (REQUIRES RESTART)", show_hidden_files);
    s->addSaveFunc([show_hidden_files, s] {
        if (show_hidden_files->getState() != Settings::getInstance()->getBool("ShowHiddenFiles")) {
            Settings::getInstance()->setBool("ShowHiddenFiles", show_hidden_files->getState());
            s->setNeedsSaving();
        }
    });

    // Show hidden games.
    auto show_hidden_games = std::make_shared<SwitchComponent>(mWindow);
    show_hidden_games->setState(Settings::getInstance()->getBool("ShowHiddenGames"));
    s->addWithLabel("SHOW HIDDEN GAMES (REQUIRES RESTART)", show_hidden_games);
    s->addSaveFunc([show_hidden_games, s] {
        if (show_hidden_games->getState() != Settings::getInstance()->getBool("ShowHiddenGames")) {
            Settings::getInstance()->setBool("ShowHiddenGames", show_hidden_games->getState());
            s->setNeedsSaving();
        }
    });

    // Custom event scripts, fired using Scripting::fireEvent().
    auto custom_eventscripts = std::make_shared<SwitchComponent>(mWindow);
    custom_eventscripts->setState(Settings::getInstance()->getBool("CustomEventScripts"));
    s->addWithLabel("ENABLE CUSTOM EVENT SCRIPTS", custom_eventscripts);
    s->addSaveFunc([custom_eventscripts, s] {
        if (custom_eventscripts->getState() !=
                Settings::getInstance()->getBool("CustomEventScripts")) {
            Settings::getInstance()->setBool("CustomEventScripts", custom_eventscripts->getState());
            s->setNeedsSaving();
        }
    });

    // Only show ROMs included in the gamelist.xml files.
    auto parse_gamelist_only = std::make_shared<SwitchComponent>(mWindow);
    parse_gamelist_only->setState(Settings::getInstance()->getBool("ParseGamelistOnly"));
    s->addWithLabel("ONLY SHOW ROMS FROM GAMELIST.XML FILES", parse_gamelist_only);
    s->addSaveFunc([parse_gamelist_only, s] {
        if (parse_gamelist_only->getState() !=
                Settings::getInstance()->getBool("ParseGamelistOnly")) {
            Settings::getInstance()->setBool("ParseGamelistOnly", parse_gamelist_only->getState());
            s->setNeedsSaving();
        }
    });

    // Display game media from the ROM directories.
    auto rom_dir_game_media = std::make_shared<SwitchComponent>(mWindow);
    rom_dir_game_media->setState(Settings::getInstance()->getBool("ROMDirGameMedia"));
    s->addWithLabel("DISPLAY GAME MEDIA FROM ROM DIRECTORIES", rom_dir_game_media);
    s->addSaveFunc([rom_dir_game_media, s] {
        if (rom_dir_game_media->getState() != Settings::getInstance()->getBool("ROMDirGameMedia")) {
            Settings::getInstance()->setBool("ROMDirGameMedia", rom_dir_game_media->getState());
            s->setNeedsSaving();
        }
    });

    // GPU statistics overlay.
    auto display_gpu_statistics = std::make_shared<SwitchComponent>(mWindow);
    display_gpu_statistics->setState(Settings::getInstance()->getBool("DisplayGPUStatistics"));
    s->addWithLabel("DISPLAY GPU STATISTICS OVERLAY", display_gpu_statistics);
    s->addSaveFunc([display_gpu_statistics, s] {
        if (display_gpu_statistics->getState() !=
                Settings::getInstance()->getBool("DisplayGPUStatistics")) {
            Settings::getInstance()->setBool("DisplayGPUStatistics",
                    display_gpu_statistics->getState());
            s->setNeedsSaving();
        }
    });

    // macOS requires root privileges to reboot and power off so it doesn't make much
    // sense to enable these settings and menu entries for this operating system.
    #if !defined(__APPLE__)
    // Hide Reboot System option in the quit menu.
    auto show_reboot_system = std::make_shared<SwitchComponent>(mWindow);
    show_reboot_system->setState(Settings::getInstance()->getBool("ShowRebootSystem"));
    s->addWithLabel("SHOW 'REBOOT SYSTEM' MENU ENTRY", show_reboot_system);
    s->addSaveFunc([show_reboot_system, s] {
        if (show_reboot_system->getState() !=
                Settings::getInstance()->getBool("ShowRebootSystem")) {
        Settings::getInstance()->setBool("ShowRebootSystem", show_reboot_system->getState());
        s->setNeedsSaving();
        }
    });

    // Hide Power Off System option in the quit menu.
    auto show_poweroff_system = std::make_shared<SwitchComponent>(mWindow);
    show_poweroff_system->setState(Settings::getInstance()->getBool("ShowPoweroffSystem"));
    s->addWithLabel("SHOW 'POWER OFF SYSTEM' MENU ENTRY", show_poweroff_system);
    s->addSaveFunc([show_poweroff_system, s] {
        if (show_poweroff_system->getState() !=
                Settings::getInstance()->getBool("ShowPoweroffSystem")) {
        Settings::getInstance()->setBool("ShowPoweroffSystem", show_poweroff_system->getState());
        s->setNeedsSaving();
        }
    });
    #endif

    mWindow->pushGui(s);
}

void GuiMenu::openConfigInput()
{
    Window* window = mWindow;
    window->pushGui(new GuiMsgBox(window, getHelpStyle(),
            "ARE YOU SURE YOU WANT TO CONFIGURE INPUT?", "YES", [window] {
        window->pushGui(new GuiDetectDevice(window, false, false, nullptr));
    }, "NO", nullptr)
    );
}

void GuiMenu::openQuitMenu()
{
    auto s = new GuiSettings(mWindow, "QUIT");

    Window* window = mWindow;
    HelpStyle style = getHelpStyle();

    ComponentListRow row;
    if (UIModeController::getInstance()->isUIModeFull()) {
        if (Settings::getInstance()->getBool("ShowExit")) {
            row.makeAcceptInputHandler([window, this] {
                window->pushGui(new GuiMsgBox(window, this->getHelpStyle(),
                    "REALLY QUIT?", "YES", [] {
                        Scripting::fireEvent("quit");
                        quitES();
                }, "NO", nullptr));
            });
            row.addElement(std::make_shared<TextComponent>(window, "QUIT EMULATIONSTATION",
                    Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
            s->addRow(row);
        }
    }

    // macOS requires root privileges to reboot and power off so it doesn't make much
    // sense to enable these settings and menu entries for this operating system.
    #if !defined(__APPLE__)
    if (Settings::getInstance()->getBool("ShowRebootSystem")) {
        row.elements.clear();
        row.makeAcceptInputHandler([window, this] {
            window->pushGui(new GuiMsgBox(window, this->getHelpStyle(),
                    "REALLY REBOOT?", "YES", [] {
                    Scripting::fireEvent("quit", "reboot");
                    Scripting::fireEvent("reboot");
                    if (quitES(QuitMode::REBOOT) != 0) {
                        LOG(LogWarning) << "Reboot terminated with non-zero result!";
                    }
            }, "NO", nullptr));
        });
        row.addElement(std::make_shared<TextComponent>(window, "REBOOT SYSTEM",
                Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
        s->addRow(row);
    }

    if (Settings::getInstance()->getBool("ShowPoweroffSystem")) {
        row.elements.clear();
        row.makeAcceptInputHandler([window, this] {
            window->pushGui(new GuiMsgBox(window, this->getHelpStyle(),
                    "REALLY POWER OFF?", "YES", [] {
                    Scripting::fireEvent("quit", "poweroff");
                    Scripting::fireEvent("poweroff");
                    if (quitES(QuitMode::POWEROFF) != 0) {
                        LOG(LogWarning) << "Power off terminated with non-zero result!";
                    }
            }, "NO", nullptr));
        });
        row.addElement(std::make_shared<TextComponent>(window, "POWER OFF SYSTEM",
                Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
        s->addRow(row);
    }
    #endif

    mWindow->pushGui(s);
}

void GuiMenu::addVersionInfo()
{
    mVersion.setFont(Font::get(FONT_SIZE_SMALL));
    mVersion.setColor(0x5E5E5EFF);
    mVersion.setText("EMULATIONSTATION-DE  V" + Utils::String::toUpper(PROGRAM_VERSION_STRING));
    mVersion.setHorizontalAlignment(ALIGN_CENTER);
    addChild(&mVersion);
}

void GuiMenu::openScreensaverOptions() {
    mWindow->pushGui(new GuiScreensaverOptions(mWindow, "SCREENSAVER SETTINGS"));
}

void GuiMenu::openCollectionSystemSettings() {
    mWindow->pushGui(new GuiCollectionSystemsOptions(mWindow));
}

void GuiMenu::onSizeChanged()
{
    mVersion.setSize(mSize.x(), 0);
    mVersion.setPosition(0, mSize.y() - mVersion.getSize().y());
}

void GuiMenu::addEntry(const char* name, unsigned int color,
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

void GuiMenu::close(bool closeAllWindows)
{
    std::function<void()> closeFunc;
    if (!closeAllWindows) {
        closeFunc = [this] { delete this; };
    }
    else {
        Window* window = mWindow;
        closeFunc = [window, this] {
            while (window->peekGui() != ViewController::get())
                delete window->peekGui();
        };
    }
    closeFunc();
}

bool GuiMenu::input(InputConfig* config, Input input)
{
    if (GuiComponent::input(config, input))
        return true;

    const bool isStart = config->isMappedTo("start", input);
    if (input.value != 0 && (config->isMappedTo("b", input) || isStart)) {
        close(isStart);
        return true;
    }

    return false;
}

std::vector<HelpPrompt> GuiMenu::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("up/down", "choose"));
    prompts.push_back(HelpPrompt("a", "select"));
    prompts.push_back(HelpPrompt("b", "close menu"));
    prompts.push_back(HelpPrompt("start", "close menu"));
    return prompts;
}

HelpStyle GuiMenu::getHelpStyle()
{
    HelpStyle style = HelpStyle();
    style.applyTheme(ViewController::get()->getState().getSystem()->getTheme(), "system");
    return style;
}

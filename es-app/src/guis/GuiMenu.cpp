//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMenu.cpp
//
//  Main menu.
//  Some submenus are covered in separate source files.
//

#include "guis/GuiMenu.h"

#if defined(_WIN64)
#include <winsock2.h>
#endif

#include "CollectionSystemsManager.h"
#include "EmulationStation.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Scripting.h"
#include "SystemData.h"
#include "UIModeController.h"
#include "VolumeControl.h"
#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiAlternativeEmulators.h"
#include "guis/GuiCollectionSystemsOptions.h"
#include "guis/GuiDetectDevice.h"
#include "guis/GuiMediaViewerOptions.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiScraperMenu.h"
#include "guis/GuiScreensaverOptions.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"
#include "utils/PlatformUtil.h"
#include "views/ViewController.h"

#include <SDL2/SDL_events.h>
#include <algorithm>

GuiMenu::GuiMenu()
    : mMenu {"MAIN MENU"}
{
    bool isFullUI = UIModeController::getInstance()->isUIModeFull();

    if (isFullUI)
        addEntry("SCRAPER", 0x777777FF, true, [this] { openScraperOptions(); });

    if (isFullUI)
        addEntry("UI SETTINGS", 0x777777FF, true, [this] { openUIOptions(); });

    addEntry("SOUND SETTINGS", 0x777777FF, true, [this] { openSoundOptions(); });

    if (isFullUI)
        addEntry("INPUT DEVICE SETTINGS", 0x777777FF, true, [this] { openInputDeviceOptions(); });

    if (isFullUI)
        addEntry("GAME COLLECTION SETTINGS", 0x777777FF, true,
                 [this] { openCollectionSystemOptions(); });

    if (isFullUI)
        addEntry("OTHER SETTINGS", 0x777777FF, true, [this] { openOtherOptions(); });

    // TEMPORARY: Disabled for now, will be used in the future.
    //    if (isFullUI)
    //        addEntry("UTILITIES", 0x777777FF, true, [this] {
    //                openUtilitiesMenu(); });

    if (!Settings::getInstance()->getBool("ForceKiosk") &&
        Settings::getInstance()->getString("UIMode") != "kiosk") {
        if (Settings::getInstance()->getBool("ShowQuitMenu"))
            addEntry("QUIT", 0x777777FF, true, [this] { openQuitMenu(); });
        else
            addEntry("QUIT EMULATIONSTATION", 0x777777FF, false, [this] { openQuitMenu(); });
    }

    addChild(&mMenu);
    addVersionInfo();
    setSize(mMenu.getSize());
    setPosition((Renderer::getScreenWidth() - mSize.x) / 2.0f, Renderer::getScreenHeight() * 0.13f);
}

GuiMenu::~GuiMenu()
{
    // This is required for the situation where scrolling started just before the menu
    // was openened. Without this, the scrolling would run until manually stopped after
    // the menu has been closed.
    ViewController::getInstance()->stopScrolling();
}

void GuiMenu::openScraperOptions()
{
    // Open the scraper menu.
    mWindow->pushGui(new GuiScraperMenu("SCRAPER"));
}

void GuiMenu::openUIOptions()
{
    auto s = new GuiSettings("UI SETTINGS");

    // Optionally start in selected system/gamelist.
    auto startupSystem = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "GAMELIST ON STARTUP", false);
    startupSystem->add("NONE", "", Settings::getInstance()->getString("StartupSystem") == "");
    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        if ((*it)->getName() != "retropie") {
            // If required, abbreviate the system name so it doesn't overlap the setting name.
            float maxNameLength = mSize.x * 0.48f;
            startupSystem->add((*it)->getFullName(), (*it)->getName(),
                               Settings::getInstance()->getString("StartupSystem") ==
                                   (*it)->getName(),
                               maxNameLength);
        }
    }
    // This can probably not happen but as an extra precaution select the "NONE" entry if no
    // entry is selected.
    if (startupSystem->getSelectedObjects().size() == 0)
        startupSystem->selectEntry(0);
    s->addWithLabel("GAMELIST ON STARTUP", startupSystem);
    s->addSaveFunc([startupSystem, s] {
        if (startupSystem->getSelected() != Settings::getInstance()->getString("StartupSystem")) {
            Settings::getInstance()->setString("StartupSystem", startupSystem->getSelected());
            s->setNeedsSaving();
        }
    });

    // Gamelist view style.
    auto gamelist_view_style = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "GAMELIST VIEW STYLE", false);
    std::string selectedViewStyle = Settings::getInstance()->getString("GamelistViewStyle");
    gamelist_view_style->add("automatic", "automatic", selectedViewStyle == "automatic");
    gamelist_view_style->add("basic", "basic", selectedViewStyle == "basic");
    gamelist_view_style->add("detailed", "detailed", selectedViewStyle == "detailed");
    gamelist_view_style->add("video", "video", selectedViewStyle == "video");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the view style to Automatic in this case.
    if (gamelist_view_style->getSelectedObjects().size() == 0)
        gamelist_view_style->selectEntry(0);
    s->addWithLabel("GAMELIST VIEW STYLE", gamelist_view_style);
    s->addSaveFunc([gamelist_view_style, s] {
        if (gamelist_view_style->getSelected() !=
            Settings::getInstance()->getString("GamelistViewStyle")) {
            Settings::getInstance()->setString("GamelistViewStyle",
                                               gamelist_view_style->getSelected());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    // Transition style.
    auto transition_style = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "TRANSITION STYLE", false);
    std::vector<std::string> transitions;
    transitions.push_back("slide");
    transitions.push_back("fade");
    transitions.push_back("instant");
    for (auto it = transitions.cbegin(); it != transitions.cend(); ++it)
        transition_style->add(*it, *it,
                              Settings::getInstance()->getString("TransitionStyle") == *it);
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
        std::map<std::string, ThemeData::ThemeSet>::const_iterator selectedSet =
            themeSets.find(Settings::getInstance()->getString("ThemeSet"));
        if (selectedSet == themeSets.cend())
            selectedSet = themeSets.cbegin();
        auto theme_set =
            std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "THEME SET", false);
        for (auto it = themeSets.cbegin(); it != themeSets.cend(); ++it) {
            // If required, abbreviate the theme set name so it doesn't overlap the setting name.
            float maxNameLength = mSize.x * 0.62f;
            theme_set->add(it->first, it->first, it == selectedSet, maxNameLength);
        }
        s->addWithLabel("THEME SET", theme_set);
        s->addSaveFunc([this, theme_set, s] {
            if (theme_set->getSelected() != Settings::getInstance()->getString("ThemeSet")) {
                Scripting::fireEvent("theme-changed", theme_set->getSelected(),
                                     Settings::getInstance()->getString("ThemeSet"));
                Settings::getInstance()->setString("ThemeSet", theme_set->getSelected());
                CollectionSystemsManager::getInstance()->updateSystemsList();
                mWindow->setChangedThemeSet();
                // This is required so that the custom collection system does not disappear
                // if the user is editing a custom collection when switching theme sets.
                if (CollectionSystemsManager::getInstance()->isEditing()) {
                    CollectionSystemsManager::getInstance()->exitEditMode();
                    s->setNeedsCollectionsUpdate();
                }
                s->setNeedsSaving();
                s->setNeedsReloading();
                s->setNeedsGoToStart();
                s->setInvalidateCachedBackground();
            }
        });
    }

    // UI mode.
    auto ui_mode =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "UI MODE", false);
    std::vector<std::string> uiModes;
    uiModes.push_back("full");
    uiModes.push_back("kiosk");
    uiModes.push_back("kid");
    std::string setMode;
    if (Settings::getInstance()->getBool("ForceKiosk"))
        setMode = "kiosk";
    else if (Settings::getInstance()->getBool("ForceKid"))
        setMode = "kid";
    else
        setMode = Settings::getInstance()->getString("UIMode");
    for (auto it = uiModes.cbegin(); it != uiModes.cend(); ++it)
        ui_mode->add(*it, *it, setMode == *it);
    s->addWithLabel("UI MODE", ui_mode);
    s->addSaveFunc([ui_mode, this, s] {
        std::string selectedMode = ui_mode->getSelected();
        // If any of the force flags are set, then always apply and save the setting.
        if (selectedMode == Settings::getInstance()->getString("UIMode") &&
            !Settings::getInstance()->getBool("ForceFull") &&
            !Settings::getInstance()->getBool("ForceKiosk") &&
            !Settings::getInstance()->getBool("ForceKid")) {
            return;
        }
        else if (selectedMode != "full") {
            std::string msg = "YOU ARE CHANGING THE UI TO THE RESTRICTED MODE\n'" +
                              Utils::String::toUpper(selectedMode) + "'\n";
            if (selectedMode == "kiosk") {
                msg += "THIS WILL HIDE MOST MENU OPTIONS TO PREVENT\n";
                msg += "CHANGES TO THE SYSTEM\n";
            }
            else {
                msg += "THIS WILL LIMIT THE AVAILABLE GAMES TO THE ONES\n";
                msg += "FLAGGED SUITABLE FOR CHILDREN\n";
            }
            msg += "TO UNLOCK AND RETURN TO THE FULL UI, ENTER THIS CODE: \n";
            msg += UIModeController::getInstance()->getFormattedPassKeyStr() + "\n\n";
            msg += "DO YOU WANT TO PROCEED?";
            mWindow->pushGui(new GuiMsgBox(
                this->getHelpStyle(), msg, "YES",
                [this, selectedMode] {
                    LOG(LogDebug) << "GuiMenu::openUISettings(): Setting UI mode to '"
                                  << selectedMode << "'.";
                    Settings::getInstance()->setString("UIMode", selectedMode);
                    Settings::getInstance()->setBool("ForceFull", false);
                    Settings::getInstance()->setBool("ForceKiosk", false);
                    Settings::getInstance()->setBool("ForceKid", false);
                    Settings::getInstance()->saveFile();
                    UIModeController::getInstance()->setCurrentUIMode(selectedMode);
                    for (auto it = SystemData::sSystemVector.cbegin();
                         it != SystemData::sSystemVector.cend(); ++it) {
                        if ((*it)->getThemeFolder() == "custom-collections") {
                            for (FileData* customSystem :
                                 (*it)->getRootFolder()->getChildrenListToDisplay())
                                customSystem->getSystem()->getIndex()->resetFilters();
                        }
                        (*it)->sortSystem();
                        (*it)->getIndex()->resetFilters();
                    }
                    ViewController::getInstance()->reloadAll();
                    ViewController::getInstance()->goToSystem(SystemData::sSystemVector.front(),
                                                              false);
                    mWindow->invalidateCachedBackground();
                },
                "NO", nullptr));
        }
        else {
            LOG(LogDebug) << "GuiMenu::openUISettings(): Setting UI mode to '" << selectedMode
                          << "'.";
            Settings::getInstance()->setString("UIMode", ui_mode->getSelected());
            Settings::getInstance()->setBool("ForceFull", false);
            Settings::getInstance()->setBool("ForceKiosk", false);
            Settings::getInstance()->setBool("ForceKid", false);
            UIModeController::getInstance()->setCurrentUIMode("full");
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setNeedsSortingCollections();
            s->setNeedsResetFilters();
            s->setNeedsReloading();
            s->setNeedsGoToSystem(SystemData::sSystemVector.front());
            s->setInvalidateCachedBackground();
        }
    });

    // Default gamelist sort order.
    std::string sortOrder;
    auto default_sort_order = std::make_shared<OptionListComponent<const FileData::SortType*>>(
        getHelpStyle(), "DEFAULT SORT ORDER", false);
    // Exclude the System sort options.
    unsigned int numSortTypes = static_cast<unsigned int>(FileSorts::SortTypes.size() - 2);
    for (unsigned int i = 0; i < numSortTypes; ++i) {
        if (FileSorts::SortTypes[i].description ==
            Settings::getInstance()->getString("DefaultSortOrder")) {
            sortOrder = FileSorts::SortTypes[i].description;
            break;
        }
    }
    // If an invalid sort order was defined in es_settings.xml, then apply the default
    // sort order 'filename, ascending'.
    if (sortOrder == "")
        sortOrder = Settings::getInstance()->getDefaultString("DefaultSortOrder");
    for (unsigned int i = 0; i < numSortTypes; ++i) {
        const FileData::SortType& sort = FileSorts::SortTypes[i];
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
            s->setInvalidateCachedBackground();
        }
    });

    // Open menu effect.
    auto menu_opening_effect = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "MENU OPENING EFFECT", false);
    std::string selectedMenuEffect = Settings::getInstance()->getString("MenuOpeningEffect");
    menu_opening_effect->add("SCALE-UP", "scale-up", selectedMenuEffect == "scale-up");
    menu_opening_effect->add("NONE", "none", selectedMenuEffect == "none");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the opening effect to "scale-up" in this case.
    if (menu_opening_effect->getSelectedObjects().size() == 0)
        menu_opening_effect->selectEntry(0);
    s->addWithLabel("MENU OPENING EFFECT", menu_opening_effect);
    s->addSaveFunc([menu_opening_effect, s] {
        if (menu_opening_effect->getSelected() !=
            Settings::getInstance()->getString("MenuOpeningEffect")) {
            Settings::getInstance()->setString("MenuOpeningEffect",
                                               menu_opening_effect->getSelected());
            s->setNeedsSaving();
        }
    });

    // Launch screen duration.
    auto launch_screen_duration = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "LAUNCH SCREEN DURATION", false);
    std::string selectedDuration = Settings::getInstance()->getString("LaunchScreenDuration");
    launch_screen_duration->add("NORMAL", "normal", selectedDuration == "normal");
    launch_screen_duration->add("BRIEF", "brief", selectedDuration == "brief");
    launch_screen_duration->add("LONG", "long", selectedDuration == "long");
    launch_screen_duration->add("DISABLED", "disabled", selectedDuration == "disabled");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the duration to "normal" in this case.
    if (launch_screen_duration->getSelectedObjects().size() == 0)
        launch_screen_duration->selectEntry(0);
    s->addWithLabel("LAUNCH SCREEN DURATION", launch_screen_duration);
    s->addSaveFunc([launch_screen_duration, s] {
        if (launch_screen_duration->getSelected() !=
            Settings::getInstance()->getString("LaunchScreenDuration")) {
            Settings::getInstance()->setString("LaunchScreenDuration",
                                               launch_screen_duration->getSelected());
            s->setNeedsSaving();
        }
    });

    // Media viewer.
    ComponentListRow media_viewer_row;
    media_viewer_row.elements.clear();
    media_viewer_row.addElement(std::make_shared<TextComponent>("MEDIA VIEWER SETTINGS",
                                                                Font::get(FONT_SIZE_MEDIUM),
                                                                0x777777FF),
                                true);
    media_viewer_row.addElement(makeArrow(), false);
    media_viewer_row.makeAcceptInputHandler(std::bind(&GuiMenu::openMediaViewerOptions, this));
    s->addRow(media_viewer_row);

    // Screensaver.
    ComponentListRow screensaver_row;
    screensaver_row.elements.clear();
    screensaver_row.addElement(std::make_shared<TextComponent>(
                                   "SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                               true);
    screensaver_row.addElement(makeArrow(), false);
    screensaver_row.makeAcceptInputHandler(std::bind(&GuiMenu::openScreensaverOptions, this));
    s->addRow(screensaver_row);

#if defined(USE_OPENGL_21)
    // Blur background when the menu is open.
    auto menu_blur_background = std::make_shared<SwitchComponent>();
    menu_blur_background->setState(Settings::getInstance()->getBool("MenuBlurBackground"));
    s->addWithLabel("BLUR BACKGROUND WHEN MENU IS OPEN", menu_blur_background);
    s->addSaveFunc([menu_blur_background, s] {
        if (menu_blur_background->getState() !=
            Settings::getInstance()->getBool("MenuBlurBackground")) {
            Settings::getInstance()->setBool("MenuBlurBackground",
                                             menu_blur_background->getState());
            s->setNeedsSaving();
            s->setInvalidateCachedBackground();
        }
    });
#endif

    // Display pillarboxes (and letterboxes) for videos in the gamelists.
    auto gamelist_video_pillarbox = std::make_shared<SwitchComponent>();
    gamelist_video_pillarbox->setState(Settings::getInstance()->getBool("GamelistVideoPillarbox"));
    s->addWithLabel("DISPLAY PILLARBOXES FOR GAMELIST VIDEOS", gamelist_video_pillarbox);
    s->addSaveFunc([gamelist_video_pillarbox, s] {
        if (gamelist_video_pillarbox->getState() !=
            Settings::getInstance()->getBool("GamelistVideoPillarbox")) {
            Settings::getInstance()->setBool("GamelistVideoPillarbox",
                                             gamelist_video_pillarbox->getState());
            s->setNeedsSaving();
        }
    });

#if defined(USE_OPENGL_21)
    // Render scanlines for videos in the gamelists.
    auto gamelist_video_scanlines = std::make_shared<SwitchComponent>();
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
    auto folders_on_top = std::make_shared<SwitchComponent>();
    folders_on_top->setState(Settings::getInstance()->getBool("FoldersOnTop"));
    s->addWithLabel("SORT FOLDERS ON TOP OF GAMELISTS", folders_on_top);
    s->addSaveFunc([folders_on_top, s] {
        if (folders_on_top->getState() != Settings::getInstance()->getBool("FoldersOnTop")) {
            Settings::getInstance()->setBool("FoldersOnTop", folders_on_top->getState());
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setInvalidateCachedBackground();
        }
    });

    // Sort favorites on top of non-favorites in the gamelists.
    auto favorites_first = std::make_shared<SwitchComponent>();
    favorites_first->setState(Settings::getInstance()->getBool("FavoritesFirst"));
    s->addWithLabel("SORT FAVORITE GAMES ABOVE NON-FAVORITES", favorites_first);
    s->addSaveFunc([favorites_first, s] {
        if (favorites_first->getState() != Settings::getInstance()->getBool("FavoritesFirst")) {
            Settings::getInstance()->setBool("FavoritesFirst", favorites_first->getState());
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setNeedsSortingCollections();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable gamelist star markings for favorite games.
    auto favorites_star = std::make_shared<SwitchComponent>();
    favorites_star->setState(Settings::getInstance()->getBool("FavoritesStar"));
    s->addWithLabel("ADD STAR MARKINGS TO FAVORITE GAMES", favorites_star);
    s->addSaveFunc([favorites_star, s] {
        if (favorites_star->getState() != Settings::getInstance()->getBool("FavoritesStar")) {
            Settings::getInstance()->setBool("FavoritesStar", favorites_star->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    // Use ASCII for special characters in the gamelist view instead of the Font Awesome symbols.
    auto special_chars_ascii = std::make_shared<SwitchComponent>();
    special_chars_ascii->setState(Settings::getInstance()->getBool("SpecialCharsASCII"));
    s->addWithLabel("USE PLAIN ASCII FOR SPECIAL GAMELIST CHARACTERS", special_chars_ascii);
    s->addSaveFunc([special_chars_ascii, s] {
        if (special_chars_ascii->getState() !=
            Settings::getInstance()->getBool("SpecialCharsASCII")) {
            Settings::getInstance()->setBool("SpecialCharsASCII", special_chars_ascii->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable quick list scrolling overlay.
    auto list_scroll_overlay = std::make_shared<SwitchComponent>();
    list_scroll_overlay->setState(Settings::getInstance()->getBool("ListScrollOverlay"));
    s->addWithLabel("ENABLE QUICK LIST SCROLLING OVERLAY", list_scroll_overlay);
    s->addSaveFunc([list_scroll_overlay, s] {
        if (list_scroll_overlay->getState() !=
            Settings::getInstance()->getBool("ListScrollOverlay")) {
            Settings::getInstance()->setBool("ListScrollOverlay", list_scroll_overlay->getState());
            s->setNeedsSaving();
        }
    });

    // Enable virtual (on-screen) keyboard.
    auto virtual_keyboard = std::make_shared<SwitchComponent>();
    virtual_keyboard->setState(Settings::getInstance()->getBool("VirtualKeyboard"));
    s->addWithLabel("ENABLE VIRTUAL KEYBOARD", virtual_keyboard);
    s->addSaveFunc([virtual_keyboard, s] {
        if (virtual_keyboard->getState() != Settings::getInstance()->getBool("VirtualKeyboard")) {
            Settings::getInstance()->setBool("VirtualKeyboard", virtual_keyboard->getState());
            s->setNeedsSaving();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable menu scroll indicators.
    auto scroll_indicators = std::make_shared<SwitchComponent>();
    scroll_indicators->setState(Settings::getInstance()->getBool("ScrollIndicators"));
    s->addWithLabel("ENABLE MENU SCROLL INDICATORS", scroll_indicators);
    s->addSaveFunc([scroll_indicators, s] {
        if (scroll_indicators->getState() != Settings::getInstance()->getBool("ScrollIndicators")) {
            Settings::getInstance()->setBool("ScrollIndicators", scroll_indicators->getState());
            s->setNeedsSaving();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable the 'Y' button for tagging games as favorites.
    auto favorites_add_button = std::make_shared<SwitchComponent>();
    favorites_add_button->setState(Settings::getInstance()->getBool("FavoritesAddButton"));
    s->addWithLabel("ENABLE TOGGLE FAVORITES BUTTON", favorites_add_button);
    s->addSaveFunc([favorites_add_button, s] {
        if (Settings::getInstance()->getBool("FavoritesAddButton") !=
            favorites_add_button->getState()) {
            Settings::getInstance()->setBool("FavoritesAddButton",
                                             favorites_add_button->getState());
            s->setNeedsSaving();
        }
    });

    // Enable the thumbstick click buttons for jumping to a random system or game.
    auto random_add_button = std::make_shared<SwitchComponent>();
    random_add_button->setState(Settings::getInstance()->getBool("RandomAddButton"));
    s->addWithLabel("ENABLE RANDOM SYSTEM OR GAME BUTTON", random_add_button);
    s->addSaveFunc([random_add_button, s] {
        if (Settings::getInstance()->getBool("RandomAddButton") != random_add_button->getState()) {
            Settings::getInstance()->setBool("RandomAddButton", random_add_button->getState());
            s->setNeedsSaving();
        }
    });

    // Gamelist filters.
    auto gamelist_filters = std::make_shared<SwitchComponent>();
    gamelist_filters->setState(Settings::getInstance()->getBool("GamelistFilters"));
    s->addWithLabel("ENABLE GAMELIST FILTERS", gamelist_filters);
    s->addSaveFunc([gamelist_filters, s] {
        if (Settings::getInstance()->getBool("GamelistFilters") != gamelist_filters->getState()) {
            Settings::getInstance()->setBool("GamelistFilters", gamelist_filters->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
        }
    });

    // Quick system select (navigate left/right in gamelist view).
    auto quick_system_select = std::make_shared<SwitchComponent>();
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
    auto show_help_prompts = std::make_shared<SwitchComponent>();
    show_help_prompts->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
    s->addWithLabel("DISPLAY ON-SCREEN HELP", show_help_prompts);
    s->addSaveFunc([show_help_prompts, s] {
        if (Settings::getInstance()->getBool("ShowHelpPrompts") != show_help_prompts->getState()) {
            Settings::getInstance()->setBool("ShowHelpPrompts", show_help_prompts->getState());
            s->setNeedsSaving();
        }
    });

    // Play videos immediately (overrides theme setting).
    auto play_videos_immediately = std::make_shared<SwitchComponent>();
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

    s->setSize(mSize);
    mWindow->pushGui(s);
}

void GuiMenu::openSoundOptions()
{
    auto s = new GuiSettings("SOUND SETTINGS");

// TODO: Hide the volume slider on macOS and BSD Unix until the volume control logic has been
// implemented for these operating systems.
#if !defined(__APPLE__) && !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)
    // System volume.
    // The reason to create the VolumeControl object every time instead of making it a singleton
    // is that this is the easiest way to detect new default audio devices or changes to the
    // audio volume done by the operating system. And we don't really need this object laying
    // around anyway as it's only used here.
    VolumeControl volumeControl;
    int currentVolume = volumeControl.getVolume();

    auto systemVolume = std::make_shared<SliderComponent>(0.f, 100.f, 1.f, "%");
    systemVolume->setValue(static_cast<float>(currentVolume));
    s->addWithLabel("SYSTEM VOLUME", systemVolume);
    s->addSaveFunc([systemVolume, currentVolume] {
        // No need to create the VolumeControl object unless the volume has actually been changed.
        if (static_cast<int>(systemVolume->getValue()) != currentVolume) {
            VolumeControl volumeControl;
            volumeControl.setVolume(static_cast<int>(std::round(systemVolume->getValue())));
        }
    });
#endif

    // Volume for navigation sounds.
    auto sound_volume_navigation = std::make_shared<SliderComponent>(0.f, 100.f, 1.f, "%");
    sound_volume_navigation->setValue(
        static_cast<float>(Settings::getInstance()->getInt("SoundVolumeNavigation")));
    s->addWithLabel("NAVIGATION SOUNDS VOLUME", sound_volume_navigation);
    s->addSaveFunc([sound_volume_navigation, s] {
        if (sound_volume_navigation->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("SoundVolumeNavigation"))) {
            Settings::getInstance()->setInt("SoundVolumeNavigation",
                                            static_cast<int>(sound_volume_navigation->getValue()));
            s->setNeedsSaving();
        }
    });

    // Volume for videos.
    auto sound_volume_videos = std::make_shared<SliderComponent>(0.f, 100.f, 1.f, "%");
    sound_volume_videos->setValue(
        static_cast<float>(Settings::getInstance()->getInt("SoundVolumeVideos")));
    s->addWithLabel("VIDEO PLAYER VOLUME", sound_volume_videos);
    s->addSaveFunc([sound_volume_videos, s] {
        if (sound_volume_videos->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("SoundVolumeVideos"))) {
            Settings::getInstance()->setInt("SoundVolumeVideos",
                                            static_cast<int>(sound_volume_videos->getValue()));
            s->setNeedsSaving();
        }
    });

    if (UIModeController::getInstance()->isUIModeFull()) {
        // Play audio for gamelist videos.
        auto gamelist_video_audio = std::make_shared<SwitchComponent>();
        gamelist_video_audio->setState(Settings::getInstance()->getBool("GamelistVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR VIDEOS IN THE GAMELIST VIEW", gamelist_video_audio);
        s->addSaveFunc([gamelist_video_audio, s] {
            if (gamelist_video_audio->getState() !=
                Settings::getInstance()->getBool("GamelistVideoAudio")) {
                Settings::getInstance()->setBool("GamelistVideoAudio",
                                                 gamelist_video_audio->getState());
                s->setNeedsSaving();
            }
        });

        // Play audio for media viewer videos.
        auto media_viewer_video_audio = std::make_shared<SwitchComponent>();
        media_viewer_video_audio->setState(
            Settings::getInstance()->getBool("MediaViewerVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR MEDIA VIEWER VIDEOS", media_viewer_video_audio);
        s->addSaveFunc([media_viewer_video_audio, s] {
            if (media_viewer_video_audio->getState() !=
                Settings::getInstance()->getBool("MediaViewerVideoAudio")) {
                Settings::getInstance()->setBool("MediaViewerVideoAudio",
                                                 media_viewer_video_audio->getState());
                s->setNeedsSaving();
            }
        });

        // Play audio for screensaver videos.
        auto screensaver_video_audio = std::make_shared<SwitchComponent>();
        screensaver_video_audio->setState(
            Settings::getInstance()->getBool("ScreensaverVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR SCREENSAVER VIDEOS", screensaver_video_audio);
        s->addSaveFunc([screensaver_video_audio, s] {
            if (screensaver_video_audio->getState() !=
                Settings::getInstance()->getBool("ScreensaverVideoAudio")) {
                Settings::getInstance()->setBool("ScreensaverVideoAudio",
                                                 screensaver_video_audio->getState());
                s->setNeedsSaving();
            }
        });

        // Navigation sounds.
        auto navigation_sounds = std::make_shared<SwitchComponent>();
        navigation_sounds->setState(Settings::getInstance()->getBool("NavigationSounds"));
        s->addWithLabel("ENABLE NAVIGATION SOUNDS", navigation_sounds);
        s->addSaveFunc([navigation_sounds, s] {
            if (navigation_sounds->getState() !=
                Settings::getInstance()->getBool("NavigationSounds")) {
                Settings::getInstance()->setBool("NavigationSounds", navigation_sounds->getState());
                s->setNeedsSaving();
            }
        });
    }

    s->setSize(mSize);
    mWindow->pushGui(s);
}

void GuiMenu::openInputDeviceOptions()
{
    auto s = new GuiSettings("INPUT DEVICE SETTINGS");

    // Controller type.
    auto input_controller_type = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "CONTROLLER TYPE", false);
    std::string selectedPlayer = Settings::getInstance()->getString("InputControllerType");
    input_controller_type->add("XBOX", "xbox", selectedPlayer == "xbox");
    input_controller_type->add("XBOX 360", "xbox360", selectedPlayer == "xbox360");
    input_controller_type->add("PLAYSTATION 4", "ps4", selectedPlayer == "ps4");
    input_controller_type->add("PLAYSTATION 5", "ps5", selectedPlayer == "ps5");
    input_controller_type->add("SNES", "snes", selectedPlayer == "snes");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the controller type to "xbox" in this case.
    if (input_controller_type->getSelectedObjects().size() == 0)
        input_controller_type->selectEntry(0);
    s->addWithLabel("CONTROLLER TYPE", input_controller_type);
    s->addSaveFunc([input_controller_type, s] {
        if (input_controller_type->getSelected() !=
            Settings::getInstance()->getString("InputControllerType")) {
            Settings::getInstance()->setString("InputControllerType",
                                               input_controller_type->getSelected());
            s->setNeedsReloadHelpPrompts();
            s->setNeedsSaving();
        }
    });

    // Whether to only accept input from the first controller.
    auto input_only_first_controller = std::make_shared<SwitchComponent>();
    input_only_first_controller->setState(
        Settings::getInstance()->getBool("InputOnlyFirstController"));
    s->addWithLabel("ONLY ACCEPT INPUT FROM FIRST CONTROLLER", input_only_first_controller);
    s->addSaveFunc([input_only_first_controller, s] {
        if (Settings::getInstance()->getBool("InputOnlyFirstController") !=
            input_only_first_controller->getState()) {
            Settings::getInstance()->setBool("InputOnlyFirstController",
                                             input_only_first_controller->getState());
            s->setNeedsSaving();
        }
    });

    // Configure keyboard and controllers.
    ComponentListRow configure_input_row;
    configure_input_row.elements.clear();
    configure_input_row.addElement(
        std::make_shared<TextComponent>("CONFIGURE KEYBOARD AND CONTROLLERS",
                                        Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
        true);
    configure_input_row.addElement(makeArrow(), false);
    configure_input_row.makeAcceptInputHandler(std::bind(&GuiMenu::openConfigInput, this, s));
    s->addRow(configure_input_row);

    s->setSize(mSize);
    mWindow->pushGui(s);
}

void GuiMenu::openConfigInput(GuiSettings* settings)
{
    // Always save the settings before starting the input configuration, in case the
    // controller type was changed.
    settings->save();
    // Also unset the save flag so that a double saving does not take place when closing
    // the input device settings menu later on.
    settings->setNeedsSaving(false);

    std::string message = "THE KEYBOARD AND CONTROLLERS ARE AUTOMATICALLY\n"
                          "CONFIGURED, BUT USING THIS CONFIGURATION TOOL\n"
                          "YOU CAN OVERRIDE THE DEFAULT BUTTON MAPPINGS\n"
                          "(THIS WILL NOT AFFECT THE HELP PROMPTS)\n"
                          "CONTINUE?";

    Window* window {mWindow};
    window->pushGui(new GuiMsgBox(
        getHelpStyle(), message, "YES",
        [window] { window->pushGui(new GuiDetectDevice(false, false, nullptr)); }, "NO", nullptr));
}

void GuiMenu::openOtherOptions()
{
    auto s = new GuiSettings("OTHER SETTINGS");

    // Alternative emulators GUI.
    ComponentListRow alternativeEmulatorsRow;
    alternativeEmulatorsRow.elements.clear();
    alternativeEmulatorsRow.addElement(std::make_shared<TextComponent>("ALTERNATIVE EMULATORS",
                                                                       Font::get(FONT_SIZE_MEDIUM),
                                                                       0x777777FF),
                                       true);
    alternativeEmulatorsRow.addElement(makeArrow(), false);
    alternativeEmulatorsRow.makeAcceptInputHandler(
        std::bind([this] { mWindow->pushGui(new GuiAlternativeEmulators); }));
    s->addRow(alternativeEmulatorsRow);

    // Game media directory.
    ComponentListRow rowMediaDir;
    auto media_directory = std::make_shared<TextComponent>("GAME MEDIA DIRECTORY",
                                                           Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
    auto bracketMediaDirectory = std::make_shared<ImageComponent>();
    bracketMediaDirectory->setImage(":/graphics/arrow.svg");
    bracketMediaDirectory->setResize(
        glm::vec2 {0.0f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()});
    rowMediaDir.addElement(media_directory, true);
    rowMediaDir.addElement(bracketMediaDirectory, false);
    std::string titleMediaDir = "ENTER GAME MEDIA DIRECTORY";
    std::string mediaDirectoryStaticText = "Default directory:";
    std::string defaultDirectoryText = "~/.emulationstation/downloaded_media/";
    std::string initValueMediaDir = Settings::getInstance()->getString("MediaDirectory");
    bool multiLineMediaDir = false;
    auto updateValMediaDir = [this](const std::string& newVal) {
        Settings::getInstance()->setString("MediaDirectory", newVal);
        Settings::getInstance()->saveFile();
        ViewController::getInstance()->reloadAll();
        mWindow->invalidateCachedBackground();
    };
    rowMediaDir.makeAcceptInputHandler([this, titleMediaDir, mediaDirectoryStaticText,
                                        defaultDirectoryText, initValueMediaDir, updateValMediaDir,
                                        multiLineMediaDir] {
        if (Settings::getInstance()->getBool("VirtualKeyboard")) {
            mWindow->pushGui(new GuiTextEditKeyboardPopup(
                getHelpStyle(), titleMediaDir, Settings::getInstance()->getString("MediaDirectory"),
                updateValMediaDir, multiLineMediaDir, "SAVE", "SAVE CHANGES?",
                mediaDirectoryStaticText, defaultDirectoryText, "load default directory"));
        }
        else {
            mWindow->pushGui(new GuiTextEditPopup(
                getHelpStyle(), titleMediaDir, Settings::getInstance()->getString("MediaDirectory"),
                updateValMediaDir, multiLineMediaDir, "SAVE", "SAVE CHANGES?",
                mediaDirectoryStaticText, defaultDirectoryText, "load default directory"));
        }
    });
    s->addRow(rowMediaDir);

    // Maximum VRAM.
    auto max_vram = std::make_shared<SliderComponent>(80.f, 1024.f, 8.f, "MiB");
    max_vram->setValue(static_cast<float>(Settings::getInstance()->getInt("MaxVRAM")));
    s->addWithLabel("VRAM LIMIT", max_vram);
    s->addSaveFunc([max_vram, s] {
        if (max_vram->getValue() != Settings::getInstance()->getInt("MaxVRAM")) {
            Settings::getInstance()->setInt("MaxVRAM",
                                            static_cast<int>(std::round(max_vram->getValue())));
            s->setNeedsSaving();
        }
    });

    // Display/monitor.
    auto display_index = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "DISPLAY/MONITOR INDEX", false);
    std::vector<std::string> displayIndex;
    displayIndex.push_back("1");
    displayIndex.push_back("2");
    displayIndex.push_back("3");
    displayIndex.push_back("4");
    for (auto it = displayIndex.cbegin(); it != displayIndex.cend(); ++it)
        display_index->add(*it, *it,
                           Settings::getInstance()->getInt("DisplayIndex") == atoi((*it).c_str()));
    s->addWithLabel("DISPLAY/MONITOR INDEX (REQUIRES RESTART)", display_index);
    s->addSaveFunc([display_index, s] {
        if (atoi(display_index->getSelected().c_str()) !=
            Settings::getInstance()->getInt("DisplayIndex")) {
            Settings::getInstance()->setInt("DisplayIndex",
                                            atoi(display_index->getSelected().c_str()));
            s->setNeedsSaving();
        }
    });

    // Exit button configuration.
    auto exit_button_config = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "EXIT BUTTON COMBO", false);
    std::string selectedExitButtonCombo = Settings::getInstance()->getString("ExitButtonCombo");
    exit_button_config->add("F4", "F4", selectedExitButtonCombo == "F4");
    exit_button_config->add("Alt + F4", "AltF4", selectedExitButtonCombo == "AltF4");
#if defined(_WIN64) || defined(__unix__)
    exit_button_config->add("Alt + Q", "AltQ", selectedExitButtonCombo == "AltQ");
#endif
#if defined(__APPLE__)
    exit_button_config->add("\u2318 + Q", "CmdQ", selectedExitButtonCombo == "CmdQ");
#endif
    s->addWithLabel("EXIT BUTTON COMBO", exit_button_config);
    s->addSaveFunc([exit_button_config, s] {
        if (exit_button_config->getSelected() !=
            Settings::getInstance()->getString("ExitButtonCombo")) {
            Settings::getInstance()->setString("ExitButtonCombo",
                                               exit_button_config->getSelected());
            s->setNeedsSaving();
        }
    });

    // When to save game metadata.
    auto save_gamelist_mode = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "WHEN TO SAVE METADATA", false);
    std::vector<std::string> saveModes;
    saveModes.push_back("on exit");
    saveModes.push_back("always");
    saveModes.push_back("never");
    for (auto it = saveModes.cbegin(); it != saveModes.cend(); ++it) {
        save_gamelist_mode->add(*it, *it,
                                Settings::getInstance()->getString("SaveGamelistsMode") == *it);
    }
    s->addWithLabel("WHEN TO SAVE GAME METADATA", save_gamelist_mode);
    s->addSaveFunc([save_gamelist_mode, s] {
        if (save_gamelist_mode->getSelected() !=
            Settings::getInstance()->getString("SaveGamelistsMode")) {
            Settings::getInstance()->setString("SaveGamelistsMode",
                                               save_gamelist_mode->getSelected());
            // Always save the gamelist.xml files if switching to "always" as there may
            // be changes that will otherwise be lost.
            if (Settings::getInstance()->getString("SaveGamelistsMode") == "always") {
                for (auto it = SystemData::sSystemVector.cbegin();
                     it != SystemData::sSystemVector.cend(); ++it)
                    (*it)->writeMetaData();
            }
            s->setNeedsSaving();
        }
    });

#if defined(_WIN64)
    // Hide taskbar during the program session.
    auto hide_taskbar = std::make_shared<SwitchComponent>();
    hide_taskbar->setState(Settings::getInstance()->getBool("HideTaskbar"));
    s->addWithLabel("HIDE TASKBAR (REQUIRES RESTART)", hide_taskbar);
    s->addSaveFunc([hide_taskbar, s] {
        if (hide_taskbar->getState() != Settings::getInstance()->getBool("HideTaskbar")) {
            Settings::getInstance()->setBool("HideTaskbar", hide_taskbar->getState());
            s->setNeedsSaving();
        }
    });
#endif

    // Run ES in the background when a game has been launched.
    auto run_in_background = std::make_shared<SwitchComponent>();
    run_in_background->setState(Settings::getInstance()->getBool("RunInBackground"));
    s->addWithLabel("RUN IN BACKGROUND (WHILE GAME IS LAUNCHED)", run_in_background);
    s->addSaveFunc([run_in_background, s] {
        if (run_in_background->getState() != Settings::getInstance()->getBool("RunInBackground")) {
            Settings::getInstance()->setBool("RunInBackground", run_in_background->getState());
            s->setNeedsSaving();
        }
    });

#if defined(VIDEO_HW_DECODING)
    // Whether to enable hardware decoding for the FFmpeg video player.
    auto video_hardware_decoding = std::make_shared<SwitchComponent>();
    video_hardware_decoding->setState(Settings::getInstance()->getBool("VideoHardwareDecoding"));
    s->addWithLabel("VIDEO HARDWARE DECODING (EXPERIMENTAL)", video_hardware_decoding);
    s->addSaveFunc([video_hardware_decoding, s] {
        if (video_hardware_decoding->getState() !=
            Settings::getInstance()->getBool("VideoHardwareDecoding")) {
            Settings::getInstance()->setBool("VideoHardwareDecoding",
                                             video_hardware_decoding->getState());
            s->setNeedsSaving();
        }
    });
#endif

    // Whether to upscale the video frame rate to 60 FPS.
    auto video_upscale_frame_rate = std::make_shared<SwitchComponent>();
    video_upscale_frame_rate->setState(Settings::getInstance()->getBool("VideoUpscaleFrameRate"));
    s->addWithLabel("UPSCALE VIDEO FRAME RATE TO 60 FPS", video_upscale_frame_rate);
    s->addSaveFunc([video_upscale_frame_rate, s] {
        if (video_upscale_frame_rate->getState() !=
            Settings::getInstance()->getBool("VideoUpscaleFrameRate")) {
            Settings::getInstance()->setBool("VideoUpscaleFrameRate",
                                             video_upscale_frame_rate->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to preload the gamelists on application startup.
    auto preloadGamelists = std::make_shared<SwitchComponent>();
    preloadGamelists->setState(Settings::getInstance()->getBool("PreloadGamelists"));
    s->addWithLabel("PRELOAD GAMELISTS ON STARTUP", preloadGamelists);
    s->addSaveFunc([preloadGamelists, s] {
        if (preloadGamelists->getState() != Settings::getInstance()->getBool("PreloadGamelists")) {
            Settings::getInstance()->setBool("PreloadGamelists", preloadGamelists->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to enable alternative emulators per game (the option to disable this is intended
    // primarily for testing purposes).
    auto alternativeEmulatorPerGame = std::make_shared<SwitchComponent>();
    alternativeEmulatorPerGame->setState(
        Settings::getInstance()->getBool("AlternativeEmulatorPerGame"));
    s->addWithLabel("ENABLE ALTERNATIVE EMULATORS PER GAME", alternativeEmulatorPerGame);
    s->addSaveFunc([alternativeEmulatorPerGame, s] {
        if (alternativeEmulatorPerGame->getState() !=
            Settings::getInstance()->getBool("AlternativeEmulatorPerGame")) {
            Settings::getInstance()->setBool("AlternativeEmulatorPerGame",
                                             alternativeEmulatorPerGame->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    // Show hidden files.
    auto show_hidden_files = std::make_shared<SwitchComponent>();
    show_hidden_files->setState(Settings::getInstance()->getBool("ShowHiddenFiles"));
    s->addWithLabel("SHOW HIDDEN FILES AND FOLDERS (REQUIRES RESTART)", show_hidden_files);
    s->addSaveFunc([show_hidden_files, s] {
        if (show_hidden_files->getState() != Settings::getInstance()->getBool("ShowHiddenFiles")) {
            Settings::getInstance()->setBool("ShowHiddenFiles", show_hidden_files->getState());
            s->setNeedsSaving();
        }
    });

    // Show hidden games.
    auto show_hidden_games = std::make_shared<SwitchComponent>();
    show_hidden_games->setState(Settings::getInstance()->getBool("ShowHiddenGames"));
    s->addWithLabel("SHOW HIDDEN GAMES (REQUIRES RESTART)", show_hidden_games);
    s->addSaveFunc([show_hidden_games, s] {
        if (show_hidden_games->getState() != Settings::getInstance()->getBool("ShowHiddenGames")) {
            Settings::getInstance()->setBool("ShowHiddenGames", show_hidden_games->getState());
            s->setNeedsSaving();
        }
    });

    // Custom event scripts, fired using Scripting::fireEvent().
    auto custom_eventscripts = std::make_shared<SwitchComponent>();
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
    auto parse_gamelist_only = std::make_shared<SwitchComponent>();
    parse_gamelist_only->setState(Settings::getInstance()->getBool("ParseGamelistOnly"));
    s->addWithLabel("ONLY SHOW ROMS FROM GAMELIST.XML FILES", parse_gamelist_only);
    s->addSaveFunc([parse_gamelist_only, s] {
        if (parse_gamelist_only->getState() !=
            Settings::getInstance()->getBool("ParseGamelistOnly")) {
            Settings::getInstance()->setBool("ParseGamelistOnly", parse_gamelist_only->getState());
            s->setNeedsSaving();
        }
    });

#if defined(__unix__)
    // Whether to disable desktop composition.
    auto disable_composition = std::make_shared<SwitchComponent>();
    disable_composition->setState(Settings::getInstance()->getBool("DisableComposition"));
    s->addWithLabel("DISABLE DESKTOP COMPOSITION (REQUIRES RESTART)", disable_composition);
    s->addSaveFunc([disable_composition, s] {
        if (disable_composition->getState() !=
            Settings::getInstance()->getBool("DisableComposition")) {
            Settings::getInstance()->setBool("DisableComposition", disable_composition->getState());
            s->setNeedsSaving();
        }
    });
#endif

    // GPU statistics overlay.
    auto display_gpu_statistics = std::make_shared<SwitchComponent>();
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

    // Whether to enable the menu in Kid mode.
    auto enable_menu_kid_mode = std::make_shared<SwitchComponent>();
    enable_menu_kid_mode->setState(Settings::getInstance()->getBool("EnableMenuKidMode"));
    s->addWithLabel("ENABLE MENU IN KID MODE", enable_menu_kid_mode);
    s->addSaveFunc([enable_menu_kid_mode, s] {
        if (Settings::getInstance()->getBool("EnableMenuKidMode") !=
            enable_menu_kid_mode->getState()) {
            Settings::getInstance()->setBool("EnableMenuKidMode", enable_menu_kid_mode->getState());
            s->setNeedsSaving();
        }
    });

// macOS requires root privileges to reboot and power off so it doesn't make much
// sense to enable this setting and menu entry for that operating system.
#if !defined(__APPLE__)
    // Whether to show the quit menu with the options to reboot and shutdown the computer.
    auto show_quit_menu = std::make_shared<SwitchComponent>();
    show_quit_menu->setState(Settings::getInstance()->getBool("ShowQuitMenu"));
    s->addWithLabel("SHOW QUIT MENU (REBOOT AND POWER OFF ENTRIES)", show_quit_menu);
    s->addSaveFunc([this, show_quit_menu, s] {
        if (show_quit_menu->getState() != Settings::getInstance()->getBool("ShowQuitMenu")) {
            Settings::getInstance()->setBool("ShowQuitMenu", show_quit_menu->getState());
            s->setNeedsSaving();
            GuiMenu::close(false);
        }
    });
#endif

    s->setSize(mSize);
    mWindow->pushGui(s);
}

void GuiMenu::openUtilitiesMenu()
{
    auto s = new GuiSettings("UTILITIES");
    s->setSize(mSize);
    mWindow->pushGui(s);
}

void GuiMenu::openQuitMenu()
{
    if (!Settings::getInstance()->getBool("ShowQuitMenu")) {
        mWindow->pushGui(new GuiMsgBox(
            this->getHelpStyle(), "REALLY QUIT?", "YES",
            [this] {
                Scripting::fireEvent("quit");
                close(true);
                Utils::Platform::quitES();
            },
            "NO", nullptr));
    }
    else {
        auto s = new GuiSettings("QUIT");

        Window* window {mWindow};
        HelpStyle style = getHelpStyle();

        ComponentListRow row;

        row.makeAcceptInputHandler([window, this] {
            window->pushGui(new GuiMsgBox(
                this->getHelpStyle(), "REALLY QUIT?", "YES",
                [this] {
                    Scripting::fireEvent("quit");
                    close(true);
                    Utils::Platform::quitES();
                },
                "NO", nullptr));
        });
        auto quitText = std::make_shared<TextComponent>("QUIT EMULATIONSTATION",
                                                        Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
        quitText->setSelectable(true);
        row.addElement(quitText, true);
        s->addRow(row);

        row.elements.clear();
        row.makeAcceptInputHandler([window, this] {
            window->pushGui(new GuiMsgBox(
                this->getHelpStyle(), "REALLY REBOOT?", "YES",
                [] {
                    Scripting::fireEvent("quit", "reboot");
                    Scripting::fireEvent("reboot");
                    if (Utils::Platform::quitES(Utils::Platform::QuitMode::REBOOT) != 0) {
                        LOG(LogWarning) << "Reboot terminated with non-zero result!";
                    }
                },
                "NO", nullptr));
        });
        auto rebootText = std::make_shared<TextComponent>("REBOOT SYSTEM",
                                                          Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
        rebootText->setSelectable(true);
        row.addElement(rebootText, true);
        s->addRow(row);

        row.elements.clear();
        row.makeAcceptInputHandler([window, this] {
            window->pushGui(new GuiMsgBox(
                this->getHelpStyle(), "REALLY POWER OFF?", "YES",
                [] {
                    Scripting::fireEvent("quit", "poweroff");
                    Scripting::fireEvent("poweroff");
                    if (Utils::Platform::quitES(Utils::Platform::QuitMode::POWEROFF) != 0) {
                        LOG(LogWarning) << "Power off terminated with non-zero result!";
                    }
                },
                "NO", nullptr));
        });
        auto powerOffText = std::make_shared<TextComponent>(
            "POWER OFF SYSTEM", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
        powerOffText->setSelectable(true);
        row.addElement(powerOffText, true);
        s->addRow(row);

        s->setSize(mSize);
        mWindow->pushGui(s);
    }
}

void GuiMenu::addVersionInfo()
{
    mVersion.setFont(Font::get(FONT_SIZE_SMALL));
    mVersion.setColor(0x5E5E5EFF);
    mVersion.setText("EMULATIONSTATION-DE  V" + Utils::String::toUpper(PROGRAM_VERSION_STRING));
    mVersion.setHorizontalAlignment(ALIGN_CENTER);
    addChild(&mVersion);
}

void GuiMenu::openMediaViewerOptions()
{
    mWindow->pushGui(new GuiMediaViewerOptions("MEDIA VIEWER SETTINGS"));
}

void GuiMenu::openScreensaverOptions()
{
    mWindow->pushGui(new GuiScreensaverOptions("SCREENSAVER SETTINGS"));
}

void GuiMenu::openCollectionSystemOptions()
{
    mWindow->pushGui(new GuiCollectionSystemsOptions("GAME COLLECTION SETTINGS"));
}

void GuiMenu::onSizeChanged()
{
    mVersion.setSize(mSize.x, 0.0f);
    mVersion.setPosition(0.0f, mSize.y - mVersion.getSize().y);
}

void GuiMenu::addEntry(const std::string& name,
                       unsigned int color,
                       bool add_arrow,
                       const std::function<void()>& func)
{
    std::shared_ptr<Font> font = Font::get(FONT_SIZE_MEDIUM);

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

void GuiMenu::close(bool closeAllWindows)
{
    std::function<void()> closeFunc;
    if (!closeAllWindows) {
        closeFunc = [this] { delete this; };
    }
    else {
        Window* window {mWindow};
        closeFunc = [window] {
            while (window->peekGui() != ViewController::getInstance())
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
    style.applyTheme(ViewController::getInstance()->getState().getSystem()->getTheme(), "system");
    return style;
}

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
// Why this is needed here is anyone's guess but without it the compilation fails.
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

    ViewController::getInstance()->startViewVideos();
}

void GuiMenu::openScraperOptions()
{
    // Open the scraper menu.
    mWindow->pushGui(new GuiScraperMenu("SCRAPER"));
}

void GuiMenu::openUIOptions()
{
    auto s = new GuiSettings("UI SETTINGS");

    // Theme options section.

    std::map<std::string, ThemeData::ThemeSet, ThemeData::StringComparator> themeSets {
        ThemeData::getThemeSets()};
    std::map<std::string, ThemeData::ThemeSet, ThemeData::StringComparator>::const_iterator
        selectedSet;

    auto themeSet =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "THEME SET", false);

    // Theme selection.
    if (!themeSets.empty()) {
        selectedSet = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
        if (selectedSet == themeSets.cend())
            selectedSet = themeSets.cbegin();

        for (auto it = themeSets.cbegin(); it != themeSets.cend(); ++it) {
            // If required, abbreviate the theme set name so it doesn't overlap the setting name.
            float maxNameLength = mSize.x * 0.62f;
            themeSet->add(it->first, it->first, it == selectedSet, maxNameLength);
        }
        s->addWithLabel("THEME SET", themeSet);
        s->addSaveFunc([this, themeSet, s] {
            if (themeSet->getSelected() != Settings::getInstance()->getString("ThemeSet")) {
                Scripting::fireEvent("theme-changed", themeSet->getSelected(),
                                     Settings::getInstance()->getString("ThemeSet"));
                Settings::getInstance()->setString("ThemeSet", themeSet->getSelected());
                mWindow->setChangedThemeSet();
                // This is required so that the custom collection system does not disappear
                // if the user is editing a custom collection when switching theme sets.
                if (CollectionSystemsManager::getInstance()->isEditing())
                    CollectionSystemsManager::getInstance()->exitEditMode();
                // TODO: Eliminate this extra reload or only execute it when switching from
                // a legacy theme to a non-legacy theme.
                ViewController::getInstance()->reloadAll();
                s->setNeedsSaving();
                s->setNeedsReloading();
                s->setNeedsGoToStart();
                s->setNeedsCollectionsUpdate();
                s->setInvalidateCachedBackground();
            }
        });
    }

    // Theme variants.
    auto themeVariant =
        std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "THEME VARIANT", false);
    s->addWithLabel("THEME VARIANT", themeVariant);
    s->addSaveFunc([themeVariant, s] {
        if (themeVariant->getSelected() != Settings::getInstance()->getString("ThemeVariant")) {
            Settings::getInstance()->setString("ThemeVariant", themeVariant->getSelected());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    auto themeVariantsFunc = [=](const std::string& selectedTheme,
                                 const std::string& selectedVariant) {
        std::map<std::string, ThemeData::ThemeSet, ThemeData::StringComparator>::const_iterator
            currentSet {themeSets.find(selectedTheme)};
        if (currentSet == themeSets.cend())
            return;
        // We need to recreate the OptionListComponent entries.
        themeVariant->clearEntries();
        int selectableVariants {0};
        for (auto& variant : currentSet->second.capabilities.variants) {
            if (variant.selectable)
                ++selectableVariants;
        }
        if (selectableVariants > 0) {
            for (auto& variant : currentSet->second.capabilities.variants) {
                if (variant.selectable) {
                    // If required, abbreviate the variant name so it doesn't overlap the
                    // setting name.
                    float maxNameLength {mSize.x * 0.62f};
                    themeVariant->add(variant.label, variant.name, variant.name == selectedVariant,
                                      maxNameLength);
                }
            }
            if (themeVariant->getSelectedObjects().size() == 0)
                themeVariant->selectEntry(0);
        }
        else {
            if (currentSet->second.capabilities.legacyTheme)
                themeVariant->add("Legacy theme set", "none", true);
            else
                themeVariant->add("None defined", "none", true);
            themeVariant->setEnabled(false);
            themeVariant->setOpacity(DISABLED_OPACITY);
            themeVariant->getParent()
                ->getChild(themeVariant->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
    };

    themeVariantsFunc(Settings::getInstance()->getString("ThemeSet"),
                      Settings::getInstance()->getString("ThemeVariant"));

    // Theme aspect ratios.
    auto themeAspectRatio = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "THEME ASPECT RATIO", false);
    s->addWithLabel("THEME ASPECT RATIO", themeAspectRatio);
    s->addSaveFunc([themeAspectRatio, s] {
        if (themeAspectRatio->getSelected() !=
            Settings::getInstance()->getString("ThemeAspectRatio")) {
            Settings::getInstance()->setString("ThemeAspectRatio", themeAspectRatio->getSelected());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    auto themeAspectRatiosFunc = [=](const std::string& selectedTheme,
                                     const std::string& selectedAspectRatio) {
        std::map<std::string, ThemeData::ThemeSet, ThemeData::StringComparator>::const_iterator
            currentSet {themeSets.find(selectedTheme)};
        if (currentSet == themeSets.cend())
            return;
        // We need to recreate the OptionListComponent entries.
        themeAspectRatio->clearEntries();
        if (currentSet->second.capabilities.aspectRatios.size() > 0) {
            for (auto& aspectRatio : currentSet->second.capabilities.aspectRatios)
                themeAspectRatio->add(ThemeData::getAspectRatioLabel(aspectRatio), aspectRatio,
                                      aspectRatio == selectedAspectRatio);
            if (themeAspectRatio->getSelectedObjects().size() == 0)
                themeAspectRatio->selectEntry(0);
        }
        else {
            if (currentSet->second.capabilities.legacyTheme)
                themeAspectRatio->add("Legacy theme set", "none", true);
            else
                themeAspectRatio->add("None defined", "none", true);
            themeAspectRatio->setEnabled(false);
            themeAspectRatio->setOpacity(DISABLED_OPACITY);
            themeAspectRatio->getParent()
                ->getChild(themeAspectRatio->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
    };

    themeAspectRatiosFunc(Settings::getInstance()->getString("ThemeSet"),
                          Settings::getInstance()->getString("ThemeAspectRatio"));

    // Legacy gamelist view style.
    auto gamelistViewStyle = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "LEGACY GAMELIST VIEW STYLE", false);
    std::string selectedViewStyle {Settings::getInstance()->getString("GamelistViewStyle")};
    gamelistViewStyle->add("automatic", "automatic", selectedViewStyle == "automatic");
    gamelistViewStyle->add("basic", "basic", selectedViewStyle == "basic");
    gamelistViewStyle->add("detailed", "detailed", selectedViewStyle == "detailed");
    gamelistViewStyle->add("video", "video", selectedViewStyle == "video");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the view style to Automatic in this case.
    if (gamelistViewStyle->getSelectedObjects().size() == 0)
        gamelistViewStyle->selectEntry(0);
    s->addWithLabel("LEGACY GAMELIST VIEW STYLE", gamelistViewStyle);
    s->addSaveFunc([gamelistViewStyle, s] {
        if (gamelistViewStyle->getSelected() !=
            Settings::getInstance()->getString("GamelistViewStyle")) {
            Settings::getInstance()->setString("GamelistViewStyle",
                                               gamelistViewStyle->getSelected());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    // Legacy ransition style.
    auto transitionStyle = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "LEGACY TRANSITION STYLE", false);
    std::vector<std::string> transitions;
    transitions.push_back("slide");
    transitions.push_back("fade");
    transitions.push_back("instant");
    for (auto it = transitions.cbegin(); it != transitions.cend(); ++it)
        transitionStyle->add(*it, *it,
                             Settings::getInstance()->getString("TransitionStyle") == *it);
    s->addWithLabel("LEGACY TRANSITION STYLE", transitionStyle);
    s->addSaveFunc([transitionStyle, s] {
        if (transitionStyle->getSelected() !=
            Settings::getInstance()->getString("TransitionStyle")) {
            Settings::getInstance()->setString("TransitionStyle", transitionStyle->getSelected());
            s->setNeedsSaving();
        }
    });

    // Optionally start in selected system/gamelist.
    auto startupSystem = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "GAMELIST ON STARTUP", false);
    startupSystem->add("NONE", "", Settings::getInstance()->getString("StartupSystem") == "");
    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        if ((*it)->getName() != "retropie") {
            // If required, abbreviate the system name so it doesn't overlap the setting name.
            float maxNameLength {mSize.x * 0.48f};
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

    // Default gamelist sort order.
    std::string sortOrder;
    auto defaultSortOrder = std::make_shared<OptionListComponent<const FileData::SortType*>>(
        getHelpStyle(), "DEFAULT SORT ORDER", false);
    // Exclude the System sort options.
    unsigned int numSortTypes {static_cast<unsigned int>(FileSorts::SortTypes.size() - 2)};
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
        const FileData::SortType& sort {FileSorts::SortTypes[i]};
        if (sort.description == sortOrder)
            defaultSortOrder->add(sort.description, &sort, true);
        else
            defaultSortOrder->add(sort.description, &sort, false);
    }
    s->addWithLabel("DEFAULT SORT ORDER", defaultSortOrder);
    s->addSaveFunc([defaultSortOrder, sortOrder, s] {
        std::string selectedSortOrder {defaultSortOrder.get()->getSelected()->description};
        if (selectedSortOrder != sortOrder) {
            Settings::getInstance()->setString("DefaultSortOrder", selectedSortOrder);
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setNeedsSortingCollections();
            s->setInvalidateCachedBackground();
        }
    });

    // Open menu effect.
    auto menuOpeningEffect = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "MENU OPENING EFFECT", false);
    std::string selectedMenuEffect {Settings::getInstance()->getString("MenuOpeningEffect")};
    menuOpeningEffect->add("SCALE-UP", "scale-up", selectedMenuEffect == "scale-up");
    menuOpeningEffect->add("NONE", "none", selectedMenuEffect == "none");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the opening effect to "scale-up" in this case.
    if (menuOpeningEffect->getSelectedObjects().size() == 0)
        menuOpeningEffect->selectEntry(0);
    s->addWithLabel("MENU OPENING EFFECT", menuOpeningEffect);
    s->addSaveFunc([menuOpeningEffect, s] {
        if (menuOpeningEffect->getSelected() !=
            Settings::getInstance()->getString("MenuOpeningEffect")) {
            Settings::getInstance()->setString("MenuOpeningEffect",
                                               menuOpeningEffect->getSelected());
            s->setNeedsSaving();
        }
    });

    // Launch screen duration.
    auto launchScreenDuration = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "LAUNCH SCREEN DURATION", false);
    std::string selectedDuration {Settings::getInstance()->getString("LaunchScreenDuration")};
    launchScreenDuration->add("NORMAL", "normal", selectedDuration == "normal");
    launchScreenDuration->add("BRIEF", "brief", selectedDuration == "brief");
    launchScreenDuration->add("LONG", "long", selectedDuration == "long");
    launchScreenDuration->add("DISABLED", "disabled", selectedDuration == "disabled");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the duration to "normal" in this case.
    if (launchScreenDuration->getSelectedObjects().size() == 0)
        launchScreenDuration->selectEntry(0);
    s->addWithLabel("LAUNCH SCREEN DURATION", launchScreenDuration);
    s->addSaveFunc([launchScreenDuration, s] {
        if (launchScreenDuration->getSelected() !=
            Settings::getInstance()->getString("LaunchScreenDuration")) {
            Settings::getInstance()->setString("LaunchScreenDuration",
                                               launchScreenDuration->getSelected());
            s->setNeedsSaving();
        }
    });

    // UI mode.
    auto uiMode =
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
        uiMode->add(*it, *it, setMode == *it);
    s->addWithLabel("UI MODE", uiMode);
    s->addSaveFunc([uiMode, this, s] {
        std::string selectedMode {uiMode->getSelected()};
        // If any of the force flags are set, then always apply and save the setting.
        if (selectedMode == Settings::getInstance()->getString("UIMode") &&
            !Settings::getInstance()->getBool("ForceFull") &&
            !Settings::getInstance()->getBool("ForceKiosk") &&
            !Settings::getInstance()->getBool("ForceKid")) {
            return;
        }
        else if (selectedMode != "full") {
            std::string msg {"YOU ARE CHANGING THE UI TO THE RESTRICTED MODE\n'" +
                             Utils::String::toUpper(selectedMode) + "'\n"};
            if (selectedMode == "kiosk") {
                msg.append("THIS WILL HIDE MOST MENU OPTIONS TO PREVENT\n");
                msg.append("CHANGES TO THE SYSTEM\n");
            }
            else {
                msg.append("THIS WILL LIMIT THE AVAILABLE GAMES TO THE ONES\n");
                msg.append("FLAGGED SUITABLE FOR CHILDREN\n");
            }
            msg.append("TO UNLOCK AND RETURN TO THE FULL UI, ENTER THIS CODE: \n");
            msg.append(UIModeController::getInstance()->getFormattedPassKeyStr() + "\n\n");
            msg.append("DO YOU WANT TO PROCEED?");
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
            Settings::getInstance()->setString("UIMode", uiMode->getSelected());
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

    // Media viewer.
    ComponentListRow mediaViewerRow;
    mediaViewerRow.elements.clear();
    mediaViewerRow.addElement(std::make_shared<TextComponent>(
                                  "MEDIA VIEWER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                              true);
    mediaViewerRow.addElement(makeArrow(), false);
    mediaViewerRow.makeAcceptInputHandler(std::bind(&GuiMenu::openMediaViewerOptions, this));
    s->addRow(mediaViewerRow);

    // Screensaver.
    ComponentListRow screensaverRow;
    screensaverRow.elements.clear();
    screensaverRow.addElement(std::make_shared<TextComponent>(
                                  "SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                              true);
    screensaverRow.addElement(makeArrow(), false);
    screensaverRow.makeAcceptInputHandler(std::bind(&GuiMenu::openScreensaverOptions, this));
    s->addRow(screensaverRow);

    // Blur background when the menu is open.
    auto menuBlurBackground = std::make_shared<SwitchComponent>();
    menuBlurBackground->setState(Settings::getInstance()->getBool("MenuBlurBackground"));
    s->addWithLabel("BLUR BACKGROUND WHEN MENU IS OPEN", menuBlurBackground);
    s->addSaveFunc([menuBlurBackground, s] {
        if (menuBlurBackground->getState() !=
            Settings::getInstance()->getBool("MenuBlurBackground")) {
            Settings::getInstance()->setBool("MenuBlurBackground", menuBlurBackground->getState());
            s->setNeedsSaving();
            s->setInvalidateCachedBackground();
        }
    });

    // Display pillarboxes (and letterboxes) for videos in the gamelists.
    auto gamelistVideoPillarbox = std::make_shared<SwitchComponent>();
    gamelistVideoPillarbox->setState(Settings::getInstance()->getBool("GamelistVideoPillarbox"));
    s->addWithLabel("DISPLAY PILLARBOXES FOR GAMELIST VIDEOS", gamelistVideoPillarbox);
    s->addSaveFunc([gamelistVideoPillarbox, s] {
        if (gamelistVideoPillarbox->getState() !=
            Settings::getInstance()->getBool("GamelistVideoPillarbox")) {
            Settings::getInstance()->setBool("GamelistVideoPillarbox",
                                             gamelistVideoPillarbox->getState());
            s->setNeedsSaving();
        }
    });

    // Render scanlines for videos in the gamelists.
    auto gamelistVideoScanlines = std::make_shared<SwitchComponent>();
    gamelistVideoScanlines->setState(Settings::getInstance()->getBool("GamelistVideoScanlines"));
    s->addWithLabel("RENDER SCANLINES FOR GAMELIST VIDEOS", gamelistVideoScanlines);
    s->addSaveFunc([gamelistVideoScanlines, s] {
        if (gamelistVideoScanlines->getState() !=
            Settings::getInstance()->getBool("GamelistVideoScanlines")) {
            Settings::getInstance()->setBool("GamelistVideoScanlines",
                                             gamelistVideoScanlines->getState());
            s->setNeedsSaving();
        }
    });

    // Sort folders on top of the gamelists.
    auto foldersOnTop = std::make_shared<SwitchComponent>();
    foldersOnTop->setState(Settings::getInstance()->getBool("FoldersOnTop"));
    s->addWithLabel("SORT FOLDERS ON TOP OF GAMELISTS", foldersOnTop);
    s->addSaveFunc([foldersOnTop, s] {
        if (foldersOnTop->getState() != Settings::getInstance()->getBool("FoldersOnTop")) {
            Settings::getInstance()->setBool("FoldersOnTop", foldersOnTop->getState());
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setInvalidateCachedBackground();
        }
    });

    // Sort favorites on top of non-favorites in the gamelists.
    auto favoritesFirst = std::make_shared<SwitchComponent>();
    favoritesFirst->setState(Settings::getInstance()->getBool("FavoritesFirst"));
    s->addWithLabel("SORT FAVORITE GAMES ABOVE NON-FAVORITES", favoritesFirst);
    s->addSaveFunc([favoritesFirst, s] {
        if (favoritesFirst->getState() != Settings::getInstance()->getBool("FavoritesFirst")) {
            Settings::getInstance()->setBool("FavoritesFirst", favoritesFirst->getState());
            s->setNeedsSaving();
            s->setNeedsSorting();
            s->setNeedsSortingCollections();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable gamelist star markings for favorite games.
    auto favoritesStar = std::make_shared<SwitchComponent>();
    favoritesStar->setState(Settings::getInstance()->getBool("FavoritesStar"));
    s->addWithLabel("ADD STAR MARKINGS TO FAVORITE GAMES", favoritesStar);
    s->addSaveFunc([favoritesStar, s] {
        if (favoritesStar->getState() != Settings::getInstance()->getBool("FavoritesStar")) {
            Settings::getInstance()->setBool("FavoritesStar", favoritesStar->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable quick list scrolling overlay.
    auto listScrollOverlay = std::make_shared<SwitchComponent>();
    listScrollOverlay->setState(Settings::getInstance()->getBool("ListScrollOverlay"));
    s->addWithLabel("ENABLE QUICK LIST SCROLLING OVERLAY", listScrollOverlay);
    s->addSaveFunc([listScrollOverlay, s] {
        if (listScrollOverlay->getState() !=
            Settings::getInstance()->getBool("ListScrollOverlay")) {
            Settings::getInstance()->setBool("ListScrollOverlay", listScrollOverlay->getState());
            s->setNeedsSaving();
        }
    });

    // Enable virtual (on-screen) keyboard.
    auto virtualKeyboard = std::make_shared<SwitchComponent>();
    virtualKeyboard->setState(Settings::getInstance()->getBool("VirtualKeyboard"));
    s->addWithLabel("ENABLE VIRTUAL KEYBOARD", virtualKeyboard);
    s->addSaveFunc([virtualKeyboard, s] {
        if (virtualKeyboard->getState() != Settings::getInstance()->getBool("VirtualKeyboard")) {
            Settings::getInstance()->setBool("VirtualKeyboard", virtualKeyboard->getState());
            s->setNeedsSaving();
            s->setInvalidateCachedBackground();
        }
    });

    // Enable the 'Y' button for tagging games as favorites.
    auto favoritesAddButton = std::make_shared<SwitchComponent>();
    favoritesAddButton->setState(Settings::getInstance()->getBool("FavoritesAddButton"));
    s->addWithLabel("ENABLE TOGGLE FAVORITES BUTTON", favoritesAddButton);
    s->addSaveFunc([favoritesAddButton, s] {
        if (Settings::getInstance()->getBool("FavoritesAddButton") !=
            favoritesAddButton->getState()) {
            Settings::getInstance()->setBool("FavoritesAddButton", favoritesAddButton->getState());
            s->setNeedsSaving();
        }
    });

    // Enable the thumbstick click buttons for jumping to a random system or game.
    auto randomAddButton = std::make_shared<SwitchComponent>();
    randomAddButton->setState(Settings::getInstance()->getBool("RandomAddButton"));
    s->addWithLabel("ENABLE RANDOM SYSTEM OR GAME BUTTON", randomAddButton);
    s->addSaveFunc([randomAddButton, s] {
        if (Settings::getInstance()->getBool("RandomAddButton") != randomAddButton->getState()) {
            Settings::getInstance()->setBool("RandomAddButton", randomAddButton->getState());
            s->setNeedsSaving();
        }
    });

    // Gamelist filters.
    auto gamelistFilters = std::make_shared<SwitchComponent>();
    gamelistFilters->setState(Settings::getInstance()->getBool("GamelistFilters"));
    s->addWithLabel("ENABLE GAMELIST FILTERS", gamelistFilters);
    s->addSaveFunc([gamelistFilters, s] {
        if (Settings::getInstance()->getBool("GamelistFilters") != gamelistFilters->getState()) {
            Settings::getInstance()->setBool("GamelistFilters", gamelistFilters->getState());
            s->setNeedsSaving();
            s->setNeedsReloading();
        }
    });

    // Quick system select (navigate left/right in gamelist view).
    auto quickSystemSelect = std::make_shared<SwitchComponent>();
    quickSystemSelect->setState(Settings::getInstance()->getBool("QuickSystemSelect"));
    s->addWithLabel("ENABLE QUICK SYSTEM SELECT", quickSystemSelect);
    s->addSaveFunc([quickSystemSelect, s] {
        if (Settings::getInstance()->getBool("QuickSystemSelect") !=
            quickSystemSelect->getState()) {
            Settings::getInstance()->setBool("QuickSystemSelect", quickSystemSelect->getState());
            s->setNeedsSaving();
        }
    });

    // On-screen help prompts.
    auto showHelpPrompts = std::make_shared<SwitchComponent>();
    showHelpPrompts->setState(Settings::getInstance()->getBool("ShowHelpPrompts"));
    s->addWithLabel("DISPLAY ON-SCREEN HELP", showHelpPrompts);
    s->addSaveFunc([showHelpPrompts, s] {
        if (Settings::getInstance()->getBool("ShowHelpPrompts") != showHelpPrompts->getState()) {
            Settings::getInstance()->setBool("ShowHelpPrompts", showHelpPrompts->getState());
            s->setNeedsSaving();
        }
    });

    // When the theme set entries are scrolled or selected, update the relevant rows.
    auto scrollThemeSetFunc = [=](const std::string& themeName, bool firstRun = false) {
        auto selectedSet = themeSets.find(themeName);
        if (selectedSet == themeSets.cend())
            return;
        if (!firstRun) {
            themeVariantsFunc(themeName, themeVariant->getSelected());
            themeAspectRatiosFunc(themeName, themeAspectRatio->getSelected());
        }
        int selectableVariants {0};
        for (auto& variant : selectedSet->second.capabilities.variants) {
            if (variant.selectable)
                ++selectableVariants;
        }
        if (!selectedSet->second.capabilities.legacyTheme && selectableVariants > 0) {
            themeVariant->setEnabled(true);
            themeVariant->setOpacity(1.0f);
            themeVariant->getParent()
                ->getChild(themeVariant->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
        else {
            themeVariant->setEnabled(false);
            themeVariant->setOpacity(DISABLED_OPACITY);
            themeVariant->getParent()
                ->getChild(themeVariant->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }

        if (!selectedSet->second.capabilities.legacyTheme &&
            selectedSet->second.capabilities.aspectRatios.size() > 0) {
            themeAspectRatio->setEnabled(true);
            themeAspectRatio->setOpacity(1.0f);
            themeAspectRatio->getParent()
                ->getChild(themeAspectRatio->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
        else {
            themeAspectRatio->setEnabled(false);
            themeAspectRatio->setOpacity(DISABLED_OPACITY);
            themeAspectRatio->getParent()
                ->getChild(themeAspectRatio->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        if (!selectedSet->second.capabilities.legacyTheme) {
            gamelistViewStyle->setEnabled(false);
            gamelistViewStyle->setOpacity(DISABLED_OPACITY);
            gamelistViewStyle->getParent()
                ->getChild(gamelistViewStyle->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
            // TEMPORARY
            // transitionStyle->setEnabled(false);
            // transitionStyle->setOpacity(DISABLED_OPACITY);
            // transitionStyle->getParent()
            //    ->getChild(transitionStyle->getChildIndex() - 1)
            //    ->setOpacity(DISABLED_OPACITY);

            // Pillarboxes are theme-controlled for newer themes.
            gamelistVideoPillarbox->setEnabled(false);
            gamelistVideoPillarbox->setOpacity(DISABLED_OPACITY);
            gamelistVideoPillarbox->getParent()
                ->getChild(gamelistVideoPillarbox->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);

            // Scanlines are theme-controlled for newer themes.
            gamelistVideoScanlines->setEnabled(false);
            gamelistVideoScanlines->setOpacity(DISABLED_OPACITY);
            gamelistVideoScanlines->getParent()
                ->getChild(gamelistVideoScanlines->getChildIndex() - 1)
                ->setOpacity(DISABLED_OPACITY);
        }
        else {
            gamelistViewStyle->setEnabled(true);
            gamelistViewStyle->setOpacity(1.0f);
            gamelistViewStyle->getParent()
                ->getChild(gamelistViewStyle->getChildIndex() - 1)
                ->setOpacity(1.0f);

            transitionStyle->setEnabled(true);
            transitionStyle->setOpacity(1.0f);
            transitionStyle->getParent()
                ->getChild(transitionStyle->getChildIndex() - 1)
                ->setOpacity(1.0f);

            gamelistVideoPillarbox->setEnabled(true);
            gamelistVideoPillarbox->setOpacity(1.0f);
            gamelistVideoPillarbox->getParent()
                ->getChild(gamelistVideoPillarbox->getChildIndex() - 1)
                ->setOpacity(1.0f);

            gamelistVideoScanlines->setEnabled(true);
            gamelistVideoScanlines->setOpacity(1.0f);
            gamelistVideoScanlines->getParent()
                ->getChild(gamelistVideoScanlines->getChildIndex() - 1)
                ->setOpacity(1.0f);
        }
    };

    scrollThemeSetFunc(selectedSet->first, true);
    themeSet->setCallback(scrollThemeSetFunc);

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
    int currentVolume {volumeControl.getVolume()};

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
    auto soundVolumeNavigation = std::make_shared<SliderComponent>(0.f, 100.f, 1.f, "%");
    soundVolumeNavigation->setValue(
        static_cast<float>(Settings::getInstance()->getInt("SoundVolumeNavigation")));
    s->addWithLabel("NAVIGATION SOUNDS VOLUME", soundVolumeNavigation);
    s->addSaveFunc([soundVolumeNavigation, s] {
        if (soundVolumeNavigation->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("SoundVolumeNavigation"))) {
            Settings::getInstance()->setInt("SoundVolumeNavigation",
                                            static_cast<int>(soundVolumeNavigation->getValue()));
            s->setNeedsSaving();
        }
    });

    // Volume for videos.
    auto soundVolumeVideos = std::make_shared<SliderComponent>(0.f, 100.f, 1.f, "%");
    soundVolumeVideos->setValue(
        static_cast<float>(Settings::getInstance()->getInt("SoundVolumeVideos")));
    s->addWithLabel("VIDEO PLAYER VOLUME", soundVolumeVideos);
    s->addSaveFunc([soundVolumeVideos, s] {
        if (soundVolumeVideos->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("SoundVolumeVideos"))) {
            Settings::getInstance()->setInt("SoundVolumeVideos",
                                            static_cast<int>(soundVolumeVideos->getValue()));
            s->setNeedsSaving();
        }
    });

    if (UIModeController::getInstance()->isUIModeFull()) {
        // Play audio for gamelist videos.
        auto viewsVideoAudio = std::make_shared<SwitchComponent>();
        viewsVideoAudio->setState(Settings::getInstance()->getBool("ViewsVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR GAMELIST AND SYSTEM VIEW VIDEOS", viewsVideoAudio);
        s->addSaveFunc([viewsVideoAudio, s] {
            if (viewsVideoAudio->getState() !=
                Settings::getInstance()->getBool("ViewsVideoAudio")) {
                Settings::getInstance()->setBool("ViewsVideoAudio", viewsVideoAudio->getState());
                s->setNeedsSaving();
            }
        });

        // Play audio for media viewer videos.
        auto mediaViewerVideoAudio = std::make_shared<SwitchComponent>();
        mediaViewerVideoAudio->setState(Settings::getInstance()->getBool("MediaViewerVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR MEDIA VIEWER VIDEOS", mediaViewerVideoAudio);
        s->addSaveFunc([mediaViewerVideoAudio, s] {
            if (mediaViewerVideoAudio->getState() !=
                Settings::getInstance()->getBool("MediaViewerVideoAudio")) {
                Settings::getInstance()->setBool("MediaViewerVideoAudio",
                                                 mediaViewerVideoAudio->getState());
                s->setNeedsSaving();
            }
        });

        // Play audio for screensaver videos.
        auto screensaverVideoAudio = std::make_shared<SwitchComponent>();
        screensaverVideoAudio->setState(Settings::getInstance()->getBool("ScreensaverVideoAudio"));
        s->addWithLabel("PLAY AUDIO FOR SCREENSAVER VIDEOS", screensaverVideoAudio);
        s->addSaveFunc([screensaverVideoAudio, s] {
            if (screensaverVideoAudio->getState() !=
                Settings::getInstance()->getBool("ScreensaverVideoAudio")) {
                Settings::getInstance()->setBool("ScreensaverVideoAudio",
                                                 screensaverVideoAudio->getState());
                s->setNeedsSaving();
            }
        });

        // Navigation sounds.
        auto navigationSounds = std::make_shared<SwitchComponent>();
        navigationSounds->setState(Settings::getInstance()->getBool("NavigationSounds"));
        s->addWithLabel("ENABLE NAVIGATION SOUNDS", navigationSounds);
        s->addSaveFunc([navigationSounds, s] {
            if (navigationSounds->getState() !=
                Settings::getInstance()->getBool("NavigationSounds")) {
                Settings::getInstance()->setBool("NavigationSounds", navigationSounds->getState());
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
    auto inputControllerType = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "CONTROLLER TYPE", false);
    std::string selectedPlayer {Settings::getInstance()->getString("InputControllerType")};
    inputControllerType->add("XBOX", "xbox", selectedPlayer == "xbox");
    inputControllerType->add("XBOX 360", "xbox360", selectedPlayer == "xbox360");
    inputControllerType->add("PLAYSTATION 4", "ps4", selectedPlayer == "ps4");
    inputControllerType->add("PLAYSTATION 5", "ps5", selectedPlayer == "ps5");
    inputControllerType->add("SNES", "snes", selectedPlayer == "snes");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the controller type to "xbox" in this case.
    if (inputControllerType->getSelectedObjects().size() == 0)
        inputControllerType->selectEntry(0);
    s->addWithLabel("CONTROLLER TYPE", inputControllerType);
    s->addSaveFunc([inputControllerType, s] {
        if (inputControllerType->getSelected() !=
            Settings::getInstance()->getString("InputControllerType")) {
            Settings::getInstance()->setString("InputControllerType",
                                               inputControllerType->getSelected());
            s->setNeedsReloadHelpPrompts();
            s->setNeedsSaving();
        }
    });

    // Whether to only accept input from the first controller.
    auto inputOnlyFirstController = std::make_shared<SwitchComponent>();
    inputOnlyFirstController->setState(
        Settings::getInstance()->getBool("InputOnlyFirstController"));
    s->addWithLabel("ONLY ACCEPT INPUT FROM FIRST CONTROLLER", inputOnlyFirstController);
    s->addSaveFunc([inputOnlyFirstController, s] {
        if (Settings::getInstance()->getBool("InputOnlyFirstController") !=
            inputOnlyFirstController->getState()) {
            Settings::getInstance()->setBool("InputOnlyFirstController",
                                             inputOnlyFirstController->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to ignore keyboard input (except the quit shortcut).
    auto inputIgnoreKeyboard = std::make_shared<SwitchComponent>();
    inputIgnoreKeyboard->setState(Settings::getInstance()->getBool("InputIgnoreKeyboard"));
    s->addWithLabel("IGNORE KEYBOARD INPUT", inputIgnoreKeyboard);
    s->addSaveFunc([inputIgnoreKeyboard, s] {
        if (Settings::getInstance()->getBool("InputIgnoreKeyboard") !=
            inputIgnoreKeyboard->getState()) {
            Settings::getInstance()->setBool("InputIgnoreKeyboard",
                                             inputIgnoreKeyboard->getState());
            s->setNeedsSaving();
        }
    });

    // Configure keyboard and controllers.
    ComponentListRow configureInputRow;
    configureInputRow.elements.clear();
    configureInputRow.addElement(
        std::make_shared<TextComponent>("CONFIGURE KEYBOARD AND CONTROLLERS",
                                        Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
        true);
    configureInputRow.addElement(makeArrow(), false);
    configureInputRow.makeAcceptInputHandler(std::bind(&GuiMenu::openConfigInput, this, s));
    s->addRow(configureInputRow);

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

    std::string message {"THE KEYBOARD AND CONTROLLERS ARE AUTOMATICALLY\n"
                         "CONFIGURED, BUT USING THIS CONFIGURATION TOOL\n"
                         "YOU CAN OVERRIDE THE DEFAULT BUTTON MAPPINGS\n"
                         "(THIS WILL NOT AFFECT THE HELP PROMPTS)\n"
                         "CONTINUE?"};

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
    auto mediaDirectory = std::make_shared<TextComponent>("GAME MEDIA DIRECTORY",
                                                          Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
    auto bracketMediaDirectory = std::make_shared<ImageComponent>();
    bracketMediaDirectory->setResize(
        glm::vec2 {0.0f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()});
    bracketMediaDirectory->setImage(":/graphics/arrow.svg");
    rowMediaDir.addElement(mediaDirectory, true);
    rowMediaDir.addElement(bracketMediaDirectory, false);
    std::string titleMediaDir {"ENTER GAME MEDIA DIRECTORY"};
    std::string mediaDirectoryStaticText {"Default directory:"};
    std::string defaultDirectoryText {"~/.emulationstation/downloaded_media/"};
    std::string initValueMediaDir {Settings::getInstance()->getString("MediaDirectory")};
    bool multiLineMediaDir {false};
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
    auto maxVram = std::make_shared<SliderComponent>(80.f, 1024.f, 8.f, "MiB");
    maxVram->setValue(static_cast<float>(Settings::getInstance()->getInt("MaxVRAM")));
    s->addWithLabel("VRAM LIMIT", maxVram);
    s->addSaveFunc([maxVram, s] {
        if (maxVram->getValue() != Settings::getInstance()->getInt("MaxVRAM")) {
            Settings::getInstance()->setInt("MaxVRAM",
                                            static_cast<int>(std::round(maxVram->getValue())));
            s->setNeedsSaving();
        }
    });

    // Display/monitor.
    auto displayIndex = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "DISPLAY/MONITOR INDEX", false);
    std::vector<std::string> displayIndexEntry;
    displayIndexEntry.push_back("1");
    displayIndexEntry.push_back("2");
    displayIndexEntry.push_back("3");
    displayIndexEntry.push_back("4");
    for (auto it = displayIndexEntry.cbegin(); it != displayIndexEntry.cend(); ++it)
        displayIndex->add(*it, *it,
                          Settings::getInstance()->getInt("DisplayIndex") == atoi((*it).c_str()));
    s->addWithLabel("DISPLAY/MONITOR INDEX (REQUIRES RESTART)", displayIndex);
    s->addSaveFunc([displayIndex, s] {
        if (atoi(displayIndex->getSelected().c_str()) !=
            Settings::getInstance()->getInt("DisplayIndex")) {
            Settings::getInstance()->setInt("DisplayIndex",
                                            atoi(displayIndex->getSelected().c_str()));
            s->setNeedsSaving();
        }
    });

    // Keyboard quit shortcut.
    auto keyboardQuitShortcut = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "KEYBOARD QUIT SHORTCUT", false);
    std::string selectedShortcut {Settings::getInstance()->getString("KeyboardQuitShortcut")};
#if defined(_WIN64) || defined(__unix__)
    keyboardQuitShortcut->add("Alt + F4", "AltF4", selectedShortcut == "AltF4");
    keyboardQuitShortcut->add("Ctrl + Q", "CtrlQ", selectedShortcut == "CtrlQ");
    keyboardQuitShortcut->add("Alt + Q", "AltQ", selectedShortcut == "AltQ");
#endif
#if defined(__APPLE__)
    keyboardQuitShortcut->add("\u2318 + Q", "CmdQ", selectedShortcut == "CmdQ");
    keyboardQuitShortcut->add("Ctrl + Q", "CtrlQ", selectedShortcut == "CtrlQ");
    keyboardQuitShortcut->add("Alt + Q", "AltQ", selectedShortcut == "AltQ");
#endif
    keyboardQuitShortcut->add("F4", "F4", selectedShortcut == "F4");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the keyboard quit shortcut to the first entry in this case.
    if (keyboardQuitShortcut->getSelectedObjects().size() == 0)
        keyboardQuitShortcut->selectEntry(0);
    s->addWithLabel("KEYBOARD QUIT SHORTCUT", keyboardQuitShortcut);
    s->addSaveFunc([keyboardQuitShortcut, s] {
        if (keyboardQuitShortcut->getSelected() !=
            Settings::getInstance()->getString("KeyboardQuitShortcut")) {
            Settings::getInstance()->setString("KeyboardQuitShortcut",
                                               keyboardQuitShortcut->getSelected());
            s->setNeedsSaving();
        }
    });

    // When to save game metadata.
    auto saveGamelistsMode = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "WHEN TO SAVE METADATA", false);
    std::vector<std::string> saveModes;
    saveModes.push_back("on exit");
    saveModes.push_back("always");
    saveModes.push_back("never");
    for (auto it = saveModes.cbegin(); it != saveModes.cend(); ++it) {
        saveGamelistsMode->add(*it, *it,
                               Settings::getInstance()->getString("SaveGamelistsMode") == *it);
    }
    s->addWithLabel("WHEN TO SAVE GAME METADATA", saveGamelistsMode);
    s->addSaveFunc([saveGamelistsMode, s] {
        if (saveGamelistsMode->getSelected() !=
            Settings::getInstance()->getString("SaveGamelistsMode")) {
            Settings::getInstance()->setString("SaveGamelistsMode",
                                               saveGamelistsMode->getSelected());
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
    auto runInBackground = std::make_shared<SwitchComponent>();
    runInBackground->setState(Settings::getInstance()->getBool("RunInBackground"));
    s->addWithLabel("RUN IN BACKGROUND (WHILE GAME IS LAUNCHED)", runInBackground);
    s->addSaveFunc([runInBackground, s] {
        if (runInBackground->getState() != Settings::getInstance()->getBool("RunInBackground")) {
            Settings::getInstance()->setBool("RunInBackground", runInBackground->getState());
            s->setNeedsSaving();
        }
    });

#if defined(VIDEO_HW_DECODING)
    // Whether to enable hardware decoding for the FFmpeg video player.
    auto videoHardwareDecoding = std::make_shared<SwitchComponent>();
    videoHardwareDecoding->setState(Settings::getInstance()->getBool("VideoHardwareDecoding"));
    s->addWithLabel("VIDEO HARDWARE DECODING (EXPERIMENTAL)", videoHardwareDecoding);
    s->addSaveFunc([videoHardwareDecoding, s] {
        if (videoHardwareDecoding->getState() !=
            Settings::getInstance()->getBool("VideoHardwareDecoding")) {
            Settings::getInstance()->setBool("VideoHardwareDecoding",
                                             videoHardwareDecoding->getState());
            s->setNeedsSaving();
        }
    });
#endif

    // Whether to upscale the video frame rate to 60 FPS.
    auto videoUpscaleFrameRate = std::make_shared<SwitchComponent>();
    videoUpscaleFrameRate->setState(Settings::getInstance()->getBool("VideoUpscaleFrameRate"));
    s->addWithLabel("UPSCALE VIDEO FRAME RATE TO 60 FPS", videoUpscaleFrameRate);
    s->addSaveFunc([videoUpscaleFrameRate, s] {
        if (videoUpscaleFrameRate->getState() !=
            Settings::getInstance()->getBool("VideoUpscaleFrameRate")) {
            Settings::getInstance()->setBool("VideoUpscaleFrameRate",
                                             videoUpscaleFrameRate->getState());
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
    auto showHiddenFiles = std::make_shared<SwitchComponent>();
    showHiddenFiles->setState(Settings::getInstance()->getBool("ShowHiddenFiles"));
    s->addWithLabel("SHOW HIDDEN FILES AND FOLDERS (REQUIRES RESTART)", showHiddenFiles);
    s->addSaveFunc([showHiddenFiles, s] {
        if (showHiddenFiles->getState() != Settings::getInstance()->getBool("ShowHiddenFiles")) {
            Settings::getInstance()->setBool("ShowHiddenFiles", showHiddenFiles->getState());
            s->setNeedsSaving();
        }
    });

    // Show hidden games.
    auto showHiddenGames = std::make_shared<SwitchComponent>();
    showHiddenGames->setState(Settings::getInstance()->getBool("ShowHiddenGames"));
    s->addWithLabel("SHOW HIDDEN GAMES (REQUIRES RESTART)", showHiddenGames);
    s->addSaveFunc([showHiddenGames, s] {
        if (showHiddenGames->getState() != Settings::getInstance()->getBool("ShowHiddenGames")) {
            Settings::getInstance()->setBool("ShowHiddenGames", showHiddenGames->getState());
            s->setNeedsSaving();
        }
    });

    // Custom event scripts, fired using Scripting::fireEvent().
    auto customEventScripts = std::make_shared<SwitchComponent>();
    customEventScripts->setState(Settings::getInstance()->getBool("CustomEventScripts"));
    s->addWithLabel("ENABLE CUSTOM EVENT SCRIPTS", customEventScripts);
    s->addSaveFunc([customEventScripts, s] {
        if (customEventScripts->getState() !=
            Settings::getInstance()->getBool("CustomEventScripts")) {
            Settings::getInstance()->setBool("CustomEventScripts", customEventScripts->getState());
            s->setNeedsSaving();
        }
    });

    // Only show ROMs included in the gamelist.xml files.
    auto parseGamelistOnly = std::make_shared<SwitchComponent>();
    parseGamelistOnly->setState(Settings::getInstance()->getBool("ParseGamelistOnly"));
    s->addWithLabel("ONLY SHOW ROMS FROM GAMELIST.XML FILES", parseGamelistOnly);
    s->addSaveFunc([parseGamelistOnly, s] {
        if (parseGamelistOnly->getState() !=
            Settings::getInstance()->getBool("ParseGamelistOnly")) {
            Settings::getInstance()->setBool("ParseGamelistOnly", parseGamelistOnly->getState());
            s->setNeedsSaving();
        }
    });

#if defined(__unix__)
    // Whether to disable desktop composition.
    auto disableComposition = std::make_shared<SwitchComponent>();
    disableComposition->setState(Settings::getInstance()->getBool("DisableComposition"));
    s->addWithLabel("DISABLE DESKTOP COMPOSITION (REQUIRES RESTART)", disableComposition);
    s->addSaveFunc([disableComposition, s] {
        if (disableComposition->getState() !=
            Settings::getInstance()->getBool("DisableComposition")) {
            Settings::getInstance()->setBool("DisableComposition", disableComposition->getState());
            s->setNeedsSaving();
        }
    });
#endif

    // GPU statistics overlay.
    auto displayGpuStatistics = std::make_shared<SwitchComponent>();
    displayGpuStatistics->setState(Settings::getInstance()->getBool("DisplayGPUStatistics"));
    s->addWithLabel("DISPLAY GPU STATISTICS OVERLAY", displayGpuStatistics);
    s->addSaveFunc([displayGpuStatistics, s] {
        if (displayGpuStatistics->getState() !=
            Settings::getInstance()->getBool("DisplayGPUStatistics")) {
            Settings::getInstance()->setBool("DisplayGPUStatistics",
                                             displayGpuStatistics->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to enable the menu in Kid mode.
    auto enableMenuKidMode = std::make_shared<SwitchComponent>();
    enableMenuKidMode->setState(Settings::getInstance()->getBool("EnableMenuKidMode"));
    s->addWithLabel("ENABLE MENU IN KID MODE", enableMenuKidMode);
    s->addSaveFunc([enableMenuKidMode, s] {
        if (Settings::getInstance()->getBool("EnableMenuKidMode") !=
            enableMenuKidMode->getState()) {
            Settings::getInstance()->setBool("EnableMenuKidMode", enableMenuKidMode->getState());
            s->setNeedsSaving();
        }
    });

// macOS requires root privileges to reboot and power off so it doesn't make much
// sense to enable this setting and menu entry for that operating system.
#if !defined(__APPLE__)
    // Whether to show the quit menu with the options to reboot and shutdown the computer.
    auto showQuitMenu = std::make_shared<SwitchComponent>();
    showQuitMenu->setState(Settings::getInstance()->getBool("ShowQuitMenu"));
    s->addWithLabel("SHOW QUIT MENU (REBOOT AND POWER OFF ENTRIES)", showQuitMenu);
    s->addSaveFunc([this, showQuitMenu, s] {
        if (showQuitMenu->getState() != Settings::getInstance()->getBool("ShowQuitMenu")) {
            Settings::getInstance()->setBool("ShowQuitMenu", showQuitMenu->getState());
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
                close(true);
                Utils::Platform::quitES();
            },
            "NO", nullptr));
    }
    else {
        auto s = new GuiSettings("QUIT");

        Window* window {mWindow};
        HelpStyle style {getHelpStyle()};

        ComponentListRow row;

        row.makeAcceptInputHandler([window, this] {
            window->pushGui(new GuiMsgBox(
                this->getHelpStyle(), "REALLY QUIT?", "YES",
                [this] {
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

#if defined(MENU_BUILD_DATE)
    mVersion.setText("EMULATIONSTATION-DE  V" + Utils::String::toUpper(PROGRAM_VERSION_STRING) +
                     " (Built " + __DATE__ + ")");
#else
    mVersion.setText("EMULATIONSTATION-DE  V" + Utils::String::toUpper(PROGRAM_VERSION_STRING));
#endif

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

    const bool isStart {config->isMappedTo("start", input)};
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

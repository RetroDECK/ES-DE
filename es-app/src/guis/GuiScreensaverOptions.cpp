//  SPDX-License-Identifier: MIT
//
//  ES-DE
//  GuiScreensaverOptions.cpp
//
//  User interface for the screensaver options.
//  Submenu to the GuiMenu main menu.
//

#include "guis/GuiScreensaverOptions.h"

#include "Settings.h"
#include "Window.h"
#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"

GuiScreensaverOptions::GuiScreensaverOptions(const std::string& title)
    : GuiSettings {title}
{
    // Screensaver timer.
    auto screensaverTimer = std::make_shared<SliderComponent>(0.0f, 30.0f, 1.0f, "m");
    screensaverTimer->setValue(
        static_cast<float>(Settings::getInstance()->getInt("ScreensaverTimer") / (1000 * 60)));
    addWithLabel("START SCREENSAVER AFTER (MINUTES)", screensaverTimer);
    addSaveFunc([screensaverTimer, this] {
        if (static_cast<int>(std::round(screensaverTimer->getValue()) * (1000 * 60)) !=
            Settings::getInstance()->getInt("ScreensaverTimer")) {
            Settings::getInstance()->setInt(
                "ScreensaverTimer",
                static_cast<int>(std::round(screensaverTimer->getValue()) * (1000 * 60)));
            setNeedsSaving();
        }
    });

    // Screensaver type.
    auto screensaverType = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), "SCREENSAVER TYPE", false);
    std::string selectedScreensaver {Settings::getInstance()->getString("ScreensaverType")};
    screensaverType->add("DIM", "dim", selectedScreensaver == "dim");
    screensaverType->add("BLACK", "black", selectedScreensaver == "black");
    screensaverType->add("SLIDESHOW", "slideshow", selectedScreensaver == "slideshow");
    screensaverType->add("VIDEO", "video", selectedScreensaver == "video");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set the screensaver type to "dim" in this case.
    if (screensaverType->getSelectedObjects().size() == 0)
        screensaverType->selectEntry(0);
    addWithLabel("SCREENSAVER TYPE", screensaverType);
    addSaveFunc([screensaverType, this] {
        if (screensaverType->getSelected() !=
            Settings::getInstance()->getString("ScreensaverType")) {
            Settings::getInstance()->setString("ScreensaverType", screensaverType->getSelected());
            setNeedsSaving();
        }
    });

    // Whether to enable screensaver controls.
    auto screensaverControls = std::make_shared<SwitchComponent>();
    screensaverControls->setState(Settings::getInstance()->getBool("ScreensaverControls"));
    addWithLabel("ENABLE SCREENSAVER CONTROLS", screensaverControls);
    addSaveFunc([screensaverControls, this] {
        if (screensaverControls->getState() !=
            Settings::getInstance()->getBool("ScreensaverControls")) {
            Settings::getInstance()->setBool("ScreensaverControls",
                                             screensaverControls->getState());
            setNeedsSaving();
        }
    });

    // Show filtered menu.
    ComponentListRow row;
    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>("SLIDESHOW SCREENSAVER SETTINGS",
                                                   Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary),
                   true);
    row.addElement(getMenu().makeArrow(), false);
    row.makeAcceptInputHandler(
        std::bind(&GuiScreensaverOptions::openSlideshowScreensaverOptions, this));
    addRow(row);

    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>("VIDEO SCREENSAVER SETTINGS",
                                                   Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary),
                   true);
    row.addElement(getMenu().makeArrow(), false);
    row.makeAcceptInputHandler(
        std::bind(&GuiScreensaverOptions::openVideoScreensaverOptions, this));
    addRow(row);

    setSize(getMenuSize());
}

void GuiScreensaverOptions::openSlideshowScreensaverOptions()
{
    auto s = new GuiSettings("SLIDESHOW SCREENSAVER");

    // Timer for swapping images (in seconds).
    auto screensaverSwapImageTimeout = std::make_shared<SliderComponent>(2.0f, 120.0f, 2.0f, "s");
    screensaverSwapImageTimeout->setValue(static_cast<float>(
        Settings::getInstance()->getInt("ScreensaverSwapImageTimeout") / (1000)));
    s->addWithLabel("SWAP IMAGES AFTER (SECONDS)", screensaverSwapImageTimeout);
    s->addSaveFunc([screensaverSwapImageTimeout, s] {
        if (screensaverSwapImageTimeout->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScreensaverSwapImageTimeout") /
                               (1000))) {
            Settings::getInstance()->setInt(
                "ScreensaverSwapImageTimeout",
                static_cast<int>(std::round(screensaverSwapImageTimeout->getValue()) * (1000)));
            s->setNeedsSaving();
        }
    });

    // Only include favorite games.
    auto screensaverSlideshowOnlyFavorites = std::make_shared<SwitchComponent>();
    screensaverSlideshowOnlyFavorites->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowOnlyFavorites"));
    s->addWithLabel("ONLY INCLUDE FAVORITE GAMES", screensaverSlideshowOnlyFavorites);
    s->addSaveFunc([screensaverSlideshowOnlyFavorites, s] {
        if (screensaverSlideshowOnlyFavorites->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowOnlyFavorites")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowOnlyFavorites",
                                             screensaverSlideshowOnlyFavorites->getState());
            s->setNeedsSaving();
        }
    });

    // Stretch images to screen resolution.
    auto screensaverStretchImages = std::make_shared<SwitchComponent>();
    screensaverStretchImages->setState(
        Settings::getInstance()->getBool("ScreensaverStretchImages"));
    s->addWithLabel("STRETCH IMAGES TO SCREEN RESOLUTION", screensaverStretchImages);
    s->addSaveFunc([screensaverStretchImages, s] {
        if (screensaverStretchImages->getState() !=
            Settings::getInstance()->getBool("ScreensaverStretchImages")) {
            Settings::getInstance()->setBool("ScreensaverStretchImages",
                                             screensaverStretchImages->getState());
            s->setNeedsSaving();
        }
    });

    // Show game info overlay for slideshow screensaver.
    auto screensaverSlideshowGameInfo = std::make_shared<SwitchComponent>();
    screensaverSlideshowGameInfo->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo"));
    s->addWithLabel("DISPLAY GAME INFO OVERLAY", screensaverSlideshowGameInfo);
    s->addSaveFunc([screensaverSlideshowGameInfo, s] {
        if (screensaverSlideshowGameInfo->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowGameInfo",
                                             screensaverSlideshowGameInfo->getState());
            s->setNeedsSaving();
        }
    });

    // Render scanlines using a shader.
    auto screensaverSlideshowScanlines = std::make_shared<SwitchComponent>();
    screensaverSlideshowScanlines->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowScanlines"));
    s->addWithLabel("RENDER SCANLINES", screensaverSlideshowScanlines);
    s->addSaveFunc([screensaverSlideshowScanlines, s] {
        if (screensaverSlideshowScanlines->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowScanlines")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowScanlines",
                                             screensaverSlideshowScanlines->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to use custom images.
    auto screensaverSlideshowCustomImages = std::make_shared<SwitchComponent>();
    screensaverSlideshowCustomImages->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages"));
    s->addWithLabel("USE CUSTOM IMAGES", screensaverSlideshowCustomImages);
    s->addSaveFunc([screensaverSlideshowCustomImages, s] {
        if (screensaverSlideshowCustomImages->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowCustomImages",
                                             screensaverSlideshowCustomImages->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to recurse the custom image directory.
    auto screensaverSlideshowRecurse = std::make_shared<SwitchComponent>();
    screensaverSlideshowRecurse->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowRecurse"));
    s->addWithLabel("CUSTOM IMAGE DIRECTORY RECURSIVE SEARCH", screensaverSlideshowRecurse);
    s->addSaveFunc([screensaverSlideshowRecurse, s] {
        if (screensaverSlideshowRecurse->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowRecurse")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowRecurse",
                                             screensaverSlideshowRecurse->getState());
            s->setNeedsSaving();
        }
    });

    // Custom image directory.
    ComponentListRow rowCustomImageDir;
    auto screensaverSlideshowImageDir = std::make_shared<TextComponent>(
        "CUSTOM IMAGE DIRECTORY", Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);
    auto bracketCustomImageDir = std::make_shared<ImageComponent>();
    bracketCustomImageDir->setResize(
        glm::vec2 {0.0f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()});
    bracketCustomImageDir->setImage(":/graphics/arrow.svg");
    bracketCustomImageDir->setColorShift(mMenuColorPrimary);
    rowCustomImageDir.addElement(screensaverSlideshowImageDir, true);
    rowCustomImageDir.addElement(bracketCustomImageDir, false);
    const std::string titleCustomImageDir {"CUSTOM IMAGE DIRECTORY"};
    const std::string defaultImageDirStaticText {"Default directory:"};
    const std::string defaultImageDirText {Utils::FileSystem::getAppDataDirectory()
                                               .append("screensavers")
                                               .append("custom_slideshow")
                                               .string()};
    const std::string initValueMediaDir {
        Settings::getInstance()->getString("ScreensaverSlideshowImageDir")};
    const bool multiLineMediaDir {false};
    auto updateValMediaDir = [this, s](const std::string& newVal) {
        Settings::getInstance()->setString("ScreensaverSlideshowImageDir", newVal);
        s->setNeedsSaving();
    };
    rowCustomImageDir.makeAcceptInputHandler(
        [this, s, titleCustomImageDir, defaultImageDirStaticText, defaultImageDirText,
         initValueMediaDir, updateValMediaDir, multiLineMediaDir] {
            if (Settings::getInstance()->getBool("VirtualKeyboard")) {
                mWindow->pushGui(new GuiTextEditKeyboardPopup(
                    getHelpStyle(), s->getMenu().getPosition().y, titleCustomImageDir,
                    Settings::getInstance()->getString("ScreensaverSlideshowImageDir"),
                    updateValMediaDir, multiLineMediaDir, "SAVE", "SAVE CHANGES?",
                    defaultImageDirStaticText, defaultImageDirText, "load default directory"));
            }
            else {
                mWindow->pushGui(new GuiTextEditPopup(
                    getHelpStyle(), titleCustomImageDir,
                    Settings::getInstance()->getString("ScreensaverSlideshowImageDir"),
                    updateValMediaDir, multiLineMediaDir, "SAVE", "SAVE CHANGES?",
                    defaultImageDirStaticText, defaultImageDirText, "load default directory"));
            }
        });
    s->addRow(rowCustomImageDir);

    s->setSize(mSize);
    mWindow->pushGui(s);
}

void GuiScreensaverOptions::openVideoScreensaverOptions()
{
    auto s = new GuiSettings("VIDEO SCREENSAVER");

    // Timer for swapping videos (in seconds).
    auto screensaverSwapVideoTimeout = std::make_shared<SliderComponent>(0.0f, 120.0f, 2.0f, "s");
    screensaverSwapVideoTimeout->setValue(static_cast<float>(
        Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout") / (1000)));
    s->addWithLabel("SWAP VIDEOS AFTER (SECONDS)", screensaverSwapVideoTimeout);
    s->addSaveFunc([screensaverSwapVideoTimeout, s] {
        if (screensaverSwapVideoTimeout->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout") /
                               (1000))) {
            Settings::getInstance()->setInt(
                "ScreensaverSwapVideoTimeout",
                static_cast<int>(std::round(screensaverSwapVideoTimeout->getValue()) * (1000)));
            s->setNeedsSaving();
        }
    });

    // Only include favorite games.
    auto screensaverVideoOnlyFavorites = std::make_shared<SwitchComponent>();
    screensaverVideoOnlyFavorites->setState(
        Settings::getInstance()->getBool("ScreensaverVideoOnlyFavorites"));
    s->addWithLabel("ONLY INCLUDE FAVORITE GAMES", screensaverVideoOnlyFavorites);
    s->addSaveFunc([screensaverVideoOnlyFavorites, s] {
        if (screensaverVideoOnlyFavorites->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoOnlyFavorites")) {
            Settings::getInstance()->setBool("ScreensaverVideoOnlyFavorites",
                                             screensaverVideoOnlyFavorites->getState());
            s->setNeedsSaving();
        }
    });

    // Stretch videos to screen resolution.
    auto screensaverStretchVideos = std::make_shared<SwitchComponent>();
    screensaverStretchVideos->setState(
        Settings::getInstance()->getBool("ScreensaverStretchVideos"));
    s->addWithLabel("STRETCH VIDEOS TO SCREEN RESOLUTION", screensaverStretchVideos);
    s->addSaveFunc([screensaverStretchVideos, s] {
        if (screensaverStretchVideos->getState() !=
            Settings::getInstance()->getBool("ScreensaverStretchVideos")) {
            Settings::getInstance()->setBool("ScreensaverStretchVideos",
                                             screensaverStretchVideos->getState());
            s->setNeedsSaving();
        }
    });

    // Show game info overlay for video screensaver.
    auto screensaverVideoGameInfo = std::make_shared<SwitchComponent>();
    screensaverVideoGameInfo->setState(
        Settings::getInstance()->getBool("ScreensaverVideoGameInfo"));
    s->addWithLabel("DISPLAY GAME INFO OVERLAY", screensaverVideoGameInfo);
    s->addSaveFunc([screensaverVideoGameInfo, s] {
        if (screensaverVideoGameInfo->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoGameInfo")) {
            Settings::getInstance()->setBool("ScreensaverVideoGameInfo",
                                             screensaverVideoGameInfo->getState());
            s->setNeedsSaving();
        }
    });

    // Render scanlines using a shader.
    auto screensaverVideoScanlines = std::make_shared<SwitchComponent>();
    screensaverVideoScanlines->setState(
        Settings::getInstance()->getBool("ScreensaverVideoScanlines"));
    s->addWithLabel("RENDER SCANLINES", screensaverVideoScanlines);
    s->addSaveFunc([screensaverVideoScanlines, s] {
        if (screensaverVideoScanlines->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoScanlines")) {
            Settings::getInstance()->setBool("ScreensaverVideoScanlines",
                                             screensaverVideoScanlines->getState());
            s->setNeedsSaving();
        }
    });

    // Render blur using a shader.
    auto screensaverVideoBlur = std::make_shared<SwitchComponent>();
    screensaverVideoBlur->setState(Settings::getInstance()->getBool("ScreensaverVideoBlur"));
    s->addWithLabel("RENDER BLUR", screensaverVideoBlur);
    s->addSaveFunc([screensaverVideoBlur, s] {
        if (screensaverVideoBlur->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoBlur")) {
            Settings::getInstance()->setBool("ScreensaverVideoBlur",
                                             screensaverVideoBlur->getState());
            s->setNeedsSaving();
        }
    });

    s->setSize(mSize);
    mWindow->pushGui(s);
}

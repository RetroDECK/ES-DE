//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
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
    std::vector<std::string> screensavers;
    screensavers.push_back("dim");
    screensavers.push_back("black");
    screensavers.push_back("slideshow");
    screensavers.push_back("video");
    for (auto it = screensavers.cbegin(); it != screensavers.cend(); ++it)
        screensaverType->add(*it, *it,
                             Settings::getInstance()->getString("ScreensaverType") == *it);
    addWithLabel("SCREENSAVER TYPE", screensaverType);
    addSaveFunc([screensaverType, this] {
        if (screensaverType->getSelected() !=
            Settings::getInstance()->getString("ScreensaverType")) {
            if (screensaverType->getSelected() == "video") {
                // If before it wasn't risky but now there's a risk of problems, show warning.
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    "THE 'VIDEO' SCREENSAVER SHOWS\n"
                    "VIDEOS FROM YOUR GAMELISTS\n\n"
                    "IF YOU DO NOT HAVE ANY VIDEOS, THE\n"
                    "SCREENSAVER WILL DEFAULT TO 'DIM'",
                    "OK", [] { return; }, "", nullptr, "", nullptr));
            }
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
                                                   Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                   true);
    row.addElement(makeArrow(), false);
    row.makeAcceptInputHandler(
        std::bind(&GuiScreensaverOptions::openSlideshowScreensaverOptions, this));
    addRow(row);

    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>("VIDEO SCREENSAVER SETTINGS",
                                                   Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                   true);
    row.addElement(makeArrow(), false);
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
    auto screensaverSlideshowImageDir =
        std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_RIGHT);
    s->addEditableTextComponent(
        "CUSTOM IMAGE DIRECTORY", screensaverSlideshowImageDir,
        Settings::getInstance()->getString("ScreensaverSlideshowImageDir"),
        Settings::getInstance()->getDefaultString("ScreensaverSlideshowImageDir"));
    s->addSaveFunc([screensaverSlideshowImageDir, s] {
        if (screensaverSlideshowImageDir->getValue() !=
            Settings::getInstance()->getString("ScreensaverSlideshowImageDir")) {
            Settings::getInstance()->setString("ScreensaverSlideshowImageDir",
                                               screensaverSlideshowImageDir->getValue());
            s->setNeedsSaving();
        }
    });

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

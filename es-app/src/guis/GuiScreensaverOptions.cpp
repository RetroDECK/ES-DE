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
#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"

GuiScreensaverOptions::GuiScreensaverOptions(Window* window, const std::string& title)
    : GuiSettings(window, title)
{
    // Screensaver timer.
    auto screensaver_timer = std::make_shared<SliderComponent>(mWindow, 0.0f, 30.0f, 1.0f, "m");
    screensaver_timer->setValue(
        static_cast<float>(Settings::getInstance()->getInt("ScreensaverTimer") / (1000 * 60)));
    addWithLabel("START SCREENSAVER AFTER (MINUTES)", screensaver_timer);
    addSaveFunc([screensaver_timer, this] {
        if (static_cast<int>(std::round(screensaver_timer->getValue()) * (1000 * 60)) !=
            Settings::getInstance()->getInt("ScreensaverTimer")) {
            Settings::getInstance()->setInt(
                "ScreensaverTimer",
                static_cast<int>(std::round(screensaver_timer->getValue()) * (1000 * 60)));
            setNeedsSaving();
        }
    });

    // Screensaver type.
    auto screensaver_type = std::make_shared<OptionListComponent<std::string>>(
        mWindow, getHelpStyle(), "SCREENSAVER TYPE", false);
    std::vector<std::string> screensavers;
    screensavers.push_back("dim");
    screensavers.push_back("black");
    screensavers.push_back("slideshow");
    screensavers.push_back("video");
    for (auto it = screensavers.cbegin(); it != screensavers.cend(); it++)
        screensaver_type->add(*it, *it,
                              Settings::getInstance()->getString("ScreensaverType") == *it);
    addWithLabel("SCREENSAVER TYPE", screensaver_type);
    addSaveFunc([screensaver_type, this] {
        if (screensaver_type->getSelected() !=
            Settings::getInstance()->getString("ScreensaverType")) {
            if (screensaver_type->getSelected() == "video") {
                // If before it wasn't risky but now there's a risk of problems, show warning.
                mWindow->pushGui(new GuiMsgBox(
                    mWindow, getHelpStyle(),
                    "THE 'VIDEO' SCREENSAVER SHOWS\n"
                    "VIDEOS FROM YOUR GAMELISTS\n\n"
                    "IF YOU DO NOT HAVE ANY VIDEOS, THE\n"
                    "SCREENSAVER WILL DEFAULT TO 'DIM'",
                    "OK", [] { return; }, "", nullptr, "", nullptr));
            }
            Settings::getInstance()->setString("ScreensaverType", screensaver_type->getSelected());
            setNeedsSaving();
        }
    });

    // Whether to enable screensaver controls.
    auto screensaver_controls = std::make_shared<SwitchComponent>(mWindow);
    screensaver_controls->setState(Settings::getInstance()->getBool("ScreensaverControls"));
    addWithLabel("ENABLE SCREENSAVER CONTROLS", screensaver_controls);
    addSaveFunc([screensaver_controls, this] {
        if (screensaver_controls->getState() !=
            Settings::getInstance()->getBool("ScreensaverControls")) {
            Settings::getInstance()->setBool("ScreensaverControls",
                                             screensaver_controls->getState());
            setNeedsSaving();
        }
    });

    // Show filtered menu.
    ComponentListRow row;
    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow, "SLIDESHOW SCREENSAVER SETTINGS",
                                                   Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                   true);
    row.addElement(makeArrow(mWindow), false);
    row.makeAcceptInputHandler(
        std::bind(&GuiScreensaverOptions::openSlideshowScreensaverOptions, this));
    addRow(row);

    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow, "VIDEO SCREENSAVER SETTINGS",
                                                   Font::get(FONT_SIZE_MEDIUM), 0x777777FF),
                   true);
    row.addElement(makeArrow(mWindow), false);
    row.makeAcceptInputHandler(
        std::bind(&GuiScreensaverOptions::openVideoScreensaverOptions, this));
    addRow(row);
}

void GuiScreensaverOptions::openSlideshowScreensaverOptions()
{
    auto s = new GuiSettings(mWindow, "SLIDESHOW SCREENSAVER");

    // Timer for swapping images (in seconds).
    auto screensaver_swap_image_timeout =
        std::make_shared<SliderComponent>(mWindow, 2.0f, 120.0f, 2.0f, "s");
    screensaver_swap_image_timeout->setValue(static_cast<float>(
        Settings::getInstance()->getInt("ScreensaverSwapImageTimeout") / (1000)));
    s->addWithLabel("SWAP IMAGES AFTER (SECONDS)", screensaver_swap_image_timeout);
    s->addSaveFunc([screensaver_swap_image_timeout, s] {
        if (screensaver_swap_image_timeout->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScreensaverSwapImageTimeout") /
                               (1000))) {
            Settings::getInstance()->setInt(
                "ScreensaverSwapImageTimeout",
                static_cast<int>(std::round(screensaver_swap_image_timeout->getValue()) * (1000)));
            s->setNeedsSaving();
        }
    });

    // Stretch images to screen resolution.
    auto screensaver_stretch_images = std::make_shared<SwitchComponent>(mWindow);
    screensaver_stretch_images->setState(
        Settings::getInstance()->getBool("ScreensaverStretchImages"));
    s->addWithLabel("STRETCH IMAGES TO SCREEN RESOLUTION", screensaver_stretch_images);
    s->addSaveFunc([screensaver_stretch_images, s] {
        if (screensaver_stretch_images->getState() !=
            Settings::getInstance()->getBool("ScreensaverStretchImages")) {
            Settings::getInstance()->setBool("ScreensaverStretchImages",
                                             screensaver_stretch_images->getState());
            s->setNeedsSaving();
        }
    });

    // Show game info overlay for slideshow screensaver.
    auto screensaver_slideshow_game_info = std::make_shared<SwitchComponent>(mWindow);
    screensaver_slideshow_game_info->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo"));
    s->addWithLabel("DISPLAY GAME INFO OVERLAY", screensaver_slideshow_game_info);
    s->addSaveFunc([screensaver_slideshow_game_info, s] {
        if (screensaver_slideshow_game_info->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowGameInfo",
                                             screensaver_slideshow_game_info->getState());
            s->setNeedsSaving();
        }
    });

#if defined(USE_OPENGL_21)
    // Render scanlines using a shader.
    auto screensaver_slideshow_scanlines = std::make_shared<SwitchComponent>(mWindow);
    screensaver_slideshow_scanlines->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowScanlines"));
    s->addWithLabel("RENDER SCANLINES", screensaver_slideshow_scanlines);
    s->addSaveFunc([screensaver_slideshow_scanlines, s] {
        if (screensaver_slideshow_scanlines->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowScanlines")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowScanlines",
                                             screensaver_slideshow_scanlines->getState());
            s->setNeedsSaving();
        }
    });
#endif

    // Whether to use custom images.
    auto screensaver_slideshow_custom_images = std::make_shared<SwitchComponent>(mWindow);
    screensaver_slideshow_custom_images->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages"));
    s->addWithLabel("USE CUSTOM IMAGES", screensaver_slideshow_custom_images);
    s->addSaveFunc([screensaver_slideshow_custom_images, s] {
        if (screensaver_slideshow_custom_images->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowCustomImages",
                                             screensaver_slideshow_custom_images->getState());
            s->setNeedsSaving();
        }
    });

    // Whether to recurse the custom image directory.
    auto screensaver_slideshow_recurse = std::make_shared<SwitchComponent>(mWindow);
    screensaver_slideshow_recurse->setState(
        Settings::getInstance()->getBool("ScreensaverSlideshowRecurse"));
    s->addWithLabel("CUSTOM IMAGE DIRECTORY RECURSIVE SEARCH", screensaver_slideshow_recurse);
    s->addSaveFunc([screensaver_slideshow_recurse, s] {
        if (screensaver_slideshow_recurse->getState() !=
            Settings::getInstance()->getBool("ScreensaverSlideshowRecurse")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowRecurse",
                                             screensaver_slideshow_recurse->getState());
            s->setNeedsSaving();
        }
    });

    // Custom image directory.
    auto screensaver_slideshow_image_dir = std::make_shared<TextComponent>(
        mWindow, "", Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_RIGHT);
    s->addEditableTextComponent(
        "CUSTOM IMAGE DIRECTORY", screensaver_slideshow_image_dir,
        Settings::getInstance()->getString("ScreensaverSlideshowImageDir"),
        Settings::getInstance()->getDefaultString("ScreensaverSlideshowImageDir"));
    s->addSaveFunc([screensaver_slideshow_image_dir, s] {
        if (screensaver_slideshow_image_dir->getValue() !=
            Settings::getInstance()->getString("ScreensaverSlideshowImageDir")) {
            Settings::getInstance()->setString("ScreensaverSlideshowImageDir",
                                               screensaver_slideshow_image_dir->getValue());
            s->setNeedsSaving();
        }
    });

    mWindow->pushGui(s);
}

void GuiScreensaverOptions::openVideoScreensaverOptions()
{
    auto s = new GuiSettings(mWindow, "VIDEO SCREENSAVER");

    // Timer for swapping videos (in seconds).
    auto screensaver_swap_video_timeout =
        std::make_shared<SliderComponent>(mWindow, 0.0f, 120.0f, 2.0f, "s");
    screensaver_swap_video_timeout->setValue(static_cast<float>(
        Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout") / (1000)));
    s->addWithLabel("SWAP VIDEOS AFTER (SECONDS)", screensaver_swap_video_timeout);
    s->addSaveFunc([screensaver_swap_video_timeout, s] {
        if (screensaver_swap_video_timeout->getValue() !=
            static_cast<float>(Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout") /
                               (1000))) {
            Settings::getInstance()->setInt(
                "ScreensaverSwapVideoTimeout",
                static_cast<int>(std::round(screensaver_swap_video_timeout->getValue()) * (1000)));
            s->setNeedsSaving();
        }
    });

    // Stretch videos to screen resolution.
    auto screensaver_stretch_videos = std::make_shared<SwitchComponent>(mWindow);
    screensaver_stretch_videos->setState(
        Settings::getInstance()->getBool("ScreensaverStretchVideos"));
    s->addWithLabel("STRETCH VIDEOS TO SCREEN RESOLUTION", screensaver_stretch_videos);
    s->addSaveFunc([screensaver_stretch_videos, s] {
        if (screensaver_stretch_videos->getState() !=
            Settings::getInstance()->getBool("ScreensaverStretchVideos")) {
            Settings::getInstance()->setBool("ScreensaverStretchVideos",
                                             screensaver_stretch_videos->getState());
            s->setNeedsSaving();
        }
    });

    // Show game info overlay for video screensaver.
    auto screensaver_video_game_info = std::make_shared<SwitchComponent>(mWindow);
    screensaver_video_game_info->setState(
        Settings::getInstance()->getBool("ScreensaverVideoGameInfo"));
    s->addWithLabel("DISPLAY GAME INFO OVERLAY", screensaver_video_game_info);
    s->addSaveFunc([screensaver_video_game_info, s] {
        if (screensaver_video_game_info->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoGameInfo")) {
            Settings::getInstance()->setBool("ScreensaverVideoGameInfo",
                                             screensaver_video_game_info->getState());
            s->setNeedsSaving();
        }
    });

#if defined(USE_OPENGL_21)
    // Render scanlines using a shader.
    auto screensaver_video_scanlines = std::make_shared<SwitchComponent>(mWindow);
    screensaver_video_scanlines->setState(
        Settings::getInstance()->getBool("ScreensaverVideoScanlines"));
    s->addWithLabel("RENDER SCANLINES", screensaver_video_scanlines);
    s->addSaveFunc([screensaver_video_scanlines, s] {
        if (screensaver_video_scanlines->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoScanlines")) {
            Settings::getInstance()->setBool("ScreensaverVideoScanlines",
                                             screensaver_video_scanlines->getState());
            s->setNeedsSaving();
        }
    });

    // Render blur using a shader.
    auto screensaver_video_blur = std::make_shared<SwitchComponent>(mWindow);
    screensaver_video_blur->setState(Settings::getInstance()->getBool("ScreensaverVideoBlur"));
    s->addWithLabel("RENDER BLUR", screensaver_video_blur);
    s->addSaveFunc([screensaver_video_blur, s] {
        if (screensaver_video_blur->getState() !=
            Settings::getInstance()->getBool("ScreensaverVideoBlur")) {
            Settings::getInstance()->setBool("ScreensaverVideoBlur",
                                             screensaver_video_blur->getState());
            s->setNeedsSaving();
        }
    });
#endif

    mWindow->pushGui(s);
}

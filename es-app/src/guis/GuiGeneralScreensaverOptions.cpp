//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiGeneralScreensaverOptions.cpp
//
//  User interface for the screensaver options.
//  Submenu to the GuiMenu main menu.
//

#include "guis/GuiGeneralScreensaverOptions.h"

#include "components/OptionListComponent.h"
#include "components/SliderComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "Settings.h"

GuiGeneralScreensaverOptions::GuiGeneralScreensaverOptions(Window* window, const char* title)
        : GuiSettings(window, title)
{
    // Screensaver timer.
    auto screensaver_time = std::make_shared<SliderComponent>(mWindow, 0.f, 30.f, 1.f, "m");
    screensaver_time->setValue(static_cast<float>(Settings::getInstance()->
            getInt("ScreensaverTimer") / (1000 * 60)));
    addWithLabel("SCREENSAVER AFTER", screensaver_time);
    addSaveFunc([screensaver_time, this] {
        if (static_cast<int>(Math::round(screensaver_time->getValue()) * (1000 * 60)) !=
                Settings::getInstance()->getInt("ScreensaverTimer")) {
            Settings::getInstance()->setInt("ScreensaverTimer",
                    static_cast<int>(Math::round(screensaver_time->getValue()) * (1000 * 60)));
            PowerSaver::updateTimeouts();
            setNeedsSaving();
        }
    });

    // Whether to enable screensaver controls.
    auto screensaver_controls = std::make_shared<SwitchComponent>(mWindow);
    screensaver_controls->setState(Settings::getInstance()->getBool("ScreensaverControls"));
    addWithLabel("SCREENSAVER CONTROLS", screensaver_controls);
    addSaveFunc([screensaver_controls, this] {
        if (screensaver_controls->getState() !=
                Settings::getInstance()->getBool("ScreensaverControls")) {
            Settings::getInstance()->setBool("ScreensaverControls",
                    screensaver_controls->getState());
            setNeedsSaving();
        }
    });

    // Screensaver behavior.
    auto screensaver_behavior = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "SCREENSAVER BEHAVIOR", false);
    std::vector<std::string> screensavers;
    screensavers.push_back("dim");
    screensavers.push_back("black");
    screensavers.push_back("slideshow");
    screensavers.push_back("video");
    for (auto it = screensavers.cbegin(); it != screensavers.cend(); it++)
        screensaver_behavior->add(*it, *it, Settings::getInstance()->
                getString("ScreensaverBehavior") == *it);
    addWithLabel("SCREENSAVER BEHAVIOR", screensaver_behavior);
    addSaveFunc([screensaver_behavior, this] {
        if (screensaver_behavior->getSelected() !=
                Settings::getInstance()->getString("ScreensaverBehavior")) {
            if (screensaver_behavior->getSelected() == "video") {
                // If before it wasn't risky but now there's a risk of problems, show warning.
                mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                        "THE \"VIDEO\" SCREENSAVER SHOWS\nVIDEOS FROM YOUR GAMELISTS.\n\n"
                        "IF YOU DO NOT HAVE ANY VIDEOS, THE\n"
                        "SCREENSAVER WILL DEFAULT TO \"BLACK\"",
                        "OK", [] { return; }, "", nullptr, "", nullptr));
                }
            Settings::getInstance()->setString("ScreensaverBehavior",
                    screensaver_behavior->getSelected());
            setNeedsSaving();
            PowerSaver::updateTimeouts();
        }
    });

    ComponentListRow row;

    // Show filtered menu.
    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow,
            "SLIDESHOW SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
    row.addElement(makeArrow(mWindow), false);
    row.makeAcceptInputHandler(std::bind(
            &GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions, this));
    addRow(row);

    row.elements.clear();
    row.addElement(std::make_shared<TextComponent>(mWindow,
            "VIDEO SCREENSAVER SETTINGS", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
    row.addElement(makeArrow(mWindow), false);
    row.makeAcceptInputHandler(std::bind(
                &GuiGeneralScreensaverOptions::openVideoScreensaverOptions, this));
    addRow(row);
}

void GuiGeneralScreensaverOptions::openSlideshowScreensaverOptions()
{
    auto s = new GuiSettings(mWindow, "SLIDESHOW SCREENSAVER");

    // Timer for swapping images (in seconds).
    auto screensaver_swap_image_timeout =
            std::make_shared<SliderComponent>(mWindow, 5.f, 120.f, 1.f, "s");
    screensaver_swap_image_timeout->setValue(static_cast<float>(Settings::getInstance()->
            getInt("ScreensaverSwapImageTimeout") / (1000)));
    s->addWithLabel("SWAP IMAGE AFTER (SECS)", screensaver_swap_image_timeout);
    s->addSaveFunc([screensaver_swap_image_timeout, s] {
        if (screensaver_swap_image_timeout->getValue() !=
                static_cast<float>(Settings::getInstance()->
                getInt("ScreensaverSwapImageTimeout") / (1000))) {
            Settings::getInstance()->setInt("ScreensaverSwapImageTimeout",
                    static_cast<int>(Math::round(screensaver_swap_image_timeout->getValue()) *
                    (1000)));
            s->setNeedsSaving();
            PowerSaver::updateTimeouts();
        }
    });

    // Stretch images to screen resolution.
    auto screensaver_stretch_images = std::make_shared<SwitchComponent>(mWindow);
    screensaver_stretch_images->
            setState(Settings::getInstance()->getBool("ScreensaverStretchImages"));
    s->addWithLabel("STRETCH IMAGES TO SCREEN RESOLUTION", screensaver_stretch_images);
    s->addSaveFunc([screensaver_stretch_images, s] {
        if (screensaver_stretch_images->getState() !=
                Settings::getInstance()->getBool("ScreensaverStretchImages")) {
            Settings::getInstance()->setBool("ScreensaverStretchImages",
                    screensaver_stretch_images->getState());
            s->setNeedsSaving();
        }
    });

    #if defined(USE_OPENGL_21)
    // Render scanlines using a shader.
    auto screensaver_image_scanlines = std::make_shared<SwitchComponent>(mWindow);
    screensaver_image_scanlines->
            setState(Settings::getInstance()->getBool("ScreensaverImageScanlines"));
    s->addWithLabel("RENDER SCANLINES", screensaver_image_scanlines);
    s->addSaveFunc([screensaver_image_scanlines, s] {
        if (screensaver_image_scanlines->getState() !=
                Settings::getInstance()->getBool("ScreensaverImageScanlines")) {
            Settings::getInstance()->
                    setBool("ScreensaverImageScanlines", screensaver_image_scanlines->getState());
            s->setNeedsSaving();
        }
    });
    #endif

    // Background audio file.
    auto screensaver_slideshow_audio_file = std::make_shared<TextComponent>(mWindow, "",
            Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_RIGHT);
    s->addEditableTextComponent("BACKGROUND AUDIO", screensaver_slideshow_audio_file,
            Settings::getInstance()->getString("ScreensaverSlideshowAudioFile"),
            "~/.emulationstation/slideshow/audio/slideshow.wav");
    s->addSaveFunc([screensaver_slideshow_audio_file, s] {
        if (screensaver_slideshow_audio_file->getValue() !=
                Settings::getInstance()->getString("ScreensaverSlideshowAudioFile")) {
            Settings::getInstance()->setString("ScreensaverSlideshowAudioFile",
                    screensaver_slideshow_audio_file->getValue());
            s->setNeedsSaving();
        }
    });

    // Whether to use custom images.
    auto screensaver_slideshow_custom_images = std::make_shared<SwitchComponent>(mWindow);
    screensaver_slideshow_custom_images->setState(Settings::getInstance()->
            getBool("ScreensaverSlideshowCustomImages"));
    s->addWithLabel("USE CUSTOM IMAGES", screensaver_slideshow_custom_images);
    s->addSaveFunc([screensaver_slideshow_custom_images, s] {
        if (screensaver_slideshow_custom_images->getState() !=
                Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowCustomImages",
                    screensaver_slideshow_custom_images->getState());
            s->setNeedsSaving();
        }
    });

    // Custom image directory.
    auto screensaver_slideshow_image_dir = std::make_shared<TextComponent>(mWindow, "",
            Font::get(FONT_SIZE_SMALL), 0x777777FF, ALIGN_RIGHT);
    s->addEditableTextComponent("CUSTOM IMAGE DIR", screensaver_slideshow_image_dir,
            Settings::getInstance()->getString("ScreensaverSlideshowImageDir"),
            "~/.emulationstation/slideshow/custom_images");
    s->addSaveFunc([screensaver_slideshow_image_dir, s] {
        if (screensaver_slideshow_image_dir->getValue() !=
                Settings::getInstance()->getString("ScreensaverSlideshowImageDir")) {
            Settings::getInstance()->setString("ScreensaverSlideshowImageDir",
                    screensaver_slideshow_image_dir->getValue());
            s->setNeedsSaving();
        }
    });

    // Whether to recurse the custom image directory.
    auto screensaver_slideshow_recurse = std::make_shared<SwitchComponent>(mWindow);
    screensaver_slideshow_recurse->setState(Settings::getInstance()->
            getBool("ScreensaverSlideshowRecurse"));
    s->addWithLabel("CUSTOM IMAGE DIR RECURSIVE", screensaver_slideshow_recurse);
    s->addSaveFunc([screensaver_slideshow_recurse, s] {
        if (screensaver_slideshow_recurse->getState() !=
                Settings::getInstance()->getBool("ScreensaverSlideshowRecurse")) {
            Settings::getInstance()->setBool("ScreensaverSlideshowRecurse",
                    screensaver_slideshow_recurse->getState());
            s->setNeedsSaving();
        }
    });

    mWindow->pushGui(s);
}

void GuiGeneralScreensaverOptions::openVideoScreensaverOptions()
{
    auto s = new GuiSettings(mWindow, "VIDEO SCREENSAVER");

    // Timer for swapping videos (in seconds).
    auto screensaver_swap_video_timeout =
            std::make_shared<SliderComponent>(mWindow, 5.f, 120.f, 1.f, "s");
    screensaver_swap_video_timeout->setValue(static_cast<float>(Settings::getInstance()->
            getInt("ScreensaverSwapVideoTimeout") / (1000)));
    s->addWithLabel("SWAP VIDEO AFTER (SECS)", screensaver_swap_video_timeout);
    s->addSaveFunc([screensaver_swap_video_timeout,s ] {
        if (screensaver_swap_video_timeout->getValue() !=
                static_cast<float>(Settings::getInstance()->
                getInt("ScreensaverSwapVideoTimeout") / (1000))) {
            Settings::getInstance()->setInt("ScreensaverSwapVideoTimeout",
                    static_cast<int>(Math::round(screensaver_swap_video_timeout->getValue()) *
                    (1000)));
            s->setNeedsSaving();
            PowerSaver::updateTimeouts();
        }
    });

    // Stretch videos to screen resolution.
    auto screensaver_stretch_videos = std::make_shared<SwitchComponent>(mWindow);
    screensaver_stretch_videos->
            setState(Settings::getInstance()->getBool("ScreensaverStretchVideos"));
    s->addWithLabel("STRETCH VIDEOS TO SCREEN RESOLUTION", screensaver_stretch_videos);
    s->addSaveFunc([screensaver_stretch_videos, s] {
        if (screensaver_stretch_videos->getState() !=
                Settings::getInstance()->getBool("ScreensaverStretchVideos")) {
            Settings::getInstance()->setBool("ScreensaverStretchVideos",
                    screensaver_stretch_videos->getState());
            s->setNeedsSaving();
        }
    });

    #if defined(_RPI_)
    auto screensaver_omx_player = std::make_shared<SwitchComponent>(mWindow);
    screensaver_omx_player->setState(Settings::getInstance()->getBool("ScreensaverOmxPlayer"));
    s->addWithLabel("USE OMX PLAYER FOR SCREENSAVER", screensaver_omx_player);
    s->addSaveFunc([screensaver_omx_player, s] {
        if (screensaver_omx_player->getState() !=
                Settings::getInstance()->getBool("ScreensaverOmxPlayer")) {
            Settings::getInstance()->
                    setBool("ScreensaverOmxPlayer", screensaver_omx_player->getState());
            s->setNeedsSaving();
        }
    });

    // TEMPORARY - Disabled for now, need to find a proper way to make an overlay on top of
    // the videos. The solution with rendering subtitles is not a good solution.
    // And as well the VLC video player subtitles seem to be somehow broken.
    // Render video game name as subtitles.
//    auto ss_info = std::make_shared<OptionListComponent<std::string>>
//            (mWindow,getHelpStyle(), "SHOW GAME INFO", false);
//    std::vector<std::string> info_type;
//    info_type.push_back("always");
//    info_type.push_back("start & end");
//    info_type.push_back("never");
//    for (auto it = info_type.cbegin(); it != info_type.cend(); it++)
//        ss_info->add(*it, *it, Settings::getInstance()->getString("ScreensaverGameInfo") == *it);
//    addWithLabel("SHOW GAME INFO ON SCREENSAVER", ss_info);
//    addSaveFunc([ss_info, this] { Settings::getInstance()->
//            setString("ScreensaverGameInfo", ss_info->getSelected()); });

//    ComponentListRow row;

    // Set subtitle position.
//    auto ss_omx_subs_align = std::make_shared<OptionListComponent<std::string>>
//            (mWindow, getHelpStyle(), "GAME INFO ALIGNMENT", false);
//    std::vector<std::string> align_mode;
//    align_mode.push_back("left");
//    align_mode.push_back("center");
//    for (auto it = align_mode.cbegin(); it != align_mode.cend(); it++)
//        ss_omx_subs_align->add(*it, *it, Settings::getInstance()->
//                getString("SubtitleAlignment") == *it);
//    addWithLabel("GAME INFO ALIGNMENT", ss_omx_subs_align);
//    addSaveFunc([ss_omx_subs_align, this] { Settings::getInstance()->
//            setString("SubtitleAlignment", ss_omx_subs_align->getSelected()); });

    // Set font size.
//    auto ss_omx_font_size = std::make_shared<SliderComponent>(mWindow, 1.f, 64.f, 1.f, "h");
//    ss_omx_font_size->setValue((float)(Settings::getInstance()->getInt("SubtitleSize")));
//    addWithLabel("GAME INFO FONT SIZE", ss_omx_font_size);
//    addSaveFunc([ss_omx_font_size] {
//        int subSize = (int)Math::round(ss_omx_font_size->getValue());
//        Settings::getInstance()->setInt("SubtitleSize", subSize);
//    });
    #endif

    auto screensaver_video_audio = std::make_shared<SwitchComponent>(mWindow);
    screensaver_video_audio->setState(Settings::getInstance()->getBool("ScreensaverVideoAudio"));
    s->addWithLabel("PLAY AUDIO FOR SCREENSAVER VIDEO FILES", screensaver_video_audio);
    s->addSaveFunc([screensaver_video_audio, s] {
        if (screensaver_video_audio->getState() !=
                Settings::getInstance()->getBool("ScreensaverVideoAudio")) {
            Settings::getInstance()->setBool("ScreensaverVideoAudio",
                    screensaver_video_audio->getState());
            s->setNeedsSaving();
        }
    });


    #if defined(USE_OPENGL_21)
    // Render scanlines using a shader.
    auto screensaver_video_scanlines = std::make_shared<SwitchComponent>(mWindow);
    screensaver_video_scanlines->
            setState(Settings::getInstance()->getBool("ScreensaverVideoScanlines"));
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

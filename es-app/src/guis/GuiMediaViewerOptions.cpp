//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiMediaViewerOptions.cpp
//
//  User interface for the media viewer options.
//  Submenu to the GuiMenu main menu.
//

#include "guis/GuiMediaViewerOptions.h"

#include "Settings.h"
#include "components/SwitchComponent.h"

GuiMediaViewerOptions::GuiMediaViewerOptions(const std::string& title)
    : GuiSettings {title}
{
    // Keep videos running when viewing images.
    auto keepVideoRunning = std::make_shared<SwitchComponent>();
    keepVideoRunning->setState(Settings::getInstance()->getBool("MediaViewerKeepVideoRunning"));
    addWithLabel("KEEP VIDEOS RUNNING WHEN VIEWING IMAGES", keepVideoRunning);
    addSaveFunc([keepVideoRunning, this] {
        if (keepVideoRunning->getState() !=
            Settings::getInstance()->getBool("MediaViewerKeepVideoRunning")) {
            Settings::getInstance()->setBool("MediaViewerKeepVideoRunning",
                                             keepVideoRunning->getState());
            setNeedsSaving();
        }
    });

    // Stretch videos to screen resolution.
    auto stretchVideos = std::make_shared<SwitchComponent>();
    stretchVideos->setState(Settings::getInstance()->getBool("MediaViewerStretchVideos"));
    addWithLabel("STRETCH VIDEOS TO SCREEN RESOLUTION", stretchVideos);
    addSaveFunc([stretchVideos, this] {
        if (stretchVideos->getState() !=
            Settings::getInstance()->getBool("MediaViewerStretchVideos")) {
            Settings::getInstance()->setBool("MediaViewerStretchVideos", stretchVideos->getState());
            setNeedsSaving();
        }
    });

    // Render scanlines for videos using a shader.
    auto videoScanlines = std::make_shared<SwitchComponent>();
    videoScanlines->setState(Settings::getInstance()->getBool("MediaViewerVideoScanlines"));
    addWithLabel("RENDER SCANLINES FOR VIDEOS", videoScanlines);
    addSaveFunc([videoScanlines, this] {
        if (videoScanlines->getState() !=
            Settings::getInstance()->getBool("MediaViewerVideoScanlines")) {
            Settings::getInstance()->setBool("MediaViewerVideoScanlines",
                                             videoScanlines->getState());
            setNeedsSaving();
        }
    });

    // Render blur for videos using a shader.
    auto videoBlur = std::make_shared<SwitchComponent>();
    videoBlur->setState(Settings::getInstance()->getBool("MediaViewerVideoBlur"));
    addWithLabel("RENDER BLUR FOR VIDEOS", videoBlur);
    addSaveFunc([videoBlur, this] {
        if (videoBlur->getState() != Settings::getInstance()->getBool("MediaViewerVideoBlur")) {
            Settings::getInstance()->setBool("MediaViewerVideoBlur", videoBlur->getState());
            setNeedsSaving();
        }
    });

    // Render scanlines for screenshots and title screens using a shader.
    auto screenshotScanlines = std::make_shared<SwitchComponent>();
    screenshotScanlines->setState(
        Settings::getInstance()->getBool("MediaViewerScreenshotScanlines"));
    addWithLabel("RENDER SCANLINES FOR SCREENSHOTS AND TITLES", screenshotScanlines);
    addSaveFunc([screenshotScanlines, this] {
        if (screenshotScanlines->getState() !=
            Settings::getInstance()->getBool("MediaViewerScreenshotScanlines")) {
            Settings::getInstance()->setBool("MediaViewerScreenshotScanlines",
                                             screenshotScanlines->getState());
            setNeedsSaving();
        }
    });
}

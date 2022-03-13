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
    auto keep_video_running = std::make_shared<SwitchComponent>();
    keep_video_running->setState(Settings::getInstance()->getBool("MediaViewerKeepVideoRunning"));
    addWithLabel("KEEP VIDEOS RUNNING WHEN VIEWING IMAGES", keep_video_running);
    addSaveFunc([keep_video_running, this] {
        if (keep_video_running->getState() !=
            Settings::getInstance()->getBool("MediaViewerKeepVideoRunning")) {
            Settings::getInstance()->setBool("MediaViewerKeepVideoRunning",
                                             keep_video_running->getState());
            setNeedsSaving();
        }
    });

    // Stretch videos to screen resolution.
    auto stretch_videos = std::make_shared<SwitchComponent>();
    stretch_videos->setState(Settings::getInstance()->getBool("MediaViewerStretchVideos"));
    addWithLabel("STRETCH VIDEOS TO SCREEN RESOLUTION", stretch_videos);
    addSaveFunc([stretch_videos, this] {
        if (stretch_videos->getState() !=
            Settings::getInstance()->getBool("MediaViewerStretchVideos")) {
            Settings::getInstance()->setBool("MediaViewerStretchVideos",
                                             stretch_videos->getState());
            setNeedsSaving();
        }
    });

    // Render scanlines for videos using a shader.
    auto video_scanlines = std::make_shared<SwitchComponent>();
    video_scanlines->setState(Settings::getInstance()->getBool("MediaViewerVideoScanlines"));
    addWithLabel("RENDER SCANLINES FOR VIDEOS", video_scanlines);
    addSaveFunc([video_scanlines, this] {
        if (video_scanlines->getState() !=
            Settings::getInstance()->getBool("MediaViewerVideoScanlines")) {
            Settings::getInstance()->setBool("MediaViewerVideoScanlines",
                                             video_scanlines->getState());
            setNeedsSaving();
        }
    });

    // Render blur for videos using a shader.
    auto video_blur = std::make_shared<SwitchComponent>();
    video_blur->setState(Settings::getInstance()->getBool("MediaViewerVideoBlur"));
    addWithLabel("RENDER BLUR FOR VIDEOS", video_blur);
    addSaveFunc([video_blur, this] {
        if (video_blur->getState() != Settings::getInstance()->getBool("MediaViewerVideoBlur")) {
            Settings::getInstance()->setBool("MediaViewerVideoBlur", video_blur->getState());
            setNeedsSaving();
        }
    });

    // Render scanlines for screenshots and title screens using a shader.
    auto screenshot_scanlines = std::make_shared<SwitchComponent>();
    screenshot_scanlines->setState(
        Settings::getInstance()->getBool("MediaViewerScreenshotScanlines"));
    addWithLabel("RENDER SCANLINES FOR SCREENSHOTS AND TITLES", screenshot_scanlines);
    addSaveFunc([screenshot_scanlines, this] {
        if (screenshot_scanlines->getState() !=
            Settings::getInstance()->getBool("MediaViewerScreenshotScanlines")) {
            Settings::getInstance()->setBool("MediaViewerScreenshotScanlines",
                                             screenshot_scanlines->getState());
            setNeedsSaving();
        }
    });
}

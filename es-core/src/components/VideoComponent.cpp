//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoComponent.cpp
//
//  Base class for playing videos.
//

#include "components/VideoComponent.h"

#include "ThemeData.h"
#include "Window.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <SDL2/SDL_timer.h>

#define SCREENSAVER_FADE_IN_TIME 1100
#define MEDIA_VIEWER_FADE_IN_TIME 600

VideoComponent::VideoComponent()
    : mVideoWidth {0}
    , mVideoHeight {0}
    , mTargetSize {0.0f, 0.0f}
    , mVideoAreaPos {0.0f, 0.0f}
    , mVideoAreaSize {0.0f, 0.0f}
    , mStartDelayed {false}
    , mIsPlaying {false}
    , mIsActuallyPlaying {false}
    , mPause {false}
    , mShowing {false}
    , mDisable {false}
    , mMediaViewerMode {false}
    , mScreensaverActive {false}
    , mScreensaverMode {false}
    , mGameLaunched {false}
    , mBlockPlayer {false}
    , mTargetIsMax {false}
    , mDrawPillarboxes {true}
    , mRenderScanlines {false}
    , mLegacyTheme {false}
    , mFadeIn {1.0f}
    , mFadeInTime {1000.0f}
{
    // Setup the default configuration.
    mConfig.showSnapshotDelay = false;
    mConfig.showSnapshotNoVideo = false;
    mConfig.startDelay = 1500;

    if (mWindow->getGuiStackSize() > 1)
        topWindow(false);
}

VideoComponent::~VideoComponent()
{
    // Stop any currently running video.
    stopVideo();
}

bool VideoComponent::setVideo(std::string path)
{
    // Convert the path into a generic format.
    std::string fullPath = Utils::FileSystem::getCanonicalPath(path);

    // Check that it's changed.
    if (fullPath == mVideoPath)
        return !path.empty();

    // Store the path.
    mVideoPath = fullPath;

    // If the file exists then set the new video.
    if (!fullPath.empty() && ResourceManager::getInstance().fileExists(fullPath)) {
        // Return true to show that we are going to attempt to play a video.
        return true;
    }

    // Return false to show that no video will be displayed.
    return false;
}

void VideoComponent::setImage(const std::string& path, bool tile)
{
    std::string imagePath {path};

    if (imagePath == "")
        imagePath = mDefaultImagePath;

    // Check that the image has changed.
    if (imagePath == mStaticImagePath)
        return;

    mStaticImage.setImage(imagePath, tile);
    mStaticImagePath = imagePath;
}

void VideoComponent::onShow()
{
    mBlockPlayer = false;
    mPause = false;
    mShowing = true;
    manageState();
}

void VideoComponent::onHide()
{
    mShowing = false;
    manageState();
}

void VideoComponent::onStopVideo()
{
    stopVideo();
    manageState();
}

void VideoComponent::onPauseVideo()
{
    mBlockPlayer = true;
    mPause = true;
    manageState();
}

void VideoComponent::onUnpauseVideo()
{
    mBlockPlayer = false;
    mPause = false;
    manageState();
}

void VideoComponent::onScreensaverActivate()
{
    mBlockPlayer = true;
    mPause = true;

    if (Settings::getInstance()->getString("ScreensaverType") == "dim")
        stopVideo();
    else
        pauseVideo();
    manageState();
}

void VideoComponent::onScreensaverDeactivate()
{
    mBlockPlayer = false;
    // Stop video when deactivating the screensaver to force a reload of the
    // static image (if the theme is configured as such).
    stopVideo();
    manageState();
}

void VideoComponent::onGameLaunchedActivate()
{
    mGameLaunched = true;
    manageState();
}

void VideoComponent::onGameLaunchedDeactivate()
{
    mGameLaunched = false;
    stopVideo();
    manageState();
}

void VideoComponent::topWindow(bool isTop)
{
    if (isTop) {
        mBlockPlayer = false;
        mPause = false;

        // Stop video when closing the menu to force a reload of the
        // static image (if the theme is configured as such).
        stopVideo();
    }
    else {
        mBlockPlayer = true;
        mPause = true;
    }
    manageState();
}

void VideoComponent::render(const glm::mat4& parentTrans)
{
    if (!isVisible())
        return;

    glm::mat4 trans {parentTrans * getTransform()};
    GuiComponent::renderChildren(trans);

    Renderer::setMatrix(trans);

    // Handle the case where the video is delayed.
    handleStartDelay();

    // Handle looping of the video.
    handleLooping();

    // Pause video in case a game has been launched.
    pauseVideo();
}

void VideoComponent::renderSnapshot(const glm::mat4& parentTrans)
{
    // This function is called when the video is not currently being played. We need to
    // work out if we should display a static image. If the menu is open, then always render
    // the static image as the metadata may have been changed. In that case the gamelist
    // was reloaded and there would just be a blank space unless we render the image here.
    // The side effect of this is that a static image is displayed even for themes that are
    // set to start playing the video immediately. Although this may seem a bit inconsistent it
    // simply looks better than leaving an empty space where the video would have been located.
    if (mWindow->getGuiStackSize() > 1 || (mConfig.showSnapshotNoVideo && mVideoPath.empty()) ||
        (mStartDelayed && mConfig.showSnapshotDelay)) {
        mStaticImage.setOpacity(mOpacity * mThemeOpacity);
        mStaticImage.render(parentTrans);
    }
}

void VideoComponent::applyTheme(const std::shared_ptr<ThemeData>& theme,
                                const std::string& view,
                                const std::string& element,
                                unsigned int properties)
{
    using namespace ThemeFlags;
    GuiComponent::applyTheme(theme, view, element,
                             (properties ^ ThemeFlags::SIZE) |
                                 ((properties & (ThemeFlags::SIZE | POSITION)) ? ORIGIN : 0));

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "video");

    mLegacyTheme = theme->isLegacyTheme();

    if (!elem)
        return;

    glm::vec2 scale {getParent() ?
                         getParent()->getSize() :
                         glm::vec2 {Renderer::getScreenWidth(), Renderer::getScreenHeight()}};

    if (properties & ThemeFlags::SIZE) {
        if (elem->has("size")) {
            setResize(elem->get<glm::vec2>("size") * scale);
            mVideoAreaSize = elem->get<glm::vec2>("size") * scale;
        }
        else if (elem->has("maxSize")) {
            setMaxSize(elem->get<glm::vec2>("maxSize") * scale);
            mVideoAreaSize = elem->get<glm::vec2>("maxSize") * scale;
        }
    }

    if (properties & ThemeFlags::POSITION) {
        if (elem->has("pos"))
            mVideoAreaPos = elem->get<glm::vec2>("pos") * scale;
    }

    if (elem->has("interpolation")) {
        const std::string interpolation {elem->get<std::string>("interpolation")};
        if (interpolation == "linear") {
            mStaticImage.setLinearInterpolation(true);
        }
        else if (interpolation == "nearest") {
            mStaticImage.setLinearInterpolation(false);
        }
        else {
            mStaticImage.setLinearInterpolation(false);
            LOG(LogWarning) << "ImageComponent: Invalid theme configuration, property "
                               "<interpolation> set to \""
                            << interpolation << "\"";
        }
    }

    if (elem->has("default"))
        mConfig.defaultVideoPath = elem->get<std::string>("default");

    if (elem->has("defaultImage")) {
        mStaticImage.setDefaultImage(elem->get<std::string>("defaultImage"));
        mStaticImage.setImage(mStaticImagePath);
        mDefaultImagePath = elem->get<std::string>("defaultImage");
    }

    if (elem->has("path"))
        mConfig.staticVideoPath = elem->get<std::string>("path");

    if ((properties & ThemeFlags::DELAY) && elem->has("delay"))
        mConfig.startDelay =
            static_cast<unsigned int>(glm::clamp(elem->get<float>("delay"), 0.0f, 15.0f) * 1000.0f);

    if (!theme->isLegacyTheme())
        mConfig.showSnapshotNoVideo = true;
    else if (elem->has("showSnapshotNoVideo"))
        mConfig.showSnapshotNoVideo = elem->get<bool>("showSnapshotNoVideo");

    if (!theme->isLegacyTheme() && mConfig.startDelay != 0)
        mConfig.showSnapshotDelay = true;
    else if (elem->has("showSnapshotDelay"))
        mConfig.showSnapshotDelay = elem->get<bool>("showSnapshotDelay");

    if (properties && elem->has("fadeInTime"))
        mFadeInTime = glm::clamp(elem->get<float>("fadeInTime"), 0.0f, 8.0f) * 1000.0f;

    if (properties && elem->has("imageType")) {
        std::string imageTypes {elem->get<std::string>("imageType")};
        for (auto& character : imageTypes) {
            if (std::isspace(character))
                character = ',';
        }
        imageTypes = Utils::String::replace(imageTypes, ",,", ",");
        mThemeImageTypes = Utils::String::delimitedStringToVector(imageTypes, ",");
    }

    if (elem->has("pillarboxes"))
        mDrawPillarboxes = elem->get<bool>("pillarboxes");

    if (elem->has("scanlines"))
        mRenderScanlines = elem->get<bool>("scanlines");

    if (elem->has("scrollFadeIn") && elem->get<bool>("scrollFadeIn"))
        mComponentThemeFlags |= ComponentThemeFlags::SCROLL_FADE_IN;
}

std::vector<HelpPrompt> VideoComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> ret;
    ret.push_back(HelpPrompt("a", "select"));
    return ret;
}

void VideoComponent::update(int deltaTime)
{
    if (mBlockPlayer) {
        setImage(mStaticImagePath);
        return;
    }

    manageState();

    // Fade in videos, the time period is a bit different between the screensaver and media viewer.
    // For the theme controlled videos in the gamelist and system views, the fade-in time is set
    // via the theme configuration.
    if (mScreensaverMode && mFadeIn < 1.0f) {
        mFadeIn = glm::clamp(mFadeIn + (deltaTime / static_cast<float>(SCREENSAVER_FADE_IN_TIME)),
                             0.0f, 1.0f);
    }
    else if (mMediaViewerMode && mFadeIn < 1.0f) {
        mFadeIn = glm::clamp(mFadeIn + (deltaTime / static_cast<float>(MEDIA_VIEWER_FADE_IN_TIME)),
                             0.0f, 1.0f);
    }
    else if (mFadeIn < 1.0f) {
        mFadeIn = glm::clamp(mFadeIn + (deltaTime / static_cast<float>(mFadeInTime)), 0.0f, 1.0f);
    }

    GuiComponent::update(deltaTime);
}

void VideoComponent::startVideoWithDelay()
{
    mPause = false;

    // If not playing then either start the video or initiate the delay.
    if (!mIsPlaying) {
        // Set the video that we are going to be playing so we don't attempt to restart it.
        mPlayingVideoPath = mVideoPath;

        if (mConfig.startDelay == 0) {
            // No delay. Just start the video.
            mStartDelayed = false;
            startVideo();
        }
        else {
            // Configure the start delay.
            mStartDelayed = true;
            mStartTime = SDL_GetTicks() + mConfig.startDelay;
        }
        mIsPlaying = true;
    }
}

void VideoComponent::handleStartDelay()
{
    if (mBlockPlayer || mGameLaunched)
        return;

    // Only play if any delay has timed out.
    if (mStartDelayed) {
        // If the setting to override the theme-supplied video delay setting has been enabled,
        // then play the video immediately.
        if (!Settings::getInstance()->getBool("PlayVideosImmediately")) {
            // If there is a video file available but no static image, then start playing the
            // video immediately regardless of theme configuration or settings.
            if (mStaticImagePath != "") {
                if (mStartTime > SDL_GetTicks()) {
                    // Timeout not yet completed.
                    return;
                }
            }
        }
        // Completed.
        mStartDelayed = false;
        // Clear the playing flag so startVideo works.
        mIsPlaying = false;
        startVideo();
    }
}

void VideoComponent::manageState()
{
    // We will only show the video if the component is on display and the screensaver
    // is not active.
    bool show = mShowing && !mScreensaverActive && !mDisable;

    // See if we're already playing.
    if (mIsPlaying) {
        // If we are not on display then stop the video from playing.
        if (!show) {
            stopVideo();
        }
        else {
            if (mVideoPath != mPlayingVideoPath) {
                // Path changed. Stop the video. We will start it again below because
                // mIsPlaying will be modified by stopVideo to be false.
                stopVideo();
            }
        }
        updatePlayer();
    }
    // Need to recheck variable rather than 'else' because it may be modified above.
    if (!mIsPlaying) {
        // If we are on display then see if we should start the video.
        if (show && !mVideoPath.empty())
            startVideoWithDelay();
    }

    // If a game has just been launched and a video is actually shown, then request a
    // pause of the video so it doesn't continue to play in the background while the
    // game is running.
    if (mGameLaunched && show && !mPause)
        mPause = true;
}

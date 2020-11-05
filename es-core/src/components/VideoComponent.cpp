//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VideoComponent.cpp
//
//  Base class for playing videos.
//

#include "components/VideoComponent.h"

#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "PowerSaver.h"
#include "ThemeData.h"
#include "Window.h"

#include <SDL2/SDL_timer.h>

#define SCREENSAVER_FADE_IN_TIME 800

void VideoComponent::setScreensaverMode(bool isScreensaver)
{
    mScreensaverMode = isScreensaver;
}

VideoComponent::VideoComponent(
    Window* window)
    : GuiComponent(window),
    mStaticImage(window),
    mVideoHeight(0),
    mVideoWidth(0),
    mStartDelayed(false),
    mIsPlaying(false),
    mPause(false),
    mShowing(false),
    mDisable(false),
    mScreensaverActive(false),
    mScreensaverMode(false),
    mGameLaunched(false),
    mBlockPlayer(false),
    mTargetIsMax(false),
    mFadeIn(1.0),
    mTargetSize(0, 0)
{
    // Setup the default configuration.
    mConfig.showSnapshotDelay = false;
    mConfig.showSnapshotNoVideo = false;
    mConfig.startDelay = 0;

    if (mWindow->getGuiStackSize() > 1)
        topWindow(false);
}

VideoComponent::~VideoComponent()
{
    // Stop any currently running video.
    stopVideo();
}

void VideoComponent::onOriginChanged()
{
    // Update the embeded static image.
    mStaticImage.setOrigin(mOrigin);
}

void VideoComponent::onPositionChanged()
{
    // Update the embeded static image.
    mStaticImage.setPosition(mPosition);
}

void VideoComponent::onSizeChanged()
{
    // Update the embeded static image.
    mStaticImage.onSizeChanged();
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
    if (!fullPath.empty() && ResourceManager::getInstance()->fileExists(fullPath)) {
        // Return true to show that we are going to attempt to play a video.
        return true;
    }

    // Return false to show that no video will be displayed.
    return false;
}

void VideoComponent::setImage(std::string path)
{
    // Check that the image has changed.
    if (path == mStaticImagePath)
        return;

    mStaticImage.setImage(path);
    mStaticImagePath = path;
}

void VideoComponent::setDefaultVideo()
{
    setVideo(mConfig.defaultVideoPath);
}

void VideoComponent::setOpacity(unsigned char opacity)
{
    // Set the opacity for the embedded static image.
    mOpacity = opacity;
}

void VideoComponent::render(const Transform4x4f& parentTrans)
{
    if (!isVisible())
        return;

    Transform4x4f trans = parentTrans * getTransform();
    GuiComponent::renderChildren(trans);

    Renderer::setMatrix(trans);

    // Handle the case where the video is delayed.
    handleStartDelay();

    // Handle looping of the video.
    handleLooping();

    // Pause video in case a game has been launched.
    pauseVideo();
}

void VideoComponent::renderSnapshot(const Transform4x4f& parentTrans)
{
    // This is the case where the video is not currently being displayed. Work out
    // if we need to display a static image.
    if ((mConfig.showSnapshotNoVideo && mVideoPath.empty()) ||
            (mStartDelayed && mConfig.showSnapshotDelay)) {
        // Display the static image instead.
        mStaticImage.setOpacity(mOpacity);
        mStaticImage.render(parentTrans);
    }
}

void VideoComponent::applyTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view,
        const std::string& element, unsigned int properties)
{
    using namespace ThemeFlags;

    GuiComponent::applyTheme(theme, view, element, (properties ^ ThemeFlags::SIZE) |
            ((properties & (ThemeFlags::SIZE | POSITION)) ? ORIGIN : 0));

    const ThemeData::ThemeElement* elem = theme->getElement(view, element, "video");

    if (!elem)
        return;

    Vector2f scale = getParent() ? getParent()->getSize() :
            Vector2f(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight()));

    if (properties & ThemeFlags::SIZE) {
        if (elem->has("size"))
            setResize(elem->get<Vector2f>("size") * scale);
        else if (elem->has("maxSize"))
            setMaxSize(elem->get<Vector2f>("maxSize") * scale);
    }

    if (elem->has("default"))
        mConfig.defaultVideoPath = elem->get<std::string>("default");

    if ((properties & ThemeFlags::DELAY) && elem->has("delay"))
        mConfig.startDelay = static_cast<unsigned>((elem->get<float>("delay") * 1000.0f));

    if (elem->has("showSnapshotNoVideo"))
        mConfig.showSnapshotNoVideo = elem->get<bool>("showSnapshotNoVideo");

    if (elem->has("showSnapshotDelay"))
        mConfig.showSnapshotDelay = elem->get<bool>("showSnapshotDelay");
}

std::vector<HelpPrompt> VideoComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> ret;
    ret.push_back(HelpPrompt("a", "select"));
    return ret;
}

void VideoComponent::handleStartDelay()
{
    if (mBlockPlayer)
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

void VideoComponent::handleLooping()
{
}

void VideoComponent::startVideoWithDelay()
{
    mPause = false;

    // If not playing then either start the video or initiate the delay.
    if (!mIsPlaying) {
        // Set the video that we are going to be playing so we don't attempt to restart it.
        mPlayingVideoPath = mVideoPath;

        if (mConfig.startDelay == 0 || PowerSaver::getMode() == PowerSaver::INSTANT) {
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

void VideoComponent::update(int deltaTime)
{
    if (mBlockPlayer) {
        setImage(mStaticImagePath);
        return;
    }

    manageState();

    // This is only used to fade in the screensaver, fade-in of the static image is
    // handled using a lambda animation in VideoGameListView.
    if (mFadeIn < 1.0f)
        mFadeIn = Math::clamp(mFadeIn + (deltaTime /
                static_cast<float>(SCREENSAVER_FADE_IN_TIME)), 0.0, 1.0);

    GuiComponent::update(deltaTime);
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

void VideoComponent::onScreenSaverActivate()
{
    mBlockPlayer = true;
    mPause = true;
    if (Settings::getInstance()->getString("ScreensaverBehavior") == "dim")
        stopVideo();
    manageState();
}

void VideoComponent::onScreenSaverDeactivate()
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

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
    , mStartTime {0}
    , mIsPlaying {false}
    , mIsActuallyPlaying {false}
    , mPaused {false}
    , mMediaViewerMode {false}
    , mScreensaverMode {false}
    , mTargetIsMax {false}
    , mPlayAudio {true}
    , mDrawPillarboxes {true}
    , mRenderScanlines {false}
    , mLegacyTheme {false}
    , mHasVideo {false}
    , mFadeIn {1.0f}
    , mFadeInTime {1000.0f}
{
    // Setup default configuration.
    mConfig.showSnapshotDelay = false;
    mConfig.showSnapshotNoVideo = false;
    mConfig.startDelay = 1500;
}

VideoComponent::~VideoComponent()
{
    // Stop any currently running video.
    stopVideoPlayer();
}

bool VideoComponent::setVideo(std::string path)
{
    // Convert the path into a generic format.
    std::string fullPath {Utils::FileSystem::getCanonicalPath(path)};

    // Check that it's changed.
    if (fullPath == mVideoPath)
        return !path.empty();

    // Store the path.
    mVideoPath = fullPath;

    // If the file exists then set the new video.
    if (!fullPath.empty() && ResourceManager::getInstance().fileExists(fullPath)) {
        mHasVideo = true;
        // Return true to show that we are going to attempt to play a video.
        return true;
    }

    if (!mVideoPath.empty() || !mConfig.defaultVideoPath.empty() ||
        !mConfig.staticVideoPath.empty())
        mHasVideo = true;
    else
        mHasVideo = false;

    // Return false to show that no video will be displayed.
    return false;
}

void VideoComponent::setImage(const std::string& path, bool tile)
{
    std::string imagePath {path};

    if (imagePath == "")
        imagePath = mDefaultImagePath;

    // Check if the image has changed.
    if (imagePath == mStaticImagePath)
        return;

    mStaticImage.setImage(imagePath, tile);
    mStaticImagePath = imagePath;
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

    if (elem->has("audio"))
        mPlayAudio = elem->get<bool>("audio");

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
    if (!mHasVideo) {
        // We need this update so the static image gets updated (e.g. used for fade animations).
        GuiComponent::update(deltaTime);
        return;
    }

    // Hack to prevent the video from starting to play if the static image was shown when paused.
    if (mPaused)
        mStartTime = SDL_GetTicks() + mConfig.startDelay;

    if (mWindow->getGameLaunchedState())
        return;

    if (!mIsPlaying && mConfig.startDelay == 0) {
        startVideoStream();
    }
    else if (mStartTime == 0 || SDL_GetTicks() > mStartTime) {
        mStartTime = 0;
        startVideoStream();
    }

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

    if (mIsPlaying)
        updatePlayer();

    handleLooping();

    GuiComponent::update(deltaTime);
}

void VideoComponent::startVideoPlayer()
{
    if (mIsPlaying)
        stopVideoPlayer();

    if (mConfig.startDelay != 0 && mStaticImagePath != "") {
        mStartTime = SDL_GetTicks() + mConfig.startDelay;
        setImage(mStaticImagePath);
    }

    mPaused = false;
}

void VideoComponent::renderSnapshot(const glm::mat4& parentTrans)
{
    if (mLegacyTheme && !mHasVideo && !mConfig.showSnapshotNoVideo)
        return;

    if (mHasVideo && (!mConfig.showSnapshotDelay || mConfig.startDelay == 0))
        return;

    if (mStaticImagePath != "") {
        mStaticImage.setOpacity(mOpacity * mThemeOpacity);
        mStaticImage.render(parentTrans);
    }
}

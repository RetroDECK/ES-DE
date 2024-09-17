//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  VideoComponent.cpp
//
//  Base class for playing videos.
//

#include "components/VideoComponent.h"

#include "ThemeData.h"
#include "Window.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

#define SCREENSAVER_FADE_IN_TIME 900
#define MEDIA_VIEWER_FADE_IN_TIME 600

VideoComponent::VideoComponent()
    : mRenderer {Renderer::getInstance()}
    , mVideoWidth {0}
    , mVideoHeight {0}
    , mColorShift {0xFFFFFFFF}
    , mColorShiftEnd {0xFFFFFFFF}
    , mVideoCornerRadius {0.0f}
    , mColorGradientHorizontal {true}
    , mTargetSize {0.0f, 0.0f}
    , mCropPos {0.5f, 0.5f}
    , mCropOffset {0.0f, 0.0f}
    , mVideoAreaPos {0.0f, 0.0f}
    , mVideoAreaSize {0.0f, 0.0f}
    , mTopLeftCrop {0.0f, 0.0f}
    , mBottomRightCrop {1.0f, 1.0f}
    , mPillarboxThreshold {0.85f, 0.90f}
    , mOnIterationsDone {OnIterationsDone::NOTHING}
    , mStartTime {0}
    , mIsPlaying {false}
    , mIsActuallyPlaying {false}
    , mPaused {false}
    , mMediaViewerMode {false}
    , mScreensaverMode {false}
    , mTargetIsMax {false}
    , mTargetIsCrop {false}
    , mPlayAudio {true}
    , mDrawPillarboxes {true}
    , mRenderScanlines {false}
    , mLinearInterpolation {false}
    , mHasVideo {false}
    , mGeneralFade {false}
    , mFadeIn {1.0f}
    , mFadeInTime {1000.0f}
    , mIterationCount {0}
    , mPlayCount {0}
{
    // Setup default configuration.
    mConfig.showStaticImageDelay = false;
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

    if (!mVideoPath.empty() || !mConfig.staticVideoPath.empty())
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

void VideoComponent::setImageNoDefault(const std::string& path)
{
    // Check if the image has changed.
    if (path == mStaticImagePath)
        return;

    mStaticImage.setImage(path, false);
    mStaticImagePath = path;
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

    const ThemeData::ThemeElement* elem {theme->getElement(view, element, "video")};

    if (!elem)
        return;

    glm::vec2 scale {getParent() ?
                         getParent()->getSize() :
                         glm::vec2 {mRenderer->getScreenWidth(), mRenderer->getScreenHeight()}};

    if (properties & ThemeFlags::SIZE) {
        if (elem->has("size")) {
            glm::vec2 videoSize {elem->get<glm::vec2>("size")};
            if (videoSize == glm::vec2 {0.0f, 0.0f}) {
                LOG(LogWarning) << "VideoComponent: Invalid theme configuration, property \"size\" "
                                   "for element \""
                                << element.substr(6) << "\" is set to zero";
                videoSize = {0.01f, 0.01f};
            }
            if (videoSize.x > 0.0f)
                videoSize.x = glm::clamp(videoSize.x, 0.01f, 2.0f);
            if (videoSize.y > 0.0f)
                videoSize.y = glm::clamp(videoSize.y, 0.01f, 2.0f);
            setResize(videoSize.x * scale.x, videoSize.y * scale.y);
            mVideoAreaSize = videoSize * scale;
        }
        else if (elem->has("maxSize")) {
            glm::vec2 videoMaxSize {elem->get<glm::vec2>("maxSize")};
            videoMaxSize.x = glm::clamp(videoMaxSize.x, 0.01f, 2.0f);
            videoMaxSize.y = glm::clamp(videoMaxSize.y, 0.01f, 2.0f);
            setMaxSize(videoMaxSize * scale);
            mVideoAreaSize = videoMaxSize * scale;
        }
        else if (elem->has("cropSize")) {
            glm::vec2 videoCropSize {elem->get<glm::vec2>("cropSize")};
            videoCropSize.x = glm::clamp(videoCropSize.x, 0.01f, 2.0f);
            videoCropSize.y = glm::clamp(videoCropSize.y, 0.01f, 2.0f);
            if (elem->has("cropPos"))
                mCropPos = glm::clamp(elem->get<glm::vec2>("cropPos"), 0.0f, 1.0f);
            setCroppedSize(videoCropSize * scale);
            mVideoAreaSize = videoCropSize * scale;
        }
    }

    if (properties & ThemeFlags::POSITION) {
        if (elem->has("pos"))
            mVideoAreaPos = elem->get<glm::vec2>("pos") * scale;
    }

    if (properties & ThemeFlags::POSITION && elem->has("stationary")) {
        const std::string& stationary {elem->get<std::string>("stationary")};
        if (stationary == "never")
            mStationary = Stationary::NEVER;
        else if (stationary == "always")
            mStationary = Stationary::ALWAYS;
        else if (stationary == "withinView")
            mStationary = Stationary::WITHIN_VIEW;
        else if (stationary == "betweenViews")
            mStationary = Stationary::BETWEEN_VIEWS;
        else
            LOG(LogWarning) << "VideoComponent: Invalid theme configuration, property "
                               "\"stationary\" for element \""
                            << element.substr(6) << "\" defined as \"" << stationary << "\"";
    }

    if (elem->has("metadataElement") && elem->get<bool>("metadataElement"))
        mComponentThemeFlags |= ComponentThemeFlags::METADATA_ELEMENT;

    if (elem->has("iterationCount")) {
        mIterationCount = glm::clamp(elem->get<unsigned int>("iterationCount"), 0u, 10u);

        if (properties && elem->has("onIterationsDone")) {
            const std::string& onIterationsDone {elem->get<std::string>("onIterationsDone")};
            if (onIterationsDone == "nothing")
                mOnIterationsDone = OnIterationsDone::NOTHING;
            else if (onIterationsDone == "image")
                mOnIterationsDone = OnIterationsDone::IMAGE;
            else
                LOG(LogWarning) << "VideoComponent: Invalid theme configuration, property "
                                   "\"onIterationsDone\" for element \""
                                << element.substr(6) << "\" defined as \"" << onIterationsDone
                                << "\"";
        }
    }

    if (elem->has("audio"))
        mPlayAudio = elem->get<bool>("audio");

    mStaticImage.setRotation(mRotation);
    mStaticImage.setRotationOrigin(mRotationOrigin);

    // Enable linear interpolation by default if element is arbitrarily rotated.
    if (properties & ThemeFlags::ROTATION && elem->has("rotation")) {
        const float rotation {std::abs(elem->get<float>("rotation"))};
        if (rotation != 0.0f &&
            (std::round(rotation) != rotation || static_cast<int>(rotation) % 90 != 0)) {
            mLinearInterpolation = true;
            mStaticImage.setLinearInterpolation(true);
        }
    }

    if (elem->has("interpolation")) {
        const std::string& interpolation {elem->get<std::string>("interpolation")};
        if (interpolation == "linear") {
            mLinearInterpolation = true;
            mStaticImage.setLinearInterpolation(true);
        }
        else if (interpolation == "nearest") {
            mLinearInterpolation = false;
            mStaticImage.setLinearInterpolation(false);
        }
        else {
            LOG(LogWarning) << "VideoComponent: Invalid theme configuration, property "
                               "\"interpolation\" for element \""
                            << element.substr(6) << "\" defined as \"" << interpolation << "\"";
        }
    }

    if (elem->has("imageCornerRadius"))
        mStaticImage.setCornerRadius(glm::clamp(elem->get<float>("imageCornerRadius"), 0.0f, 0.5f) *
                                     mRenderer->getScreenWidth());

    if (elem->has("videoCornerRadius"))
        mVideoCornerRadius = glm::clamp(elem->get<float>("videoCornerRadius"), 0.0f, 0.5f) *
                             mRenderer->getScreenWidth();

    if (elem->has("default")) {
        const std::string defaultVideo {elem->get<std::string>("default")};
        if (ResourceManager::getInstance().fileExists(defaultVideo)) {
            mConfig.defaultVideoPath = defaultVideo;
        }
        else {
            LOG(LogWarning)
                << "VideoComponent: File defined for property \"default\" for element \""
                << element.substr(6) << "\" does not exist: \"" << defaultVideo << "\"";
        }
    }

    if (elem->has("defaultImage")) {
        mStaticImage.setDefaultImage(elem->get<std::string>("defaultImage"));
        mStaticImage.setImage(mStaticImagePath);
        mDefaultImagePath = elem->get<std::string>("defaultImage");
    }

    if (elem->has("path")) {
        const std::string& staticPath {elem->get<std::string>("path")};
        if (ResourceManager::getInstance().fileExists(staticPath)) {
            mConfig.staticVideoPath = staticPath;
        }
        else {
            mThemeGameSelector = ":none:";
            LOG(LogWarning) << "VideoComponent: File defined for property \"path\" for element \""
                            << element.substr(6) << "\" does not exist: \"" << staticPath << "\"";
        }
    }

    if ((properties & ThemeFlags::DELAY) && elem->has("delay"))
        mConfig.startDelay =
            static_cast<unsigned int>(glm::clamp(elem->get<float>("delay"), 0.0f, 15.0f) * 1000.0f);

    if (mConfig.startDelay != 0)
        mConfig.showStaticImageDelay = true;

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

        if (mThemeImageTypes.empty()) {
            LOG(LogError) << "VideoComponent: Invalid theme configuration, property \"imageType\" "
                             "for element \""
                          << element.substr(6) << "\" contains no values";
        }

        for (std::string& type : mThemeImageTypes) {
            if (std::find(supportedImageTypes.cbegin(), supportedImageTypes.cend(), type) ==
                supportedImageTypes.cend()) {
                LOG(LogError)
                    << "VideoComponent: Invalid theme configuration, property \"imageType\" "
                       "for element \""
                    << element.substr(6) << "\" defined as \"" << type << "\"";
                mThemeImageTypes.clear();
                break;
            }
        }

        std::vector<std::string> sortedTypes {mThemeImageTypes};
        std::stable_sort(sortedTypes.begin(), sortedTypes.end());

        if (std::adjacent_find(sortedTypes.begin(), sortedTypes.end()) != sortedTypes.end()) {
            LOG(LogError) << "VideoComponent: Invalid theme configuration, property \"imageType\" "
                             "for element \""
                          << element.substr(6) << "\" contains duplicate values";
            mThemeImageTypes.clear();
        }
    }

    if (mThemeImageTypes.empty())
        mConfig.startDelay = 0;

    if (elem->has("color")) {
        mColorShift = elem->get<unsigned int>("color");
        mColorShiftEnd = mColorShift;
    }
    if (elem->has("colorEnd"))
        mColorShiftEnd = elem->get<unsigned int>("colorEnd");

    if (elem->has("gradientType")) {
        const std::string& gradientType {elem->get<std::string>("gradientType")};
        if (gradientType == "horizontal") {
            mColorGradientHorizontal = true;
        }
        else if (gradientType == "vertical") {
            mColorGradientHorizontal = false;
        }
        else {
            mColorGradientHorizontal = true;
            LOG(LogWarning) << "VideoComponent: Invalid theme configuration, property "
                               "\"gradientType\" for element \""
                            << element.substr(6) << "\" defined as \"" << gradientType << "\"";
        }
    }

    if (elem->has("pillarboxes"))
        mDrawPillarboxes = elem->get<bool>("pillarboxes");

    // The black frame is rendered behind all videos and may be expanded to render pillarboxes
    // or letterboxes.
    mBlackFrame.setZIndex(mZIndex);
    mBlackFrame.setCornerRadius(mVideoCornerRadius);
    mBlackFrame.setCornerAntiAliasing(false);
    mBlackFrame.setColorShift(0x000000FF);
    mBlackFrame.setImage(":/graphics/white.png");

    if (elem->has("pillarboxThreshold")) {
        const glm::vec2 pillarboxThreshold {elem->get<glm::vec2>("pillarboxThreshold")};
        mPillarboxThreshold.x = glm::clamp(pillarboxThreshold.x, 0.2f, 1.0f);
        mPillarboxThreshold.y = glm::clamp(pillarboxThreshold.y, 0.2f, 1.0f);
    }

    if (elem->has("scanlines")) {
        if (elem->has("cropSize") && elem->get<bool>("scanlines")) {
            LOG(LogWarning) << "VideoComponent: Invalid theme configuration, property "
                               "\"cropSize\" for element \""
                            << element.substr(6)
                            << "\" can't be combined with the \"scanlines\" property";
        }
        else {
            mRenderScanlines = elem->get<bool>("scanlines");
        }
    }

    if (elem->has("scrollFadeIn") && elem->get<bool>("scrollFadeIn"))
        mComponentThemeFlags |= ComponentThemeFlags::SCROLL_FADE_IN;
}

std::vector<HelpPrompt> VideoComponent::getHelpPrompts()
{
    std::vector<HelpPrompt> ret;
    ret.push_back(HelpPrompt("a", _("select")));
    return ret;
}

void VideoComponent::update(int deltaTime)
{
    if (mIterationCount != 0 && mPlayCount == mIterationCount)
        return;

    // A deltaTime value of 0 would lead to mFadeIn being an invalid number which would prevent
    // the video from being rendered. This can happen on application startup in some instances.
    if (deltaTime == 0)
        deltaTime = 1;

    if (!mHasVideo) {
        // We need this update so the static image gets updated (e.g. used for fade animations).
        GuiComponent::update(deltaTime);
        return;
    }

    if (mVideoPath == "")
        return;

    // Hack to prevent the video from starting to play if the static image was shown when paused.
    if (mConfig.showStaticImageDelay && mPaused)
        mStartTime = SDL_GetTicks() + mConfig.startDelay;

    if (mWindow->getGameLaunchedState())
        return;

    if (!mIsPlaying && (mConfig.startDelay == 0 || mStaticImagePath == "")) {
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
    mPlayCount = 0;

    if (mIsPlaying)
        stopVideoPlayer();

    if (mConfig.showStaticImageDelay && mConfig.startDelay != 0 && mStaticImagePath != "") {
        mStartTime = SDL_GetTicks() + mConfig.startDelay;
        setImage(mStaticImagePath);
    }

    mPaused = false;
}

void VideoComponent::renderStaticImage(const glm::mat4& parentTrans, bool forceRender)
{
    if (mHasVideo && (!forceRender && (!mConfig.showStaticImageDelay || mConfig.startDelay == 0)))
        return;

    if (mStaticImagePath != "") {
        mStaticImage.setOpacity(mOpacity * mThemeOpacity);
        mStaticImage.setSaturation(mSaturation * mThemeSaturation);
        if (mBrightness != 0.0f)
            mStaticImage.setBrightness(mBrightness);
        if (mColorShift != 0xFFFFFFFF)
            mStaticImage.setColorShift(mColorShift);
        if (mColorShift != mColorShiftEnd)
            mStaticImage.setColorShiftEnd(mColorShiftEnd);
        if (!mColorGradientHorizontal)
            mStaticImage.setColorGradientHorizontal(mColorGradientHorizontal);
        mStaticImage.setDimming(mDimming);
        mStaticImage.render(parentTrans);
    }
}

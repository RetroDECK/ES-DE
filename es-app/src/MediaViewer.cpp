//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MediaViewer.cpp
//
//  Fullscreen game media viewer.
//

#include "MediaViewer.h"

#include "components/VideoFFmpegComponent.h"
#if defined(BUILD_VLC_PLAYER)
#include "components/VideoVlcComponent.h"
#endif
#include "AudioManager.h"
#include "Sound.h"
#include "views/ViewController.h"

MediaViewer::MediaViewer(Window* window)
    : mWindow(window)
    , mVideo(nullptr)
    , mImage(nullptr)
{
    mWindow->setMediaViewer(this);
}

MediaViewer::~MediaViewer()
{
    if (mVideo) {
        delete mVideo;
        mVideo = nullptr;
    }
    if (mImage) {
        delete mImage;
        mImage = nullptr;
    }
}

bool MediaViewer::startMediaViewer(FileData* game)
{
    mHasVideo = false;
    mHasImages = false;
    mCurrentImageIndex = 0;
    mScreenShotIndex = -1;

    mGame = game;

    initiateViewer();

    if (mHasVideo)
        ViewController::get()->onPauseVideo();

    if (mHasVideo || mHasImages)
        return true;
    else
        return false;
}

void MediaViewer::stopMediaViewer()
{
    NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
    ViewController::get()->onStopVideo();

    if (mVideo) {
        delete mVideo;
        mVideo = nullptr;
    }

    if (mImage) {
        delete mImage;
        mImage = nullptr;
    }

    mVideoFile = "";
    mImageFiles.clear();
}

void MediaViewer::update(int deltaTime)
{
    if (mVideo)
        mVideo->update(deltaTime);
}

void MediaViewer::render()
{
    Transform4x4f transform = Transform4x4f::Identity();
    Renderer::setMatrix(transform);

    // Render a black background below the game media.
    Renderer::drawRect(0.0f, 0.0f, static_cast<float>(Renderer::getScreenWidth()),
                       static_cast<float>(Renderer::getScreenHeight()), 0x000000FF, 0x000000FF);

    if (mVideo && !mDisplayingImage) {
        mVideo->render(transform);

#if defined(USE_OPENGL_21)
        Renderer::shaderParameters videoParameters;
        unsigned int shaders = 0;
        if (Settings::getInstance()->getBool("MediaViewerVideoScanlines"))
            shaders = Renderer::SHADER_SCANLINES;
        if (Settings::getInstance()->getBool("MediaViewerVideoBlur")) {
            shaders |= Renderer::SHADER_BLUR_HORIZONTAL;
            float heightModifier = Renderer::getScreenHeightModifier();
            // clang-format off
            if (heightModifier < 1)
                videoParameters.blurPasses = 2;        // Below 1080
            else if (heightModifier >= 4)
                videoParameters.blurPasses = 12;       // 8K
            else if (heightModifier >= 2.9)
                videoParameters.blurPasses = 10;       // 6K
            else if (heightModifier >= 2.6)
                videoParameters.blurPasses = 8;        // 5K
            else if (heightModifier >= 2)
                videoParameters.blurPasses = 5;        // 4K
            else if (heightModifier >= 1.3)
                videoParameters.blurPasses = 3;        // 1440
            else if (heightModifier >= 1)
                videoParameters.blurPasses = 2;        // 1080
            // clang-format on
        }
        Renderer::shaderPostprocessing(shaders, videoParameters);
#endif
    }
    else if (mImage && mImage->hasImage() && mImage->getSize() != 0) {
        mImage->render(transform);

#if defined(USE_OPENGL_21)
        if (mCurrentImageIndex == mScreenShotIndex &&
            Settings::getInstance()->getBool("MediaViewerScreenshotScanlines"))
            Renderer::shaderPostprocessing(Renderer::SHADER_SCANLINES);
#endif

        // This is necessary so that the video loops if viewing an image when
        // the video ends.
        if (mVideo)
            mVideo->handleLooping();
    }
}

void MediaViewer::initiateViewer()
{
    if (mGame->getType() == PLACEHOLDER)
        return;

    findMedia();

    if (!mHasVideo && !mHasImages)
        return;

    if (mHasVideo)
        playVideo();
    else
        showImage(0);
}

void MediaViewer::findMedia()
{
    std::string mediaFile;

    if ((mediaFile = mGame->getVideoPath()) != "") {
        mVideoFile = mediaFile;
        mHasVideo = true;
    }

    if (!mHasVideo && (mediaFile = mGame->getScreenshotPath()) != "") {
        mImageFiles.push_back(mediaFile);
        mScreenShotIndex = 0;
    }

    if ((mediaFile = mGame->getCoverPath()) != "")
        mImageFiles.push_back(mediaFile);

    if (mHasVideo && (mediaFile = mGame->getScreenshotPath()) != "") {
        mImageFiles.push_back(mediaFile);
        mScreenShotIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if ((mediaFile = mGame->getMiximagePath()) != "")
        mImageFiles.push_back(mediaFile);

    if (!mImageFiles.empty())
        mHasImages = true;
}

void MediaViewer::showNext()
{
    if (mHasImages && mCurrentImageIndex != mImageFiles.size() - 1)
        NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);

    bool showedVideo = false;

    if (mVideo && !mHasImages) {
        return;
    }
    else if (mVideo && !Settings::getInstance()->getBool("MediaViewerKeepVideoRunning")) {
        delete mVideo;
        mVideo = nullptr;
        showedVideo = true;
    }

    if (mImage) {
        delete mImage;
        mImage = nullptr;
    }

    if ((mVideo || showedVideo) && !mDisplayingImage)
        mCurrentImageIndex = 0;
    else if (mImageFiles.size() > mCurrentImageIndex + 1)
        mCurrentImageIndex++;

    if (mVideo)
        mDisplayingImage = true;

    showImage(mCurrentImageIndex);
}

void MediaViewer::showPrevious()
{
    if ((mHasVideo && mDisplayingImage) || (!mHasVideo && mCurrentImageIndex != 0))
        NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);

    if (mCurrentImageIndex == 0 && !mHasVideo) {
        return;
    }
    else if (mCurrentImageIndex == 0 && mHasVideo) {
        if (mImage) {
            delete mImage;
            mImage = nullptr;
        }
        mDisplayingImage = false;
        playVideo();
        return;
    }

    if (mImage) {
        delete mImage;
        mImage = nullptr;
    }

    mCurrentImageIndex--;
    showImage(mCurrentImageIndex);
}

void MediaViewer::playVideo()
{
    if (mVideo || mVideoFile == "")
        return;

    mDisplayingImage = false;
    ViewController::get()->onStopVideo();

#if defined(BUILD_VLC_PLAYER)
    if (Settings::getInstance()->getString("VideoPlayer") == "ffmpeg")
        mVideo = new VideoFFmpegComponent(mWindow);
    else
        mVideo = new VideoVlcComponent(mWindow);
#else
    mVideo = new VideoFFmpegComponent(mWindow);
#endif

    mVideo->topWindow(true);
    mVideo->setOrigin(0.5f, 0.5f);
    mVideo->setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);

    if (Settings::getInstance()->getBool("MediaViewerStretchVideos"))
        mVideo->setResize(static_cast<float>(Renderer::getScreenWidth()),
                          static_cast<float>(Renderer::getScreenHeight()));
    else
        mVideo->setMaxSize(static_cast<float>(Renderer::getScreenWidth()),
                           static_cast<float>(Renderer::getScreenHeight()));

    mVideo->setVideo(mVideoFile);
    mVideo->setMediaViewerMode(true);
    mVideo->onShow();
}

void MediaViewer::showImage(int index)
{
    if (mImage)
        return;

    mDisplayingImage = true;

    if (!mImageFiles.empty() && mImageFiles.size() >= index) {
        mImage = new ImageComponent(mWindow, false, false);
        mImage->setImage(mImageFiles[index]);
        mImage->setOrigin(0.5f, 0.5f);
        mImage->setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);
        mImage->setMaxSize(static_cast<float>(Renderer::getScreenWidth()),
                           static_cast<float>(Renderer::getScreenHeight()));
    }
}

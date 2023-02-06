//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MediaViewer.cpp
//
//  Fullscreen game media viewer.
//

#include "MediaViewer.h"

#include "Sound.h"
#include "components/VideoFFmpegComponent.h"
#include "views/ViewController.h"

MediaViewer::MediaViewer()
    : mRenderer {Renderer::getInstance()}
{
    Window::getInstance()->setMediaViewer(this);
}

bool MediaViewer::startMediaViewer(FileData* game)
{
    mHasVideo = false;
    mHasImages = false;
    mCurrentImageIndex = 0;
    mScreenshotIndex = -1;
    mTitleScreenIndex = -1;

    mGame = game;

    initiateViewer();

    if (mHasVideo || mHasImages)
        return true;
    else
        return false;
}

void MediaViewer::stopMediaViewer()
{
    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
    ViewController::getInstance()->stopViewVideos();

    mVideoFile = "";
    mVideo.reset();
    mImageFiles.clear();
    mImages.clear();
}

void MediaViewer::update(int deltaTime)
{
    if (mVideo)
        mVideo->update(deltaTime);
}

void MediaViewer::render(const glm::mat4& /*parentTrans*/)
{
    glm::mat4 trans {Renderer::getIdentity()};
    mRenderer->setMatrix(trans);

    // Render a black background below the game media.
    mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                        0x000000FF, 0x000000FF);

    if (mVideo && !mDisplayingImage) {
        mVideo->render(trans);

        Renderer::postProcessingParams videoParameters;
        unsigned int shaders {0};
        if (Settings::getInstance()->getBool("MediaViewerVideoScanlines"))
            shaders = Renderer::Shader::SCANLINES;
        if (Settings::getInstance()->getBool("MediaViewerVideoBlur")) {
            shaders |= Renderer::Shader::BLUR_HORIZONTAL;
            const float resolutionModifier {mRenderer->getScreenResolutionModifier()};
            // clang-format off
            if (resolutionModifier < 1)
                videoParameters.blurPasses = 2;        // Below 1080
            else if (resolutionModifier >= 4)
                videoParameters.blurPasses = 12;       // 8K
            else if (resolutionModifier >= 2.9)
                videoParameters.blurPasses = 10;       // 6K
            else if (resolutionModifier >= 2.6)
                videoParameters.blurPasses = 8;        // 5K
            else if (resolutionModifier >= 2)
                videoParameters.blurPasses = 5;        // 4K
            else if (resolutionModifier >= 1.3)
                videoParameters.blurPasses = 3;        // 1440
            else if (resolutionModifier >= 1)
                videoParameters.blurPasses = 2;        // 1080
            // clang-format on
        }

        if (shaders != 0)
            mRenderer->shaderPostprocessing(shaders, videoParameters);
    }
    else if (mImages[mCurrentImageIndex]->hasImage() &&
             mImages[mCurrentImageIndex]->getSize() != glm::vec2 {0.0f, 0.0f}) {
        mImages[mCurrentImageIndex]->render(trans);

        if (mCurrentImageIndex == mScreenshotIndex &&
            Settings::getInstance()->getBool("MediaViewerScreenshotScanlines"))
            mRenderer->shaderPostprocessing(Renderer::Shader::SCANLINES);
        else if (mCurrentImageIndex == mTitleScreenIndex &&
                 Settings::getInstance()->getBool("MediaViewerScreenshotScanlines"))
            mRenderer->shaderPostprocessing(Renderer::Shader::SCANLINES);

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
    loadImages();

    if (!mHasVideo && !mHasImages)
        return;

    if (mHasVideo)
        playVideo();
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
        mScreenshotIndex = 0;
    }

    if ((mediaFile = mGame->getCoverPath()) != "")
        mImageFiles.push_back(mediaFile);

    if ((mediaFile = mGame->getBackCoverPath()) != "")
        mImageFiles.push_back(mediaFile);

    if ((mediaFile = mGame->getTitleScreenPath()) != "") {
        mImageFiles.push_back(mediaFile);
        mTitleScreenIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if (mHasVideo && (mediaFile = mGame->getScreenshotPath()) != "") {
        mImageFiles.push_back(mediaFile);
        mScreenshotIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if ((mediaFile = mGame->getFanArtPath()) != "")
        mImageFiles.push_back(mediaFile);

    if ((mediaFile = mGame->getMiximagePath()) != "")
        mImageFiles.push_back(mediaFile);

    if (!mImageFiles.empty())
        mHasImages = true;
}

void MediaViewer::loadImages()
{
    for (auto& file : mImageFiles) {
        mImages.emplace_back(std::make_unique<ImageComponent>(false, false));
        mImages.back()->setOrigin(0.5f, 0.5f);
        mImages.back()->setPosition(Renderer::getScreenWidth() / 2.0f,
                                    Renderer::getScreenHeight() / 2.0f);
        mImages.back()->setMaxSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
        mImages.back()->setImage(file);
    }
}

void MediaViewer::playVideo()
{
    if (mVideo || mVideoFile == "")
        return;

    mDisplayingImage = false;
    ViewController::getInstance()->pauseViewVideos();

    mVideo = std::make_unique<VideoFFmpegComponent>();
    mVideo->setOrigin(0.5f, 0.5f);
    mVideo->setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);

    if (Settings::getInstance()->getBool("MediaViewerStretchVideos"))
        mVideo->setResize(Renderer::getScreenWidth(), Renderer::getScreenHeight());
    else
        mVideo->setMaxSize(Renderer::getScreenWidth(), Renderer::getScreenHeight());

    mVideo->setVideo(mVideoFile);
    mVideo->setMediaViewerMode(true);
    mVideo->startVideoPlayer();
}

void MediaViewer::showNext()
{
    if (mHasImages && ((mCurrentImageIndex != static_cast<int>(mImageFiles.size()) - 1) ||
                       (!mDisplayingImage && mCurrentImageIndex == 0 && mImageFiles.size() == 1)))
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);

    bool showedVideo {false};

    if (mVideo && !mHasImages) {
        return;
    }
    else if (mVideo && !Settings::getInstance()->getBool("MediaViewerKeepVideoRunning")) {
        mVideo.reset();
        showedVideo = true;
    }

    if ((mVideo || showedVideo) && !mDisplayingImage)
        mCurrentImageIndex = 0;
    else if (static_cast<int>(mImageFiles.size()) > mCurrentImageIndex + 1)
        ++mCurrentImageIndex;

    mDisplayingImage = true;
}

void MediaViewer::showPrevious()
{
    if ((mHasVideo && mDisplayingImage) || (!mHasVideo && mCurrentImageIndex != 0))
        NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);

    if (mCurrentImageIndex == 0 && !mHasVideo) {
        return;
    }
    else if (mCurrentImageIndex == 0 && mHasVideo) {
        mDisplayingImage = false;
        playVideo();
        return;
    }

    --mCurrentImageIndex;
}

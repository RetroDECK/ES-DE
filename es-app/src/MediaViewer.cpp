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
    , mGame {nullptr}
    , mFrameHeight {0.0f}
    , mHelpInfoPosition {HelpInfoPosition::TOP}
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

    if (Settings::getInstance()->getString("MediaViewerHelpPrompts") == "disabled")
        mHelpInfoPosition = HelpInfoPosition::DISABLED;
    else if (Settings::getInstance()->getString("MediaViewerHelpPrompts") == "bottom")
        mHelpInfoPosition = HelpInfoPosition::BOTTOM;
    else
        mHelpInfoPosition = HelpInfoPosition::TOP;

    if (mHelpInfoPosition == HelpInfoPosition::DISABLED)
        mFrameHeight = 0.0f;
    else
        mFrameHeight = Font::get(FONT_SIZE_SMALL)->getLetterHeight() * 1.8f;

    mGame = game;
    mHasManual = (mGame->getManualPath() != "");

    initiateViewer();

    if (!mHasVideo && !mHasImages)
        return false;

    HelpStyle style;
    style.origin = {0.5f, 0.5f};
    style.iconColor = 0xAAAAAAFF;
    style.textColor = 0xAAAAAAFF;

    mEntryCount = std::to_string(mImages.size() + (mVideo == nullptr ? 0 : 1));

    mEntryNumText = std::make_unique<TextComponent>(
        "1/" + mEntryCount + (mHasVideo ? "   VIDEO" : mImageFiles[0].second),
        Font::get(FONT_SIZE_MINI, FONT_PATH_REGULAR), 0xAAAAAAFF);
    mEntryNumText->setOrigin(0.0f, 0.5f);

    if (mHelpInfoPosition == HelpInfoPosition::TOP) {
        mEntryNumText->setPosition(mRenderer->getScreenWidth() * 0.01f, mFrameHeight / 2.0f);
        style.position = glm::vec2 {mRenderer->getScreenWidth() / 2.0f, mFrameHeight / 2.0f};
    }
    else if (mHelpInfoPosition == HelpInfoPosition::BOTTOM) {
        mEntryNumText->setPosition(mRenderer->getScreenWidth() * 0.01f,
                                   mRenderer->getScreenHeight() - (mFrameHeight / 2.0f));
        style.position = glm::vec2 {mRenderer->getScreenWidth() / 2.0f,
                                    mRenderer->getScreenHeight() - (mFrameHeight / 2.0f)};
    }

    mHelp = std::make_unique<HelpComponent>();
    mHelp->setStyle(style);
    mHelp->setPrompts(getHelpPrompts());

    return true;
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

void MediaViewer::launchPDFViewer()
{
    if (mHasManual) {
        Window::getInstance()->stopMediaViewer();
        Window::getInstance()->startPDFViewer(mGame);
    }
}

void MediaViewer::update(int deltaTime)
{
    if (mVideo)
        mVideo->update(deltaTime);
}

void MediaViewer::render(const glm::mat4& /*parentTrans*/)
{
    const glm::mat4 trans {mRenderer->getIdentity()};
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

    if (mHelpInfoPosition != HelpInfoPosition::DISABLED) {
        // Render a dark gray frame behind the help info.
        mRenderer->setMatrix(mRenderer->getIdentity());
        mRenderer->drawRect(0.0f,
                            (mHelpInfoPosition == HelpInfoPosition::TOP ?
                                 0.0f :
                                 Renderer::getScreenHeight() - mFrameHeight),
                            Renderer::getScreenWidth(), mFrameHeight, 0x222222FF, 0x222222FF);
        mHelp->render(trans);
        mEntryNumText->render(trans);
    }
}

std::vector<HelpPrompt> MediaViewer::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("left/right", "browse"));
    if (mHasManual)
        prompts.push_back(HelpPrompt("up", "pdf manual"));
    prompts.push_back(HelpPrompt("lt", "first"));
    prompts.push_back(HelpPrompt("rt", "last"));

    return prompts;
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
        mImageFiles.push_back(std::make_pair(mediaFile, "   SCREENSHOT"));
        mScreenshotIndex = 0;
    }

    if ((mediaFile = mGame->getCoverPath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, "   BOX COVER"));

    if ((mediaFile = mGame->getBackCoverPath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, "   BOX BACK COVER"));

    if ((mediaFile = mGame->getTitleScreenPath()) != "") {
        mImageFiles.push_back(std::make_pair(mediaFile, "   TITLE SCREEN"));
        mTitleScreenIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if (mHasVideo && (mediaFile = mGame->getScreenshotPath()) != "") {
        mImageFiles.push_back(std::make_pair(mediaFile, "   SCREENSHOT"));
        mScreenshotIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if ((mediaFile = mGame->getFanArtPath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, "   FAN ART"));

    if ((mediaFile = mGame->getMiximagePath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, "   MIXIMAGE"));

    if (!mImageFiles.empty())
        mHasImages = true;
}

void MediaViewer::loadImages()
{
    for (auto& file : mImageFiles) {
        mImages.emplace_back(std::make_unique<ImageComponent>(false, false));
        mImages.back()->setOrigin(0.5f, 0.5f);
        if (mHelpInfoPosition == HelpInfoPosition::TOP) {
            mImages.back()->setPosition(Renderer::getScreenWidth() / 2.0f,
                                        (Renderer::getScreenHeight() / 2.0f) +
                                            (mFrameHeight / 2.0f));
        }
        else if (mHelpInfoPosition == HelpInfoPosition::BOTTOM) {
            mImages.back()->setPosition(Renderer::getScreenWidth() / 2.0f,
                                        (Renderer::getScreenHeight() / 2.0f) -
                                            (mFrameHeight / 2.0f));
        }
        else {
            mImages.back()->setPosition(Renderer::getScreenWidth() / 2.0f,
                                        Renderer::getScreenHeight() / 2.0f);
        }
        mImages.back()->setMaxSize(Renderer::getScreenWidth(),
                                   Renderer::getScreenHeight() - mFrameHeight);
        mImages.back()->setImage(file.first);
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
    if (mHelpInfoPosition == HelpInfoPosition::TOP) {
        mVideo->setPosition(Renderer::getScreenWidth() / 2.0f,
                            (Renderer::getScreenHeight() / 2.0f + (mFrameHeight / 2.0f)));
    }
    else if (mHelpInfoPosition == HelpInfoPosition::BOTTOM) {
        mVideo->setPosition(Renderer::getScreenWidth() / 2.0f,
                            (Renderer::getScreenHeight() / 2.0f - (mFrameHeight / 2.0f)));
    }

    else {
        mVideo->setPosition(Renderer::getScreenWidth() / 2.0f, Renderer::getScreenHeight() / 2.0f);
    }

    if (Settings::getInstance()->getBool("MediaViewerStretchVideos"))
        mVideo->setResize(Renderer::getScreenWidth(), Renderer::getScreenHeight() - mFrameHeight);
    else
        mVideo->setMaxSize(Renderer::getScreenWidth(), Renderer::getScreenHeight() - mFrameHeight);

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
    mEntryNumText->setText(std::to_string(mCurrentImageIndex + 1 + (mHasVideo ? 1 : 0)) + "/" +
                           mEntryCount + mImageFiles[mCurrentImageIndex].second);
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
        mEntryNumText->setText("1/" + mEntryCount + "   VIDEO");
        playVideo();
        return;
    }

    mEntryNumText->setText(std::to_string(mCurrentImageIndex + (mHasVideo ? 1 : 0)) + "/" +
                           mEntryCount + mImageFiles[mCurrentImageIndex - 1].second);
    --mCurrentImageIndex;
}

void MediaViewer::showFirst()
{
    if (!mHasImages)
        return;
    else if (mCurrentImageIndex == 0 && !mHasVideo)
        return;
    else if (mCurrentImageIndex == 0 && !mDisplayingImage)
        return;

    mCurrentImageIndex = 0;
    mEntryNumText->setText("1/" + mEntryCount +
                           (mHasVideo ? "   VIDEO" : mImageFiles[mCurrentImageIndex].second));

    if (mHasVideo) {
        mDisplayingImage = false;
        playVideo();
    }

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
}

void MediaViewer::showLast()
{
    if (!mHasImages)
        return;
    else if (mCurrentImageIndex == static_cast<int>(mImages.size() - 1))
        return;

    mCurrentImageIndex = static_cast<int>(mImages.size()) - 1;
    mEntryNumText->setText(mEntryCount + "/" + mEntryCount +
                           mImageFiles[mCurrentImageIndex].second);
    mDisplayingImage = true;

    if (mVideo && !Settings::getInstance()->getBool("MediaViewerKeepVideoRunning"))
        mVideo.reset();

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
}

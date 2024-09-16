//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  MediaViewer.cpp
//
//  Fullscreen game media viewer.
//

#include "MediaViewer.h"

#include "Sound.h"
#include "components/VideoFFmpegComponent.h"
#include "utils/LocalizationUtil.h"
#include "views/ViewController.h"

#define KEY_REPEAT_START_DELAY 600
#define KEY_REPEAT_SPEED 250

MediaViewer::MediaViewer()
    : mRenderer {Renderer::getInstance()}
    , mGame {nullptr}
    , mHasVideo {false}
    , mHasImages {false}
    , mDisplayingImage {false}
    , mHasManual {false}
    , mShowMediaTypes {false}
    , mFrameHeight {0.0f}
    , mCurrentImageIndex {0}
    , mScreenshotIndex {0}
    , mTitleScreenIndex {0}
    , mKeyRepeatDir {0}
    , mKeyRepeatTimer {0}
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
    mKeyRepeatDir = 0;
    mKeyRepeatTimer = 0;

    mShowMediaTypes = Settings::getInstance()->getBool("MediaViewerShowTypes");

    if (Settings::getInstance()->getString("MediaViewerHelpPrompts") == "disabled")
        mHelpInfoPosition = HelpInfoPosition::DISABLED;
    else if (Settings::getInstance()->getString("MediaViewerHelpPrompts") == "bottom")
        mHelpInfoPosition = HelpInfoPosition::BOTTOM;
    else
        mHelpInfoPosition = HelpInfoPosition::TOP;

    if (mHelpInfoPosition == HelpInfoPosition::DISABLED)
        mFrameHeight = 0.0f;
    else
        mFrameHeight = Font::get(FONT_SIZE_MINI)->getLetterHeight() * 1.9f;

    mGame = game;
    mHasManual = (mGame->getManualPath() != "");

    initiateViewer();

    if (!mHasVideo && !mHasImages)
        return false;

    ViewController::getInstance()->pauseViewVideos();
    Window::getInstance()->stopInfoPopup();

    HelpStyle style;
    style.font = Font::get(FONT_SIZE_MINI);
    style.origin = {0.5f, 0.5f};
    style.iconColor = 0xAAAAAAFF;
    style.textColor = 0xAAAAAAFF;

    mEntryCount = std::to_string(mImages.size() + (mVideo == nullptr ? 0 : 1));

    mMediaType =
        std::make_unique<TextComponent>((mHasVideo ? _("VIDEO") : mImageFiles[0].second.mediaType),
                                        Font::get(FONT_SIZE_MINI, FONT_PATH_REGULAR), 0xAAAAAAFF);
    mMediaType->setOrigin(0.0f, 0.5f);

    if (mHelpInfoPosition == HelpInfoPosition::TOP) {
        mMediaType->setPosition(mRenderer->getScreenWidth() * 0.01f, mFrameHeight / 2.0f);
        style.position = glm::vec2 {mRenderer->getScreenWidth() / 2.0f, mFrameHeight / 2.0f};
    }
    else if (mHelpInfoPosition == HelpInfoPosition::BOTTOM) {
        mMediaType->setPosition(mRenderer->getScreenWidth() * 0.01f,
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
    ViewController::getInstance()->startViewVideos();

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

void MediaViewer::input(InputConfig* config, Input input)
{
    if (config->isMappedLike("down", input) && input.value != 0) {
        mKeyRepeatDir = 0;
        return;
    }
    else if (config->isMappedLike("up", input) && input.value != 0) {
        mKeyRepeatDir = 0;
        launchPDFViewer();
        return;
    }
    else if (config->isMappedLike("left", input)) {
        if (input.value) {
            mKeyRepeatDir = -1;
            mKeyRepeatTimer = -(KEY_REPEAT_START_DELAY - KEY_REPEAT_SPEED);
            showPrevious();
        }
        else {
            mKeyRepeatDir = 0;
        }
    }
    else if (config->isMappedLike("right", input)) {
        if (input.value) {
            mKeyRepeatDir = 1;
            mKeyRepeatTimer = -(KEY_REPEAT_START_DELAY - KEY_REPEAT_SPEED);
            showNext();
        }
        else {
            mKeyRepeatDir = 0;
        }
    }
    else if (config->isMappedLike("lefttrigger", input) && input.value != 0) {
        mKeyRepeatDir = 0;
        showFirst();
    }
    else if (config->isMappedLike("righttrigger", input) && input.value != 0) {
        mKeyRepeatDir = 0;
        showLast();
    }
    else if (input.value != 0) {
        // Any other input stops the media viewer.
        Window::getInstance()->stopMediaViewer();
    }
}

void MediaViewer::update(int deltaTime)
{
    if (mKeyRepeatDir != 0) {
        mKeyRepeatTimer += deltaTime;
        while (mKeyRepeatTimer >= KEY_REPEAT_SPEED) {
            mKeyRepeatTimer -= KEY_REPEAT_SPEED;
            if (mKeyRepeatDir == 1)
                showNext();
            else
                showPrevious();
        }
    }

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
            if (mRenderer->getScreenRotation() == 90 || mRenderer->getScreenRotation() == 270)
                shaders |= Renderer::Shader::BLUR_VERTICAL;
            else
                shaders |= Renderer::Shader::BLUR_HORIZONTAL;
        }

        // We run two passes to make the blur smoother.
        videoParameters.blurPasses = 2;
        videoParameters.blurStrength = 1.35f;

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
        if (mShowMediaTypes)
            mMediaType->render(trans);
    }
}

std::vector<HelpPrompt> MediaViewer::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    prompts.push_back(HelpPrompt("left/right", _("browse")));
    if (mHasManual)
        prompts.push_back(HelpPrompt("up", _("pdf manual")));
    prompts.push_back(HelpPrompt("lt", _("first")));
    prompts.push_back(HelpPrompt("rt", _("last")));

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
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("SCREENSHOT"), false)));
        mScreenshotIndex = 0;
    }

    if ((mediaFile = mGame->getCoverPath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("BOX COVER"), true)));

    if ((mediaFile = mGame->getBackCoverPath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("BOX BACK COVER"), true)));

    if ((mediaFile = mGame->getTitleScreenPath()) != "") {
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("TITLE SCREEN"), false)));
        mTitleScreenIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if (mHasVideo && (mediaFile = mGame->getScreenshotPath()) != "") {
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("SCREENSHOT"), false)));
        mScreenshotIndex = static_cast<int>(mImageFiles.size() - 1);
    }

    if ((mediaFile = mGame->getFanArtPath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("FAN ART"), true)));

    if ((mediaFile = mGame->getMiximagePath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("MIXIMAGE"), true)));

    if ((mediaFile = mGame->getCustomImagePath()) != "")
        mImageFiles.push_back(std::make_pair(mediaFile, ImageInfo(_("CUSTOM"), true)));

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
        mImages.back()->setLinearInterpolation(file.second.linearInterpolation);
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
    mMediaType->setText(mImageFiles[mCurrentImageIndex].second.mediaType);
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
        mMediaType->setText(_("VIDEO"));
        playVideo();
        return;
    }

    mMediaType->setText(mImageFiles[mCurrentImageIndex - 1].second.mediaType);
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
    mMediaType->setText(
        (mHasVideo ? _("VIDEO") : mImageFiles[mCurrentImageIndex].second.mediaType));

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
    mMediaType->setText(mImageFiles[mCurrentImageIndex].second.mediaType);
    mDisplayingImage = true;

    if (mVideo && !Settings::getInstance()->getBool("MediaViewerKeepVideoRunning"))
        mVideo.reset();

    NavigationSounds::getInstance().playThemeNavigationSound(SCROLLSOUND);
}

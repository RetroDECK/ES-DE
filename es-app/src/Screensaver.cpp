//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Screensaver.cpp
//
//  Screensaver, supporting the following types:
//  Dim, black, slideshow, video.
//

#include "Screensaver.h"

#include "FileData.h"
#include "Log.h"
#include "SystemData.h"
#include "UIModeController.h"
#include "components/VideoFFmpegComponent.h"
#include "resources/Font.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/GamelistView.h"
#include "views/ViewController.h"

#include <random>
#include <time.h>
#include <unordered_map>

#if defined(_WIN64)
#include <cstring>
#endif

#define FADE_TIME 300.0f

Screensaver::Screensaver()
    : mRenderer {Renderer::getInstance()}
    , mWindow {Window::getInstance()}
    , mState {STATE_INACTIVE}
    , mImageScreensaver {nullptr}
    , mVideoScreensaver {nullptr}
    , mCurrentGame {nullptr}
    , mPreviousGame {nullptr}
    , mTimer {0}
    , mMediaSwapTime {0}
    , mTriggerNextGame {false}
    , mHasMediaFiles {false}
    , mFallbackScreensaver {false}
    , mOpacity {0.0f}
    , mDimValue {1.0}
    , mRectangleFadeIn {50}
    , mTextFadeIn {0}
    , mSaturationAmount {1.0}
{
    mWindow->setScreensaver(this);
}

void Screensaver::startScreensaver(bool generateMediaList)
{
    ViewController::getInstance()->pauseViewVideos();

    mScreensaverType = Settings::getInstance()->getString("ScreensaverType");
    // In case there is an invalid entry in the es_settings.xml file.
    if (mScreensaverType != "dim" && mScreensaverType != "black" &&
        mScreensaverType != "slideshow" && mScreensaverType != "video") {
        mScreensaverType = "dim";
    }
    std::string path;
    mHasMediaFiles = false;
    mFallbackScreensaver = false;
    mOpacity = 0.0f;

    // Keep a reference to the default fonts, so they don't keep getting destroyed/recreated.
    if (mGameOverlayFont.empty()) {
        mGameOverlayFont.push_back(Font::get(FONT_SIZE_SMALL));
        mGameOverlayFont.push_back(Font::get(FONT_SIZE_MEDIUM));
        mGameOverlayFont.push_back(Font::get(FONT_SIZE_LARGE));
    }

    // Set mPreviousGame which will be used to avoid showing the same game again during
    // the random selection.
    if ((mScreensaverType == "slideshow" || mScreensaverType == "video") && mCurrentGame != nullptr)
        mPreviousGame = mCurrentGame;

    if (mScreensaverType == "slideshow") {
        if (generateMediaList) {
            mImageFiles.clear();
            mImageCustomFiles.clear();
        }

        // This creates a fade transition between the images.
        mState = STATE_FADE_OUT_WINDOW;

        mMediaSwapTime = Settings::getInstance()->getInt("ScreensaverSwapImageTimeout");

        // Load a random image.
        if (Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages")) {
            if (generateMediaList)
                generateCustomImageList();
            pickRandomCustomImage(path);

            if (mImageCustomFiles.size() > 0)
                mHasMediaFiles = true;
            // Custom images are not tied to the game list.
            mCurrentGame = nullptr;
        }
        else {
            if (generateMediaList)
                generateImageList();
            pickRandomImage(path);
        }

        if (mImageFiles.size() > 0)
            mHasMediaFiles = true;

        // Don't attempt to render the screensaver if there are no images available, but
        // do flag it as running. This way render() will fade to a black screen, i.e. it
        // will activate the 'Black' screensaver type.
        if (mImageFiles.size() > 0 || mImageCustomFiles.size() > 0) {
            if (Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo"))
                generateOverlayInfo();

            if (!mImageScreensaver)
                mImageScreensaver = std::make_unique<ImageComponent>(false, false);

            mTimer = 0;

            mImageScreensaver->setImage(path);
            mImageScreensaver->setOrigin(0.5f, 0.5f);
            mImageScreensaver->setPosition(Renderer::getScreenWidth() / 2.0f,
                                           Renderer::getScreenHeight() / 2.0f);

            if (Settings::getInstance()->getBool("ScreensaverStretchImages"))
                mImageScreensaver->setResize(Renderer::getScreenWidth(),
                                             Renderer::getScreenHeight());
            else
                mImageScreensaver->setMaxSize(Renderer::getScreenWidth(),
                                              Renderer::getScreenHeight());
        }
        mTimer = 0;
        return;
    }
    else if (!mVideoScreensaver && (mScreensaverType == "video")) {
        if (generateMediaList)
            mVideoFiles.clear();

        // This creates a fade transition between the videos.
        mState = STATE_FADE_OUT_WINDOW;

        mMediaSwapTime = Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout");

        // Load a random video.
        if (generateMediaList)
            generateVideoList();
        pickRandomVideo(path);

        if (mVideoFiles.size() > 0)
            mHasMediaFiles = true;

        if (!path.empty() && Utils::FileSystem::exists(path)) {
            if (Settings::getInstance()->getBool("ScreensaverVideoGameInfo"))
                generateOverlayInfo();

            mVideoScreensaver = std::make_unique<VideoFFmpegComponent>();
            mVideoScreensaver->setOrigin(0.5f, 0.5f);
            mVideoScreensaver->setPosition(Renderer::getScreenWidth() / 2.0f,
                                           Renderer::getScreenHeight() / 2.0f);

            if (Settings::getInstance()->getBool("ScreensaverStretchVideos"))
                mVideoScreensaver->setResize(Renderer::getScreenWidth(),
                                             Renderer::getScreenHeight());
            else
                mVideoScreensaver->setMaxSize(Renderer::getScreenWidth(),
                                              Renderer::getScreenHeight());

            mVideoScreensaver->setVideo(path);
            mVideoScreensaver->setScreensaverMode(true);
            mVideoScreensaver->startVideoPlayer();
            mTimer = 0;
            return;
        }
    }
    // No videos or images, just use a standard screensaver.
    mState = STATE_SCREENSAVER_ACTIVE;
    mCurrentGame = nullptr;
}

void Screensaver::stopScreensaver()
{
    mImageScreensaver.reset();
    mVideoScreensaver.reset();

    mState = STATE_INACTIVE;
    mDimValue = 1.0f;
    mRectangleFadeIn = 50;
    mTextFadeIn = 0;
    mSaturationAmount = 1.0f;

    if (mGameOverlay)
        mGameOverlay.reset();

    ViewController::getInstance()->startViewVideos();
}

void Screensaver::nextGame()
{
    stopScreensaver();
    startScreensaver(false);
}

void Screensaver::launchGame()
{
    if (mCurrentGame != nullptr) {
        // If the game is inside a folder where a folder link entry is present, then jump to
        // that folder instead of to the actual game file. Also check the complete hierarchy in
        // case folder link entries are set on multiple levels.
        FileData* entry {mCurrentGame};
        FileData* selectGame {mCurrentGame};
        FileData* launchFolder {nullptr};

        while (entry != nullptr) {
            entry = entry->getParent();
            if (entry != nullptr && entry->metadata.get("folderlink") != "")
                launchFolder = entry;
        }

        if (launchFolder != nullptr)
            selectGame = launchFolder;

        // Launching game
        ViewController::getInstance()->triggerGameLaunch(mCurrentGame);
        ViewController::getInstance()->goToGamelist(mCurrentGame->getSystem());
        GamelistView* view {
            ViewController::getInstance()->getGamelistView(mCurrentGame->getSystem()).get()};
        view->setCursor(selectGame);
        view->stopListScrolling();
        ViewController::getInstance()->cancelViewTransitions();
        ViewController::getInstance()->pauseViewVideos();
    }
}

void Screensaver::goToGame()
{
    if (mCurrentGame != nullptr) {
        FileData* entry {mCurrentGame};
        FileData* launchFolder {nullptr};

        while (entry != nullptr) {
            entry = entry->getParent();
            if (entry != nullptr && entry->metadata.get("folderlink") != "")
                launchFolder = entry;
        }

        if (launchFolder != nullptr)
            mCurrentGame = launchFolder;

        // Go to the game in the gamelist view, but don't launch it.
        ViewController::getInstance()->goToGamelist(mCurrentGame->getSystem());
        GamelistView* view {
            ViewController::getInstance()->getGamelistView(mCurrentGame->getSystem()).get()};
        view->setCursor(mCurrentGame);
        view->stopListScrolling();
        ViewController::getInstance()->cancelViewTransitions();
    }
}

void Screensaver::renderScreensaver()
{
    glm::mat4 trans {Renderer::getIdentity()};
    mRenderer->setMatrix(trans);

    if (mVideoScreensaver && mScreensaverType == "video") {
        // Render a black background below the video.
        mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                            0x000000FF, 0x000000FF);

        // Only render the video if the state requires it.
        if (static_cast<int>(mState) >= STATE_FADE_IN_VIDEO)
            mVideoScreensaver->render(trans);
    }
    else if (mImageScreensaver && mScreensaverType == "slideshow") {
        // Render a black background below the image.
        mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                            0x000000FF, 0x000000FF);

        // Only render the image if the state requires it.
        if (static_cast<int>(mState) >= STATE_FADE_IN_VIDEO) {
            if (mImageScreensaver->hasImage()) {
                mImageScreensaver->setOpacity(1.0f - mOpacity);
                mImageScreensaver->render(trans);
            }
        }
    }

    if (isScreensaverActive()) {
        if (mScreensaverType == "slideshow") {
            if (mHasMediaFiles) {
                if (Settings::getInstance()->getBool("ScreensaverSlideshowScanlines"))
                    mRenderer->shaderPostprocessing(Renderer::Shader::SCANLINES);
                if (Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo") &&
                    mGameOverlay) {
                    mRenderer->setMatrix(mRenderer->getIdentity());
                    if (mGameOverlayRectangleCoords.size() == 4) {
                        mRenderer->drawRect(
                            mGameOverlayRectangleCoords[0], mGameOverlayRectangleCoords[1],
                            mGameOverlayRectangleCoords[2], mGameOverlayRectangleCoords[3],
                            0x00000000 | mRectangleFadeIn, 0x00000000 | mRectangleFadeIn);
                    }
                    mRectangleFadeIn =
                        glm::clamp(mRectangleFadeIn + 6 + mRectangleFadeIn / 20, 0, 170);

                    mGameOverlay.get()->setColor(0xFFFFFF00 | mTextFadeIn);
                    if (mTextFadeIn > 50)
                        mGameOverlayFont.at(0)->renderTextCache(mGameOverlay.get());
                    if (mTextFadeIn < 255)
                        mTextFadeIn = glm::clamp(mTextFadeIn + 2 + mTextFadeIn / 6, 0, 255);
                }
            }
            else {
                mFallbackScreensaver = true;
            }
        }
        else if (mScreensaverType == "video") {
            if (mHasMediaFiles) {
                Renderer::postProcessingParams videoParameters;
                unsigned int shaders {0};
                if (Settings::getInstance()->getBool("ScreensaverVideoScanlines"))
                    shaders = Renderer::Shader::SCANLINES;
                if (Settings::getInstance()->getBool("ScreensaverVideoBlur")) {
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

                if (Settings::getInstance()->getBool("ScreensaverVideoGameInfo") && mGameOverlay) {
                    mRenderer->setMatrix(mRenderer->getIdentity());
                    if (mGameOverlayRectangleCoords.size() == 4) {
                        mRenderer->drawRect(
                            mGameOverlayRectangleCoords[0], mGameOverlayRectangleCoords[1],
                            mGameOverlayRectangleCoords[2], mGameOverlayRectangleCoords[3],
                            0x00000000 | mRectangleFadeIn, 0x00000000 | mRectangleFadeIn);
                    }
                    mRectangleFadeIn =
                        glm::clamp(mRectangleFadeIn + 6 + mRectangleFadeIn / 20, 0, 170);

                    mGameOverlay.get()->setColor(0xFFFFFF00 | mTextFadeIn);
                    if (mTextFadeIn > 50)
                        mGameOverlayFont.at(0)->renderTextCache(mGameOverlay.get());
                    if (mTextFadeIn < 255)
                        mTextFadeIn = glm::clamp(mTextFadeIn + 2 + mTextFadeIn / 6, 0, 255);
                }
            }
            else {
                mFallbackScreensaver = true;
            }
        }
        if (mFallbackScreensaver || mScreensaverType == "dim") {
            Renderer::postProcessingParams dimParameters;
            dimParameters.dimming = mDimValue;
            dimParameters.saturation = mSaturationAmount;
            mRenderer->shaderPostprocessing(Renderer::Shader::CORE, dimParameters);
            if (mDimValue > 0.4)
                mDimValue = glm::clamp(mDimValue - 0.021f, 0.4f, 1.0f);
            if (mSaturationAmount > 0.0)
                mSaturationAmount = glm::clamp(mSaturationAmount - 0.035f, 0.0f, 1.0f);
        }
        else if (mScreensaverType == "black") {
            Renderer::postProcessingParams blackParameters;
            blackParameters.dimming = mDimValue;
            mRenderer->shaderPostprocessing(Renderer::Shader::CORE, blackParameters);
            if (mDimValue > 0.0)
                mDimValue = glm::clamp(mDimValue - 0.045f, 0.0f, 1.0f);
        }
    }
}

void Screensaver::update(int deltaTime)
{
    // Use this to update the fade value for the current fade stage.
    if (mState == STATE_FADE_OUT_WINDOW) {
        mOpacity += static_cast<float>(deltaTime) / FADE_TIME;
        if (mOpacity >= 1.0f) {
            mOpacity = 1.0f;

            // Update to the next state.
            mState = STATE_FADE_IN_VIDEO;
        }
    }
    else if (mState == STATE_FADE_IN_VIDEO) {
        mOpacity -= static_cast<float>(deltaTime) / FADE_TIME;
        if (mOpacity <= 0.0f) {
            mOpacity = 0.0f;
            // Update to the next state.
            mState = STATE_SCREENSAVER_ACTIVE;
        }
    }
    else if (mState == STATE_SCREENSAVER_ACTIVE) {
        // Update the timer that swaps the media, unless the swap time is set to 0 (only
        // applicable for the video screensaver). This means that videos play to the end,
        // at which point the video player will trigger a skip to the next game.
        if (mMediaSwapTime != 0) {
            mTimer += deltaTime;
            if (mTimer > mMediaSwapTime)
                nextGame();
        }
        if (mTriggerNextGame) {
            mTriggerNextGame = false;
            nextGame();
        }
    }

    // If we have a loaded a video or image, then update it.
    if (mVideoScreensaver)
        mVideoScreensaver->update(deltaTime);
    if (mImageScreensaver)
        mImageScreensaver->update(deltaTime);
}

void Screensaver::generateImageList()
{
    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        // We only want nodes from game systems that are not collections.
        if (!(*it)->isGameSystem() || (*it)->isCollection())
            continue;

        std::vector<FileData*> allFiles {(*it)->getRootFolder()->getFilesRecursive(GAME, true)};
        for (auto it2 = allFiles.cbegin(); it2 != allFiles.cend(); ++it2) {
            // Only include games suitable for children if we're in Kid UI mode.
            if (UIModeController::getInstance()->isUIModeKid() &&
                (*it2)->metadata.get("kidgame") != "true")
                continue;
            std::string imagePath {(*it2)->getImagePath()};
            if (imagePath != "")
                mImageFiles.push_back((*it2));
        }
    }
}

void Screensaver::generateVideoList()
{
    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        // We only want nodes from game systems that are not collections.
        if (!(*it)->isGameSystem() || (*it)->isCollection())
            continue;

        std::vector<FileData*> allFiles {(*it)->getRootFolder()->getFilesRecursive(GAME, true)};
        for (auto it2 = allFiles.cbegin(); it2 != allFiles.cend(); ++it2) {
            // Only include games suitable for children if we're in Kid UI mode.
            if (UIModeController::getInstance()->isUIModeKid() &&
                (*it2)->metadata.get("kidgame") != "true")
                continue;
            std::string videoPath {(*it2)->getVideoPath()};
            if (videoPath != "")
                mVideoFiles.push_back((*it2));
        }
    }
}

void Screensaver::generateCustomImageList()
{
    std::string imageDir = Utils::FileSystem::expandHomePath(
        Settings::getInstance()->getString("ScreensaverSlideshowImageDir"));

    // This makes it possible to set the custom image directory relative to the ES-DE binary
    // directory or the ROM directory.
    imageDir = Utils::String::replace(imageDir, "%ESPATH%", Utils::FileSystem::getExePath());
    imageDir = Utils::String::replace(imageDir, "%ROMPATH%", FileData::getROMDirectory());

    if (imageDir != "" && Utils::FileSystem::isDirectory(imageDir)) {
        std::string imageFilter {".jpg, .JPG, .png, .PNG"};
        Utils::FileSystem::StringList dirContent = Utils::FileSystem::getDirContent(
            imageDir, Settings::getInstance()->getBool("ScreensaverSlideshowRecurse"));

        for (auto it = dirContent.begin(); it != dirContent.end(); ++it) {
            if (Utils::FileSystem::isRegularFile(*it)) {
                if (imageFilter.find(Utils::FileSystem::getExtension(*it)) != std::string::npos)
                    mImageCustomFiles.push_back(*it);
            }
        }
    }
    else {
        LOG(LogWarning) << "Custom screensaver image directory '" << imageDir << "' does not exist";
    }
}

void Screensaver::pickRandomImage(std::string& path)
{
    mCurrentGame = nullptr;

    if (mImageFiles.size() == 0)
        return;

    if (mImageFiles.size() == 1) {
        mPreviousGame = nullptr;
        mCurrentGame = mImageFiles.front();
        path = mImageFiles.front()->getImagePath();
        mGameName = mImageFiles.front()->getName();
        mSystemName = mImageFiles.front()->getSystem()->getFullName();
        mCurrentGame = mImageFiles.front();
        return;
    }

    unsigned int index;
    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine {randDev()};
        std::uniform_int_distribution<int> uniform_dist {0,
                                                         static_cast<int>(mImageFiles.size()) - 1};
        index = uniform_dist(engine);
    } while (mPreviousGame && mImageFiles.at(index) == mPreviousGame);

    path = mImageFiles.at(index)->getImagePath();
    mGameName = mImageFiles.at(index)->getName();
    mSystemName = mImageFiles.at(index)->getSystem()->getFullName();
    mCurrentGame = mImageFiles.at(index);
}

void Screensaver::pickRandomVideo(std::string& path)
{
    mCurrentGame = nullptr;

    if (mVideoFiles.size() == 0)
        return;

    if (mVideoFiles.size() == 1) {
        mPreviousGame = nullptr;
        mCurrentGame = mVideoFiles.front();
        path = mVideoFiles.front()->getVideoPath();
        mGameName = mVideoFiles.front()->getName();
        mSystemName = mVideoFiles.front()->getSystem()->getFullName();
        mCurrentGame = mVideoFiles.front();
        return;
    }

    unsigned int index;
    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine {randDev()};
        std::uniform_int_distribution<int> uniform_dist {0,
                                                         static_cast<int>(mVideoFiles.size()) - 1};
        index = uniform_dist(engine);
    } while (mPreviousGame && mVideoFiles.at(index) == mPreviousGame);

    path = mVideoFiles.at(index)->getVideoPath();
    mGameName = mVideoFiles.at(index)->getName();
    mSystemName = mVideoFiles.at(index)->getSystem()->getFullName();
    mCurrentGame = mVideoFiles.at(index);
}

void Screensaver::pickRandomCustomImage(std::string& path)
{
    if (mImageCustomFiles.size() == 0)
        return;

    if (mImageCustomFiles.size() == 1) {
        mPreviousCustomImage = mImageCustomFiles.front();
        path = mImageCustomFiles.front();
        return;
    }

    unsigned int index;
    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine {randDev()};
        std::uniform_int_distribution<int> uniform_dist {
            0, static_cast<int>(mImageCustomFiles.size()) - 1};
        index = uniform_dist(engine);
    } while (mPreviousCustomImage != "" && mImageCustomFiles.at(index) == mPreviousCustomImage);

    path = mImageCustomFiles.at(index);
    mPreviousCustomImage = path;
    mGameName = "";
    mSystemName = "";
}

void Screensaver::generateOverlayInfo()
{
    if (mGameName == "" || mSystemName == "")
        return;

    float posX {mRenderer->getScreenWidth() * 0.023f};
    float posY {mRenderer->getScreenHeight() * 0.02f};

    std::string favoriteChar;
    if (mCurrentGame && mCurrentGame->getFavorite())
        favoriteChar.append("  ").append(ViewController::FAVORITE_CHAR);

    const std::string gameName {Utils::String::toUpper(mGameName) + favoriteChar};
    const std::string systemName {Utils::String::toUpper(mSystemName)};
    const std::string overlayText {gameName + "\n" + systemName};

    mGameOverlay = std::unique_ptr<TextCache>(
        mGameOverlayFont.at(0)->buildTextCache(overlayText, posX, posY, 0xFFFFFFFF));

    float textSizeX {0.0f};
    float textSizeY {mGameOverlayFont[0].get()->sizeText(overlayText).y};

    // There is a weird issue with sizeText() where the X size value is returned
    // as too large if there are two rows in a string and the second row is longer
    // than the first row. Possibly it's the newline character that is somehow
    // injected in the size calculation. Regardless, this workaround is working
    // fine for the time being.
    if (mGameOverlayFont[0].get()->sizeText(gameName).x >
        mGameOverlayFont[0].get()->sizeText(systemName).x)
        textSizeX = mGameOverlayFont[0].get()->sizeText(gameName).x;
    else
        textSizeX = mGameOverlayFont[0].get()->sizeText(systemName).x;

    float marginX {mRenderer->getScreenWidth() * 0.01f};

    mGameOverlayRectangleCoords.clear();
    mGameOverlayRectangleCoords.push_back(posX - marginX);
    mGameOverlayRectangleCoords.push_back(posY);
    mGameOverlayRectangleCoords.push_back(textSizeX + marginX * 2.0f);
    mGameOverlayRectangleCoords.push_back(textSizeY);
}

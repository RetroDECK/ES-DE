//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
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

#if defined(_WIN64)
#include <cstring>
#endif

#define IMAGES_FADE_IN_TIME 450.0f

Screensaver::Screensaver()
    : mRenderer {Renderer::getInstance()}
    , mWindow {Window::getInstance()}
    , mImageScreensaver {nullptr}
    , mVideoScreensaver {nullptr}
    , mCurrentGame {nullptr}
    , mPreviousGame {nullptr}
    , mTimer {0}
    , mMediaSwapTime {0}
    , mScreensaverActive {false}
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
    mGameOverlay = std::make_unique<TextComponent>("", Font::get(FONT_SIZE_SMALL), 0xFFFFFFFF,
                                                   ALIGN_LEFT, ALIGN_CENTER, glm::ivec2 {1, 1});

    mScreensaverType = Settings::getInstance()->getString("ScreensaverType");
    // In case there is an invalid entry in the es_settings.xml file.
    if (mScreensaverType != "dim" && mScreensaverType != "black" &&
        mScreensaverType != "slideshow" && mScreensaverType != "video") {
        mScreensaverType = "dim";
    }
    std::string path;
    mScreensaverActive = true;
    mHasMediaFiles = false;
    mFallbackScreensaver = false;
    mOpacity = 0.0f;

    // Set mPreviousGame which will be used to avoid showing the same game again during
    // the random selection.
    if ((mScreensaverType == "slideshow" || mScreensaverType == "video") && mCurrentGame != nullptr)
        mPreviousGame = mCurrentGame;

    if (mScreensaverType == "slideshow") {
        if (generateMediaList) {
            mImageFiles.clear();
            mFilesInventory.clear();
            mImageCustomFiles.clear();
            mCustomFilesInventory.clear();
        }

        mMediaSwapTime = Settings::getInstance()->getInt("ScreensaverSwapImageTimeout");

        // Load a random image.
        if (Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages")) {
            if (generateMediaList)
                generateCustomImageList();
            pickRandomCustomImage(path);

            // We've cycled through all games, so start from the beginning again.
            if (mImageCustomFiles.size() == 0 && mCustomFilesInventory.size() > 0)
                mImageCustomFiles.insert(mImageCustomFiles.begin(), mCustomFilesInventory.begin(),
                                         mCustomFilesInventory.end());

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

        // We've cycled through all games, so start from the beginning again.
        if (mImageFiles.size() == 0 && mFilesInventory.size() > 0)
            mImageFiles.insert(mImageFiles.begin(), mFilesInventory.begin(), mFilesInventory.end());

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
        if (generateMediaList) {
            mVideoFiles.clear();
            mFilesInventory.clear();
        }

        mMediaSwapTime = Settings::getInstance()->getInt("ScreensaverSwapVideoTimeout");

        // Load a random video.
        if (generateMediaList)
            generateVideoList();
        pickRandomVideo(path);

        // We've cycled through all games, so start from the beginning again.
        if (mVideoFiles.size() == 0 && mFilesInventory.size() > 0)
            mVideoFiles.insert(mVideoFiles.begin(), mFilesInventory.begin(), mFilesInventory.end());

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
    mCurrentGame = nullptr;
}

void Screensaver::stopScreensaver()
{
    mImageScreensaver.reset();
    mVideoScreensaver.reset();
    mGameOverlay.reset();

    mScreensaverActive = false;
    mDimValue = 1.0f;
    mRectangleFadeIn = 50;
    mTextFadeIn = 0;
    mSaturationAmount = 1.0f;

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
        mVideoScreensaver->render(trans);
    }
    else if (mImageScreensaver && mScreensaverType == "slideshow") {
        // Render a black background below the image.
        mRenderer->drawRect(0.0f, 0.0f, Renderer::getScreenWidth(), Renderer::getScreenHeight(),
                            0x000000FF, 0x000000FF);
        // Leave a small gap without rendering during fade-in.
        if (mOpacity > 0.5f) {
            mImageScreensaver->setOpacity(mOpacity);
            mImageScreensaver->render(trans);
        }
    }

    if (mScreensaverType == "slideshow") {
        if (mHasMediaFiles) {
            if (Settings::getInstance()->getBool("ScreensaverSlideshowScanlines"))
                mRenderer->shaderPostprocessing(Renderer::Shader::SCANLINES);
            if (Settings::getInstance()->getBool("ScreensaverSlideshowGameInfo") &&
                !Settings::getInstance()->getBool("ScreensaverSlideshowCustomImages")) {
                mRenderer->setMatrix(mRenderer->getIdentity());
                if (mGameOverlayRectangleCoords.size() == 4) {
                    mRenderer->drawRect(
                        mGameOverlayRectangleCoords[0], mGameOverlayRectangleCoords[1],
                        mGameOverlayRectangleCoords[2], mGameOverlayRectangleCoords[3],
                        0x00000000 | mRectangleFadeIn, 0x00000000 | mRectangleFadeIn);
                }
                mRectangleFadeIn = glm::clamp(mRectangleFadeIn + 6 + mRectangleFadeIn / 20, 0, 170);

                mGameOverlay.get()->setColor(0xFFFFFF00 | mTextFadeIn);
                if (mTextFadeIn > 50)
                    mGameOverlay->render(trans);
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

            if (Settings::getInstance()->getBool("ScreensaverVideoGameInfo")) {
                mRenderer->setMatrix(mRenderer->getIdentity());
                if (mGameOverlayRectangleCoords.size() == 4) {
                    mRenderer->drawRect(
                        mGameOverlayRectangleCoords[0], mGameOverlayRectangleCoords[1],
                        mGameOverlayRectangleCoords[2], mGameOverlayRectangleCoords[3],
                        0x00000000 | mRectangleFadeIn, 0x00000000 | mRectangleFadeIn);
                }
                mRectangleFadeIn = glm::clamp(mRectangleFadeIn + 6 + mRectangleFadeIn / 20, 0, 170);

                mGameOverlay.get()->setColor(0xFFFFFF00 | mTextFadeIn);
                if (mTextFadeIn > 50)
                    mGameOverlay->render(trans);
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

void Screensaver::update(int deltaTime)
{
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

    // Fade-in for the video screensaver is handled in VideoComponent.
    if (mImageScreensaver && mOpacity < 1.0f) {
        mOpacity += static_cast<float>(deltaTime) / IMAGES_FADE_IN_TIME;
        if (mOpacity > 1.0f)
            mOpacity = 1.0f;
    }

    if (mVideoScreensaver)
        mVideoScreensaver->update(deltaTime);
}

void Screensaver::generateImageList()
{
    const bool favoritesOnly {
        Settings::getInstance()->getBool("ScreensaverSlideshowOnlyFavorites")};

    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        // We only want nodes from game systems that are not collections.
        if (!(*it)->isGameSystem() || (*it)->isCollection())
            continue;

#if defined(_WIN64)
        const std::string mediaBaseDir {
            Utils::String::replace(FileData::getMediaDirectory(), "\\", "/")};
#else
        const std::string mediaBaseDir {FileData::getMediaDirectory()};
#endif
        const std::string mediaDirMiximages {
            mediaBaseDir + (*it)->getRootFolder()->getSystemName() + "/miximages"};
        const std::string mediaDirScreenshots {
            mediaBaseDir + (*it)->getRootFolder()->getSystemName() + "/screenshots"};
        const std::string mediaDirTitlescreens {
            mediaBaseDir + (*it)->getRootFolder()->getSystemName() + "/titlescreens"};
        const std::string mediaDirCovers {mediaBaseDir + (*it)->getRootFolder()->getSystemName() +
                                          "/covers"};

        Utils::FileSystem::StringList dirContentMiximages;
        Utils::FileSystem::StringList dirContentScreenshots;
        Utils::FileSystem::StringList dirContentTitlescreens;
        Utils::FileSystem::StringList dirContentCovers;

        // This method of building an inventory of all image files isn't pretty, but to use the
        // FileData::getImagePath() function leads to unacceptable performance issues on some
        // platforms like Android that offer very poor disk I/O performance. To instead list
        // all files recursively is much faster as this avoids stat() function calls which are
        // very expensive on such problematic platforms.

#if defined(_WIN64) || defined(__APPLE__) || defined(__ANDROID__)
        // Although macOS may have filesystem case-sensitivity enabled it's rare and the impact
        // would not be severe in this case anyway.
        const bool caseSensitiveFilesystem {false};
#else
        const bool caseSensitiveFilesystem {true};
#endif

        for (auto& entry : Utils::FileSystem::getDirContent(mediaDirMiximages, true)) {
            if (caseSensitiveFilesystem)
                dirContentMiximages.emplace_back(entry);
            else
                dirContentMiximages.emplace_back(Utils::String::toLower(entry));
        }

        for (auto& entry : Utils::FileSystem::getDirContent(mediaDirScreenshots, true)) {
            if (caseSensitiveFilesystem)
                dirContentScreenshots.emplace_back(entry);
            else
                dirContentScreenshots.emplace_back(Utils::String::toLower(entry));
        }

        for (auto& entry : Utils::FileSystem::getDirContent(mediaDirTitlescreens, true)) {
            if (caseSensitiveFilesystem)
                dirContentTitlescreens.emplace_back(entry);
            else
                dirContentTitlescreens.emplace_back(Utils::String::toLower(entry));
        }

        for (auto& entry : Utils::FileSystem::getDirContent(mediaDirCovers, true)) {
            if (caseSensitiveFilesystem)
                dirContentCovers.emplace_back(entry);
            else
                dirContentCovers.emplace_back(Utils::String::toLower(entry));
        }

        std::string subFolders;

        std::vector<FileData*> allFiles {(*it)->getRootFolder()->getFilesRecursive(GAME, true)};
        for (auto it2 = allFiles.cbegin(); it2 != allFiles.cend(); ++it2) {
            // Only include games suitable for children if we're in Kid UI mode.
            if (UIModeController::getInstance()->isUIModeKid() &&
                (*it2)->metadata.get("kidgame") != "true")
                continue;
            if (favoritesOnly && (*it2)->metadata.get("favorite") != "true")
                continue;

            subFolders = Utils::String::replace(Utils::FileSystem::getParent((*it2)->getPath()),
                                                (*it)->getStartPath(), "");
            const std::string gamePath {subFolders + "/" + (*it2)->getDisplayName()};

            for (auto& extension : FileData::sImageExtensions) {
                if (std::find(
                        dirContentMiximages.cbegin(), dirContentMiximages.cend(),
                        (caseSensitiveFilesystem ?
                             mediaDirMiximages + gamePath + extension :
                             Utils::String::toLower(mediaDirMiximages + gamePath + extension))) !=
                    dirContentMiximages.cend()) {
                    mImageFiles.push_back((*it2));
                    break;
                }
                if (std::find(
                        dirContentScreenshots.cbegin(), dirContentScreenshots.cend(),
                        (caseSensitiveFilesystem ?
                             mediaDirScreenshots + gamePath + extension :
                             Utils::String::toLower(mediaDirScreenshots + gamePath + extension))) !=
                    dirContentScreenshots.cend()) {
                    mImageFiles.push_back((*it2));
                    break;
                }
                if (std::find(dirContentTitlescreens.cbegin(), dirContentTitlescreens.cend(),
                              (caseSensitiveFilesystem ?
                                   mediaDirTitlescreens + gamePath + extension :
                                   Utils::String::toLower(mediaDirTitlescreens + gamePath +
                                                          extension))) !=
                    dirContentTitlescreens.cend()) {
                    mImageFiles.push_back((*it2));
                    break;
                }
                if (std::find(dirContentCovers.cbegin(), dirContentCovers.cend(),
                              (caseSensitiveFilesystem ?
                                   mediaDirCovers + gamePath + extension :
                                   Utils::String::toLower(mediaDirCovers + gamePath +
                                                          extension))) != dirContentCovers.cend()) {
                    mImageFiles.push_back((*it2));
                    break;
                }
            }
        }
    }

    mFilesInventory.insert(mFilesInventory.begin(), mImageFiles.begin(), mImageFiles.end());
}

void Screensaver::generateVideoList()
{
    const bool favoritesOnly {Settings::getInstance()->getBool("ScreensaverVideoOnlyFavorites")};

    for (auto it = SystemData::sSystemVector.cbegin(); // Line break.
         it != SystemData::sSystemVector.cend(); ++it) {
        // We only want nodes from game systems that are not collections.
        if (!(*it)->isGameSystem() || (*it)->isCollection())
            continue;

#if defined(_WIN64)
        const std::string mediaBaseDir {
            Utils::String::replace(FileData::getMediaDirectory(), "\\", "/")};
#else
        const std::string mediaBaseDir {FileData::getMediaDirectory()};
#endif
        const std::string mediaDir {mediaBaseDir + (*it)->getRootFolder()->getSystemName() +
                                    "/videos"};
        Utils::FileSystem::StringList dirContent;

        // This method of building an inventory of all video files isn't pretty, but to use the
        // FileData::getVideoPath() function leads to unacceptable performance issues on some
        // platforms like Android that offer very poor disk I/O performance. To instead list
        // all files recursively is much faster as this avoids stat() function calls which are
        // very expensive on such problematic platforms.

#if defined(_WIN64) || defined(__APPLE__) || defined(__ANDROID__)
        // Although macOS may have filesystem case-sensitivity enabled it's rare and the impact
        // would not be severe in this case anyway.
        const bool caseSensitiveFilesystem {false};
#else
        const bool caseSensitiveFilesystem {true};
#endif
        for (auto& entry : Utils::FileSystem::getDirContent(mediaDir, true)) {
            if (caseSensitiveFilesystem)
                dirContent.emplace_back(entry);
            else
                dirContent.emplace_back(Utils::String::toLower(entry));
        }

        std::string subFolders;

        std::vector<FileData*> allFiles {(*it)->getRootFolder()->getFilesRecursive(GAME, true)};
        for (auto it2 = allFiles.cbegin(); it2 != allFiles.cend(); ++it2) {
            // Only include games suitable for children if we're in Kid UI mode.
            if (UIModeController::getInstance()->isUIModeKid() &&
                (*it2)->metadata.get("kidgame") != "true")
                continue;
            if (favoritesOnly && (*it2)->metadata.get("favorite") != "true")
                continue;

            subFolders = Utils::String::replace(Utils::FileSystem::getParent((*it2)->getPath()),
                                                (*it)->getStartPath(), "");
            const std::string gamePath {subFolders + "/" + (*it2)->getDisplayName()};

            for (auto& extension : FileData::sVideoExtensions) {
                if (std::find(dirContent.cbegin(), dirContent.cend(),
                              (caseSensitiveFilesystem ?
                                   mediaDir + gamePath + extension :
                                   Utils::String::toLower(mediaDir + gamePath + extension))) !=
                    dirContent.cend()) {
                    mVideoFiles.push_back((*it2));
                    break;
                }
            }
        }
    }

    mFilesInventory.insert(mFilesInventory.begin(), mVideoFiles.begin(), mVideoFiles.end());
}

void Screensaver::generateCustomImageList()
{
    std::string imageDir {Utils::FileSystem::expandHomePath(
        Settings::getInstance()->getString("ScreensaverSlideshowCustomDir"))};

    if (imageDir.empty())
        imageDir = Utils::FileSystem::getAppDataDirectory() + "/screensavers/custom_slideshow";

    // This makes it possible to set the custom image directory relative to the ES-DE binary
    // directory or the ROM directory.
    imageDir = Utils::String::replace(imageDir, "%ESPATH%", Utils::FileSystem::getExePath());
    imageDir = Utils::String::replace(imageDir, "%ROMPATH%", FileData::getROMDirectory());

    if (imageDir != "" && Utils::FileSystem::isDirectory(imageDir)) {
        const std::vector<std::string> extList {".jpg", ".JPG",  ".png",  ".PNG", ".gif",
                                                ".GIF", ".webp", ".WEBP", ".svg", ".SVG"};

        Utils::FileSystem::StringList dirContent {Utils::FileSystem::getDirContent(
            imageDir, Settings::getInstance()->getBool("ScreensaverSlideshowRecurse"))};

        for (auto it = dirContent.begin(); it != dirContent.end(); ++it) {
            if (Utils::FileSystem::isRegularFile(*it)) {
                if (std::find(extList.cbegin(), extList.cend(),
                              Utils::FileSystem::getExtension(*it)) != extList.cend())
                    mImageCustomFiles.push_back(*it);
            }
        }
    }
    else {
        LOG(LogWarning) << "Custom screensaver image directory \"" << imageDir
                        << "\" does not exist";
    }

    mCustomFilesInventory.insert(mCustomFilesInventory.begin(), mImageCustomFiles.begin(),
                                 mImageCustomFiles.end());
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
        mImageFiles.clear();
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

    // Don't display the same image again until we've cycled through all entries.
    auto it = mImageFiles.begin() + index;
    mImageFiles.erase(it);
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
        mVideoFiles.clear();
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

    // Don't play the same video again until we've cycled through all entries.
    auto it = mVideoFiles.begin() + index;
    mVideoFiles.erase(it);
}

void Screensaver::pickRandomCustomImage(std::string& path)
{
    if (mImageCustomFiles.size() == 0)
        return;

    if (mImageCustomFiles.size() == 1) {
        mPreviousCustomImage = mImageCustomFiles.front();
        path = mImageCustomFiles.front();
        mImageCustomFiles.clear();
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

    // Don't display the same image again until we've cycled through all entries.
    auto it = mImageCustomFiles.begin() + index;
    mImageCustomFiles.erase(it);
}

void Screensaver::generateOverlayInfo()
{
    if (mGameName == "" || mSystemName == "")
        return;

    const float posX {mRenderer->getScreenWidth() * 0.023f};
    const float posY {mRenderer->getScreenHeight() * 0.02f};

    const bool favoritesOnly {
        (mScreensaverType == "video" &&
         Settings::getInstance()->getBool("ScreensaverVideoOnlyFavorites")) ||
        (mScreensaverType == "slideshow" &&
         Settings::getInstance()->getBool("ScreensaverSlideshowOnlyFavorites"))};

    std::string favoriteChar;
    // Don't add the favorites character if only displaying favorite games.
    if (!favoritesOnly && mCurrentGame && mCurrentGame->getFavorite())
        favoriteChar.append("  ").append(ViewController::FAVORITE_CHAR);

    const std::string gameName {Utils::String::toUpper(mGameName) + favoriteChar};
    const std::string systemName {Utils::String::toUpper(mSystemName)};
    const std::string overlayText {gameName + "\n" + systemName};

    mGameOverlay->setText(overlayText);
    mGameOverlay->setPosition(posX, posY);

    const float marginX {mRenderer->getScreenWidth() * 0.01f};

    mGameOverlayRectangleCoords.clear();
    mGameOverlayRectangleCoords.push_back(posX - marginX);
    mGameOverlayRectangleCoords.push_back(posY);
    mGameOverlayRectangleCoords.push_back(mGameOverlay->getSize().x + marginX * 2.0f);
    mGameOverlayRectangleCoords.push_back(mGameOverlay->getSize().y);
}

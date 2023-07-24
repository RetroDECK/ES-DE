//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiOrphanedDataCleanup.cpp
//
//  Removes orphaned game media, gamelist.xml entries and custom collections entries.
//

#include "guis/GuiOrphanedDataCleanup.h"

#include "CollectionSystemsManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "views/ViewController.h"

#include <SDL2/SDL.h>

GuiOrphanedDataCleanup::GuiOrphanedDataCleanup(std::function<void()> reloadCallback)
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {4, 11}}
    , mReloadCallback {reloadCallback}
    , mCursorPos {0}
    , mMediaDirectory {FileData::getMediaDirectory()}
    , mMediaTypes {"3dboxes",     "backcovers",   "covers",    "fanart",
                   "manuals",     "marquees",     "miximages", "physicalmedia",
                   "screenshots", "titlescreens", "videos"}
    , mIsProcessing {false}
    , mStopProcessing {false}
    , mCompleted {false}
    , mFailed {false}
    , mNeedsReloading {false}
    , mProcessedCount {0}
    , mHasCustomCollections {false}
    , mCleanupType {CleanupType::MEDIA}
{
    addChild(&mBackground);
    addChild(&mGrid);

    mMediaDescription =
        "THIS WILL REMOVE ALL MEDIA FILES WHERE NO MATCHING GAME FILES CAN BE FOUND. "
        "THESE FILES WILL BE MOVED TO A CLEANUP FOLDER INSIDE YOUR GAME MEDIA "
        "DIRECTORY. YOU CAN MANUALLY DELETE THIS FOLDER WHEN YOU ARE SURE IT'S NO "
        "LONGER NEEDED.";

    mGamelistDescription =
        "THIS WILL REMOVE ALL ENTRIES FROM YOUR GAMELIST XML FILES WHERE NO MATCHING "
        "GAME FILES CAN BE FOUND. BACKUPS OF THE ORIGINAL FILES WILL BE SAVED TO A CLEANUP FOLDER "
        "INSIDE YOUR GAMELISTS DIRECTORY. YOU CAN MANUALLY DELETE THIS FOLDER WHEN YOU ARE SURE "
        "IT'S NO LONGER NEEDED.";

    mCollectionsDescription =
        "THIS WILL REMOVE ALL ENTRIES FROM YOUR CUSTOM COLLECTIONS CONFIGURATION FILES WHERE NO "
        "MATCHING GAME FILES CAN BE FOUND. BACKUPS OF THE ORIGINAL FILES WILL BE SAVED TO A "
        "CLEANUP FOLDER INSIDE YOUR COLLECTIONS DIRECTORY. ONLY CURRENTLY ENABLED COLLECTIONS WILL "
        "BE PROCESSED.";

    // Stop any ongoing custom collections editing.
    if (CollectionSystemsManager::getInstance()->isEditing())
        CollectionSystemsManager::getInstance()->exitEditMode();

    for (auto& collection : CollectionSystemsManager::getInstance()->getCustomCollectionSystems()) {
        if (collection.second.isEnabled)
            mHasCustomCollections = true;
    }

    // Set up grid.
    mTitle = std::make_shared<TextComponent>("ORPHANED DATA CLEANUP", Font::get(FONT_SIZE_LARGE),
                                             mMenuColorTitle, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {4, 1},
                   GridFlags::BORDER_NONE);

    mStatus = std::make_shared<TextComponent>("NOT STARTED", Font::get(FONT_SIZE_MEDIUM),
                                              mMenuColorPrimary, ALIGN_CENTER);
    mGrid.setEntry(mStatus, glm::ivec2 {0, 1}, false, true, glm::ivec2 {4, 1},
                   GridFlags::BORDER_NONE);

    // Spacer row with bottom border.
    mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {0, 2}, false, false,
                   glm::ivec2 {4, 1}, GridFlags::BORDER_BOTTOM);

    mDescriptionHeader = std::make_shared<TextComponent>("DESCRIPTION:", Font::get(FONT_SIZE_MINI),
                                                         mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mDescriptionHeader, glm::ivec2 {1, 3}, false, true, glm::ivec2 {2, 1});

    mDescription = std::make_shared<TextComponent>(mMediaDescription, Font::get(FONT_SIZE_MEDIUM),
                                                   mMenuColorPrimary, ALIGN_LEFT, ALIGN_TOP);
    mGrid.setEntry(mDescription, glm::ivec2 {1, 4}, false, true, glm::ivec2 {2, 1});

    mEntryCountHeader = std::make_shared<TextComponent>(
        "TOTAL ENTRIES REMOVED:", Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mEntryCountHeader, glm::ivec2 {1, 6}, false, true, glm::ivec2 {1, 1});

    mEntryCount = std::make_shared<TextComponent>("0", Font::get(FONT_SIZE_SMALL),
                                                  mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mEntryCount, glm::ivec2 {2, 6}, false, true, glm::ivec2 {1, 1});

    mSystemProcessingHeader = std::make_shared<TextComponent>(
        "PROCESSING SYSTEM:", Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mSystemProcessingHeader, glm::ivec2 {1, 7}, false, true, glm::ivec2 {1, 1});

    mSystemProcessing = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL),
                                                        mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mSystemProcessing, glm::ivec2 {2, 7}, false, true, glm::ivec2 {1, 1});

    mErrorHeader = std::make_shared<TextComponent>(
        "LAST ERROR MESSAGE:", Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mErrorHeader, glm::ivec2 {1, 8}, false, true, glm::ivec2 {1, 1});

    mError =
        std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL), mMenuColorRed, ALIGN_LEFT);
    mGrid.setEntry(mError, glm::ivec2 {2, 8}, false, true, glm::ivec2 {1, 1});

    // Spacer row.
    mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {1, 9}, false, false,
                   glm::ivec2 {2, 1});

    // Buttons.
    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    mButton1 = std::make_shared<ButtonComponent>("MEDIA", "start processing", [this]() {
        if (mIsProcessing)
            return;
        if (mThread) {
            mThread->join();
            mThread.reset();
        }
        mProcessedCount = 0;
        mCurrentSystem = "";
        mCleanupType = CleanupType::MEDIA;
        mIsProcessing = true;
        mCompleted = false;
        mFailed = false;
        mStopProcessing = false;
        mError->setValue("");
        mEntryCount->setValue("0");
        mStatus->setValue("RUNNING MEDIA CLEANUP");
        mButton4->setText("STOP", "stop processing");
        mThread = std::make_unique<std::thread>(&GuiOrphanedDataCleanup::cleanupMediaFiles, this);
    });

    buttons.push_back(mButton1);

    mButton2 = std::make_shared<ButtonComponent>("GAMELISTS", "start processing", [this]() {
        if (mIsProcessing)
            return;
        if (mThread) {
            mThread->join();
            mThread.reset();
        }
        mProcessedCount = 0;
        mCurrentSystem = "";
        mCleanupType = CleanupType::GAMELISTS;
        mIsProcessing = true;
        mCompleted = false;
        mFailed = false;
        mStopProcessing = false;
        mError->setValue("");
        mEntryCount->setValue("0");
        mStatus->setValue("RUNNING GAMELISTS CLEANUP");
        mButton4->setText("STOP", "stop processing");
        // Write any gamelist.xml changes before proceeding with the cleanup.
        if (Settings::getInstance()->getString("SaveGamelistsMode") == "on exit") {
            for (auto system : SystemData::sSystemVector)
                system->writeMetaData();
        }
        mThread = std::make_unique<std::thread>(&GuiOrphanedDataCleanup::cleanupGamelists, this);
    });
    buttons.push_back(mButton2);

    mButton3 = std::make_shared<ButtonComponent>("COLLECTIONS", "start processing", [this]() {
        if (mIsProcessing)
            return;
        if (!mHasCustomCollections) {
            mError->setValue("THERE ARE NO ENABLED CUSTOM COLLECTIONS");
            return;
        }
        if (mThread) {
            mThread->join();
            mThread.reset();
        }
        mProcessedCount = 0;
        mCurrentSystem = "";
        mCleanupType = CleanupType::COLLECTIONS;
        mIsProcessing = true;
        mCompleted = false;
        mFailed = false;
        mStopProcessing = false;
        mError->setValue("");
        mEntryCount->setValue("0");
        mStatus->setValue("RUNNING COLLECTIONS CLEANUP");
        mButton4->setText("STOP", "stop processing");
        mThread = std::make_unique<std::thread>(&GuiOrphanedDataCleanup::cleanupCollections, this);
    });
    buttons.push_back(mButton3);

    mButton4 = std::make_shared<ButtonComponent>("CLOSE", "close", [this]() {
        if (mIsProcessing) {
            mStopProcessing = true;
            if (mThread) {
                mThread->join();
                mThread.reset();
            }
            mButton4->setText("CLOSE", "close");
        }
        else if (mNeedsReloading) {
            ViewController::getInstance()->rescanROMDirectory();
            mReloadCallback();
        }
        else {
            delete this;
        }
    });
    buttons.push_back(mButton4);

    mButtons = MenuComponent::makeButtonGrid(buttons);
    mGrid.setEntry(mButtons, glm::ivec2 {0, 10}, true, false, glm::ivec2 {4, 1},
                   GridFlags::BORDER_TOP);

    // Limit the width of the GUI on ultrawide monitors. The 1.778 aspect ratio value is
    // the 16:9 reference.
    const float aspectValue {1.778f / Renderer::getScreenAspectRatio()};
    const float width {glm::clamp(0.80f * aspectValue, 0.45f,
                                  (mRenderer->getIsVerticalOrientation() ? 0.95f : 0.80f)) *
                       mRenderer->getScreenWidth()};
    setSize(width,
            mTitle->getSize().y +
                (FONT_SIZE_MEDIUM * 1.5f * (mRenderer->getIsVerticalOrientation() ? 9.7f : 8.7f)) +
                mButtons->getSize().y);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                std::round(mRenderer->getScreenHeight() * 0.13f));

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText("PROCESSING");
    mBusyAnim.onSizeChanged();
}

GuiOrphanedDataCleanup::~GuiOrphanedDataCleanup()
{
    mStopProcessing = true;

    if (mThread)
        mThread->join();
}

void GuiOrphanedDataCleanup::cleanupMediaFiles()
{
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Starting cleanup of game media";

    const std::time_t currentTime {
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};

    for (auto system : SystemData::sSystemVector) {
        if (mStopProcessing)
            break;

        if (system->isCollection())
            continue;

        {
            std::unique_lock<std::mutex> lock {mMutex};
            mCurrentSystem = system->getName() + " (" + system->getFullName() + ")";
        }

        std::vector<std::string> systemFilesRelative;
        for (auto& systemFile : system->getRootFolder()->getFilesRecursive(GAME | FOLDER)) {
            std::string fileEntry {systemFile->getPath()};
            // Check that game entries are not directories as this may be the case when using the
            // directories interpreted as files functionality.
            if (systemFile->getType() == GAME && !Utils::FileSystem::isDirectory(fileEntry)) {
                // If the file has an extension, then remove it.
                const size_t separatorPos {fileEntry.find_last_of('/')};
                if (fileEntry.substr(separatorPos).find_last_of('.') != std::string::npos)
                    fileEntry = fileEntry.substr(0, fileEntry.find_last_of('.'));
            }
            systemFilesRelative.emplace_back(
                fileEntry.substr(system->getSystemEnvData()->mStartPath.length() + 1));
        }

        std::vector<std::string> cleanupFiles;
        const std::string systemMediaDir {mMediaDirectory + system->getName()};
        for (auto& mediaType : mMediaTypes) {
            const std::string mediaTypeDir {systemMediaDir + "/" + mediaType};
            const Utils::FileSystem::StringList& dirContent {
                Utils::FileSystem::getDirContent(mediaTypeDir, true)};
            for (auto& mediaFile : dirContent) {
                std::string relativePath {mediaFile.substr(mediaTypeDir.length() + 1)};
                relativePath = relativePath.substr(0, relativePath.find_last_of('.'));
                if (std::find(systemFilesRelative.cbegin(), systemFilesRelative.cend(),
                              relativePath) == systemFilesRelative.end()) {
                    cleanupFiles.emplace_back(mediaFile);
                    LOG(LogInfo) << "Found orphaned media file \"" << mediaFile << "\"";
                }
            }
        }

        if (mStopProcessing)
            break;

        if (cleanupFiles.size() > 0) {
            std::string dateString(20, '\0');
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", localtime(&currentTime));
            dateString.erase(dateString.find('\0'));
            const std::string targetDirectory {mMediaDirectory + "CLEANUP/" + dateString + "/"};

            LOG(LogInfo) << "Moving orphaned files to \"" << targetDirectory << "\"";

            for (auto& file : cleanupFiles) {
                const std::string fileDirectory {
                    targetDirectory +
                    Utils::FileSystem::getParent(file.substr(mMediaDirectory.length()))};
                const std::string fileName {Utils::FileSystem::getFileName(file)};
                if (!Utils::FileSystem::isDirectory(fileDirectory) &&
                    !Utils::FileSystem::createDirectory(fileDirectory)) {
                    LOG(LogError) << "Couldn't create target directory \"" << fileDirectory << "\"";
                    mErrorMessage = "COULDN'T CREATE TARGET DIRECTORY, PERMISSION PROBLEMS?";
                    mFailed = true;
                    mIsProcessing = false;
                    return;
                }
                if (Utils::FileSystem::renameFile(file, fileDirectory + "/" + fileName, false)) {
                    LOG(LogError) << "Couldn't move file \"" << file << "\"";
                    mErrorMessage = "COULDN'T MOVE MEDIA FILE, PERMISSION PROBLEMS?";
                    mFailed = true;
                    mIsProcessing = false;
                    return;
                }
                ++mProcessedCount;
            }
        }

        SDL_Delay(500);
    }

    mIsProcessing = false;
    mCompleted = true;
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Completed cleanup of game media, removed "
                 << mProcessedCount << " file" << (mProcessedCount == 1 ? "" : "s");
}

void GuiOrphanedDataCleanup::cleanupGamelists()
{
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Starting cleanup of gamelist.xml files";

    for (auto system : SystemData::sSystemVector) {
        if (mStopProcessing)
            break;

        if (system->isCollection())
            continue;

        {
            std::unique_lock<std::mutex> lock {mMutex};
            mCurrentSystem = system->getName() + " (" + system->getFullName() + ")";
        }

        if (mStopProcessing)
            break;

        mNeedsReloading = true;

        SDL_Delay(500);
    }

    mIsProcessing = false;
    mCompleted = true;
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Completed cleanup of gamelist.xml files, removed "
                 << mProcessedCount << (mProcessedCount == 1 ? " entry" : " entries");
}

void GuiOrphanedDataCleanup::cleanupCollections()
{
    LOG(LogInfo)
        << "GuiOrphanedDataCleanup: Starting cleanup of custom collections configuration files";

    for (auto& collection : CollectionSystemsManager::getInstance()->getCustomCollectionSystems()) {
        if (mStopProcessing)
            break;

        if (!collection.second.isEnabled)
            continue;

        {
            std::unique_lock<std::mutex> lock {mMutex};
            mCurrentSystem = collection.second.system->getName();
        }

        if (mStopProcessing)
            break;

        mNeedsReloading = true;

        SDL_Delay(500);
    }

    mIsProcessing = false;
    mCompleted = true;
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Completed cleanup of custom collections configuration "
                    "files, removed "
                 << mProcessedCount << (mProcessedCount == 1 ? " entry" : " entries");
}

void GuiOrphanedDataCleanup::update(int deltaTime)
{
    if (mIsProcessing) {
        mBusyAnim.update(deltaTime);
        if (mEntryCount->getValue() != std::to_string(mProcessedCount))
            mEntryCount->setValue(std::to_string(mProcessedCount));
    }

    if (mCompleted) {
        std::string message {mStopProcessing ? "ABORTED" : "COMPLETED"};
        if (mCleanupType == CleanupType::MEDIA)
            message.append(" MEDIA ");
        else if (mCleanupType == CleanupType::GAMELISTS)
            message.append(" GAMELISTS ");
        else
            message.append(" COLLECTIONS ");
        message.append("CLEANUP");
        mStatus->setValue(message);
        mSystemProcessing->setValue("");
        mButton4->setText("CLOSE", "close");
        mCompleted = false;
    }
    else if (mFailed) {
        std::string message;
        if (mCleanupType == CleanupType::MEDIA)
            message.append("MEDIA CLEANUP FAILED");
        else if (mCleanupType == CleanupType::GAMELISTS)
            message.append("GAMELISTS CLEANUP FAILED");
        else
            message.append("COLLECTIONS CLEANUP FAILED");
        mStatus->setValue(message);
        mError->setValue(mErrorMessage);
        mFailed = false;
    }
    else if (mIsProcessing) {
        std::unique_lock<std::mutex> lock {mMutex};
        if (mSystemProcessing->getValue() != mCurrentSystem)
            mSystemProcessing->setValue(mCurrentSystem);
    }
}

void GuiOrphanedDataCleanup::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);

    if (mIsProcessing)
        mBusyAnim.render(trans);
}

void GuiOrphanedDataCleanup::onSizeChanged()
{
    const float screenSize {mRenderer->getIsVerticalOrientation() ? mRenderer->getScreenWidth() :
                                                                    mRenderer->getScreenHeight()};
    mGrid.setRowHeightPerc(0, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  2.0f);
    mGrid.setRowHeightPerc(1, (mStatus->getFont()->getLetterHeight() + 2.0f) / mSize.y, false);
    mGrid.setRowHeightPerc(2, (mStatus->getFont()->getLetterHeight() * 0.5f) / mSize.y, false);
    mGrid.setRowHeightPerc(
        3, (mDescriptionHeader->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y / 4.0f);
    mGrid.setRowHeightPerc(4, (mDescription->getFont()->getLetterHeight() * 8.5f) / mSize.y);
    mGrid.setRowHeightPerc(5, (mStatus->getFont()->getLetterHeight() * 0.3f) / mSize.y);
    mGrid.setRowHeightPerc(
        6, (mEntryCountHeader->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y / 4.0f);
    mGrid.setRowHeightPerc(
        7, (mSystemProcessingHeader->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
               4.0f);
    mGrid.setRowHeightPerc(8, (mErrorHeader->getFont()->getLetterHeight() + screenSize * 0.2f) /
                                  mSize.y / 4.0f);
    mGrid.setRowHeightPerc(10, mButtons->getSize().y / mSize.y);

    mGrid.setColWidthPerc(0, 0.01f);
    mGrid.setColWidthPerc(1, 0.25f);
    mGrid.setColWidthPerc(3, 0.01f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize);
}

bool GuiOrphanedDataCleanup::input(InputConfig* config, Input input)
{
    if (input.value &&
        (config->isMappedLike("left", input) || config->isMappedLike("right", input))) {
        const int prevCursorPos {mCursorPos};
        if (config->isMappedLike("left", input)) {
            if (mCursorPos > 0)
                --mCursorPos;
        }
        else if (config->isMappedLike("right", input)) {
            if (mCursorPos < 3)
                ++mCursorPos;
        }

        if (mCursorPos != prevCursorPos) {
            if (mCursorPos == 0) {
                mDescription->setValue(mMediaDescription);
            }
            else if (mCursorPos == 1) {
                mDescription->setValue(mGamelistDescription);
            }
            else if (mCursorPos == 2) {
                mDescription->setValue(mCollectionsDescription);
            }
            else if (mCursorPos == 3) {
                mDescription->setValue(
                    mNeedsReloading ? "THE APPLICATION WILL RELOAD WHEN CLOSING THIS UTILITY." :
                                      "");
            }
        }
    }

    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiOrphanedDataCleanup::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};
    return prompts;
}

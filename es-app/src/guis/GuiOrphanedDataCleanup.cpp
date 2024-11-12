//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiOrphanedDataCleanup.cpp
//
//  Removes orphaned game media, gamelist.xml entries and custom collections entries.
//

#include "guis/GuiOrphanedDataCleanup.h"

#include "CollectionSystemsManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/PlatformUtil.h"
#include "views/ViewController.h"

#include <SDL2/SDL.h>
#include <pugixml.hpp>

GuiOrphanedDataCleanup::GuiOrphanedDataCleanup(std::function<void()> reloadCallback)
    : mRenderer {Renderer::getInstance()}
    , mBackground {":/graphics/frame.svg"}
    , mGrid {glm::ivec2 {4, 11}}
    , mReloadCallback {reloadCallback}
    , mCursorPos {0}
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
    , mCaseSensitiveFilesystem {true}
    , mCleanupType {CleanupType::MEDIA}
{
    // Make sure we always have a single trailing directory separator for the media directory.
    mMediaDirectory = FileData::getMediaDirectory();
    mMediaDirectory.erase(std::find_if(mMediaDirectory.rbegin(), mMediaDirectory.rend(),
                                       [](char c) { return c != '/'; })
                              .base(),
                          mMediaDirectory.end());

    mMediaDirectory.erase(std::find_if(mMediaDirectory.rbegin(), mMediaDirectory.rend(),
                                       [](char c) { return c != '\\'; })
                              .base(),
                          mMediaDirectory.end());
#if defined(_WIN64)
    mMediaDirectory.append("\\");
#else
    mMediaDirectory.append("/");
#endif

    addChild(&mBackground);
    addChild(&mGrid);

#if defined(_WIN64) || defined(__APPLE__) || defined(__ANDROID__)
    // Although macOS may have filesystem case-sensitivity enabled it's rare and in worst case
    // this will just leave some extra media files on the filesystem.
    mCaseSensitiveFilesystem = false;
#endif

    mMediaDescription =
        _("THIS WILL REMOVE ALL MEDIA FILES WHERE NO MATCHING GAME FILES CAN BE FOUND. "
          "THESE FILES WILL BE MOVED TO A CLEANUP FOLDER INSIDE YOUR GAME MEDIA "
          "DIRECTORY. YOU CAN MANUALLY DELETE THIS FOLDER WHEN YOU ARE SURE IT'S NO "
          "LONGER NEEDED.");

    mGamelistDescription = _(
        "THIS WILL REMOVE ALL ENTRIES FROM YOUR GAMELIST XML FILES WHERE NO MATCHING "
        "GAME FILES CAN BE FOUND. BACKUPS OF THE ORIGINAL FILES WILL BE SAVED TO A CLEANUP FOLDER "
        "INSIDE YOUR GAMELISTS DIRECTORY. YOU CAN MANUALLY DELETE THIS FOLDER WHEN YOU ARE SURE "
        "IT'S NO LONGER NEEDED.");

    mCollectionsDescription = _(
        "THIS WILL REMOVE ALL ENTRIES FROM YOUR CUSTOM COLLECTIONS CONFIGURATION FILES WHERE NO "
        "MATCHING GAME FILES CAN BE FOUND. BACKUPS OF THE ORIGINAL FILES WILL BE SAVED TO A "
        "CLEANUP FOLDER INSIDE YOUR COLLECTIONS DIRECTORY. ONLY CURRENTLY ENABLED COLLECTIONS WILL "
        "BE PROCESSED.");

    // Stop any ongoing custom collections editing.
    if (CollectionSystemsManager::getInstance()->isEditing())
        CollectionSystemsManager::getInstance()->exitEditMode();

    for (auto& collection : CollectionSystemsManager::getInstance()->getCustomCollectionSystems()) {
        if (collection.second.isEnabled)
            mHasCustomCollections = true;
    }

    // Set up grid.
    mTitle = std::make_shared<TextComponent>(
        _("ORPHANED DATA CLEANUP"),
        Font::get(FONT_SIZE_LARGE * Utils::Localization::sMenuTitleScaleFactor), mMenuColorTitle,
        ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {4, 1},
                   GridFlags::BORDER_NONE);

    mStatus = std::make_shared<TextComponent>(_("NOT STARTED"), Font::get(FONT_SIZE_MEDIUM),
                                              mMenuColorPrimary, ALIGN_CENTER);
    mGrid.setEntry(mStatus, glm::ivec2 {0, 1}, false, true, glm::ivec2 {4, 1},
                   GridFlags::BORDER_NONE);

    // Spacer row with bottom border.
    mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {0, 2}, false, false,
                   glm::ivec2 {4, 1}, GridFlags::BORDER_BOTTOM);

    mDescriptionHeader = std::make_shared<TextComponent>(
        _("DESCRIPTION:"), Font::get(FONT_SIZE_MINI), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mDescriptionHeader, glm::ivec2 {1, 3}, false, true, glm::ivec2 {2, 1});

    mDescription = std::make_shared<TextComponent>(
        mMediaDescription,
        Font::get(mRenderer->getScreenAspectRatio() < 1.6f ? FONT_SIZE_SMALL : FONT_SIZE_MEDIUM),
        mMenuColorPrimary, ALIGN_LEFT, ALIGN_TOP);
    mDescription->setNoSizeUpdate(true);
    mGrid.setEntry(mDescription, glm::ivec2 {1, 4}, false, true, glm::ivec2 {2, 1},
                   GridFlags::BORDER_NONE, GridFlags::UPDATE_ALWAYS, glm::ivec2 {0, 1});

    mEntryCountHeader = std::make_shared<TextComponent>(
        _("TOTAL ENTRIES REMOVED:"), Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mEntryCountHeader, glm::ivec2 {1, 6}, false, true, glm::ivec2 {1, 1});

    mEntryCount = std::make_shared<TextComponent>("0", Font::get(FONT_SIZE_SMALL),
                                                  mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mEntryCount, glm::ivec2 {2, 6}, false, true, glm::ivec2 {1, 1});

    mSystemProcessingHeader = std::make_shared<TextComponent>(
        _("LAST PROCESSED SYSTEM:"), Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mSystemProcessingHeader, glm::ivec2 {1, 7}, false, true, glm::ivec2 {1, 1});

    mSystemProcessing = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL),
                                                        mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mSystemProcessing, glm::ivec2 {2, 7}, false, true, glm::ivec2 {1, 1});

    mErrorHeader = std::make_shared<TextComponent>(
        _("LAST ERROR MESSAGE:"), Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_LEFT);
    mGrid.setEntry(mErrorHeader, glm::ivec2 {1, 8}, false, true, glm::ivec2 {1, 1});

    mError =
        std::make_shared<TextComponent>("", Font::get(FONT_SIZE_SMALL), mMenuColorRed, ALIGN_LEFT);
    mGrid.setEntry(mError, glm::ivec2 {2, 8}, false, true, glm::ivec2 {1, 1});

    // Spacer row.
    mGrid.setEntry(std::make_shared<GuiComponent>(), glm::ivec2 {1, 9}, false, false,
                   glm::ivec2 {2, 1});

    // Buttons.
    std::vector<std::shared_ptr<ButtonComponent>> buttons;

    mButton1 = std::make_shared<ButtonComponent>(_("MEDIA"), _("start processing"), [this]() {
        if (mIsProcessing && mStopProcessing)
            return;
        if (mIsProcessing) {
            mStopProcessing = true;
            return;
        }
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
        mErrorMessage = "";
        mError->setValue("");
        mEntryCount->setValue("0");
        mStatus->setValue(_("RUNNING MEDIA CLEANUP"));
        mButton1->setText(_("STOP"), _("stop processing"), true, false);
        mThread = std::make_unique<std::thread>(&GuiOrphanedDataCleanup::cleanupMediaFiles, this);
    });

    buttons.push_back(mButton1);

    mButton2 = std::make_shared<ButtonComponent>(_("GAMELISTS"), _("start processing"), [this]() {
        if (mIsProcessing && mStopProcessing)
            return;
        if (mIsProcessing) {
            mStopProcessing = true;
            return;
        }
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
        mErrorMessage = "";
        mError->setValue("");
        mEntryCount->setValue("0");
        mStatus->setValue(_("RUNNING GAMELISTS CLEANUP"));
        mButton2->setText(_("STOP"), _("stop processing"), true, false);
        // Write any gamelist.xml changes before proceeding with the cleanup.
        if (Settings::getInstance()->getString("SaveGamelistsMode") == "on exit") {
            for (auto system : SystemData::sSystemVector)
                system->writeMetaData();
        }
        mThread = std::make_unique<std::thread>(&GuiOrphanedDataCleanup::cleanupGamelists, this);
    });
    buttons.push_back(mButton2);

    mButton3 = std::make_shared<ButtonComponent>(_("COLLECTIONS"), _("start processing"), [this]() {
        if (mIsProcessing && mStopProcessing)
            return;
        if (mIsProcessing) {
            mStopProcessing = true;
            return;
        }
        if (!mHasCustomCollections) {
            mStatus->setValue(_("COLLECTIONS CLEANUP FAILED"));
            mError->setValue(_("There are no enabled custom collections"));
            mEntryCount->setValue("0");
            mSystemProcessing->setValue("");
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
        mErrorMessage = "";
        mError->setValue("");
        mEntryCount->setValue("0");
        mStatus->setValue(_("RUNNING COLLECTIONS CLEANUP"));
        mButton3->setText(_("STOP"), _("stop processing"), true, false);
        mThread = std::make_unique<std::thread>(&GuiOrphanedDataCleanup::cleanupCollections, this);
    });
    buttons.push_back(mButton3);

    mButton4 = std::make_shared<ButtonComponent>(_("CLOSE"), _("close"), [this]() {
        if (mIsProcessing) {
            mStopProcessing = true;
            if (mThread) {
                mThread->join();
                mThread.reset();
            }
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
    const float aspectValue {1.778f / mRenderer->getScreenAspectRatio()};
    // Some additional size adjustments are required for different aspect ratios.
    float multiplierY;
    if (mRenderer->getScreenAspectRatio() <= 1.0f)
        multiplierY = 10.0f;
    else if (mRenderer->getScreenAspectRatio() < 1.6f)
        multiplierY = 8.0f;
    else
        multiplierY = 8.7f;

    const float width {glm::clamp(0.81f * aspectValue, 0.40f,
                                  (mRenderer->getScreenAspectRatio() < 1.6f ? 0.97f : 0.9f)) *
                       mRenderer->getScreenWidth()};
    setSize(width,
            mTitle->getSize().y + (FONT_SIZE_MEDIUM * 1.5f * multiplierY) + mButtons->getSize().y);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                std::round(mRenderer->getScreenHeight() * 0.1f));

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText(_("PROCESSING"));
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
#if defined(_WIN64)
    // Workaround for a bug in the libintl library.
    Utils::Localization::setThreadLocale();
#endif

    LOG(LogInfo) << "GuiOrphanedDataCleanup: Starting cleanup of game media";

    const std::time_t currentTime {
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};

    int systemCounter {0};

    for (auto system : SystemData::sSystemVector) {
        if (system->isCollection())
            continue;

        if (mStopProcessing) {
            LOG(LogInfo) << "Stop signal received, aborting...";
            break;
        }

        const std::string currentSystem {system->getFullName() + " (" + system->getName() + ")"};
        LOG(LogInfo) << "Processing system \"" << currentSystem << "\"";

        {
            std::unique_lock<std::mutex> lock {mMutex};
            mCurrentSystem = currentSystem;
        }

        if (system->getFlattenFolders()) {
            LOG(LogError) << "A flatten.txt file was found, skipping system \"" << currentSystem
                          << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage = Utils::String::format(
                    _("A flatten.txt file was found, skipping \"%s\""), currentSystem.c_str());
            }
            continue;
        }

        ++systemCounter;

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
            if (mCaseSensitiveFilesystem) {
                systemFilesRelative.emplace_back(
                    fileEntry.substr(system->getSystemEnvData()->mStartPath.length() + 1));
            }
            else {
                systemFilesRelative.emplace_back(Utils::String::toUpper(
                    fileEntry.substr(system->getSystemEnvData()->mStartPath.length() + 1)));
            }
        }

        std::vector<std::string> cleanupFiles;
        const std::string systemMediaDir {mMediaDirectory + system->getName()};
        for (auto& mediaType : mMediaTypes) {
            const std::string mediaTypeDir {systemMediaDir + "/" + mediaType};
            const Utils::FileSystem::StringList& dirContent {
                Utils::FileSystem::getDirContent(mediaTypeDir, true)};
            for (auto& mediaFile : dirContent) {
                if (Utils::FileSystem::isDirectory(mediaFile))
                    continue;

                std::string relativePath;
                if (mCaseSensitiveFilesystem) {
                    relativePath = mediaFile.substr(mediaTypeDir.length() + 1);
                }
                else {
                    relativePath =
                        Utils::String::toUpper(mediaFile.substr(mediaTypeDir.length() + 1));
                }

                relativePath = relativePath.substr(0, relativePath.find_last_of('.'));
                if (std::find(systemFilesRelative.cbegin(), systemFilesRelative.cend(),
                              relativePath) == systemFilesRelative.end()) {
                    cleanupFiles.emplace_back(mediaFile);
#if defined(_WIN64)
                    LOG(LogInfo) << "Found orphaned media file \""
                                 << Utils::String::replace(mediaFile, "/", "\\") << "\"";
#else
                    LOG(LogInfo) << "Found orphaned media file \"" << mediaFile << "\"";
#endif
                }
            }
        }

        int systemProcessedCount {0};

        if (cleanupFiles.size() > 0) {
            struct tm tm;
            std::string dateString(20, '\0');
#if defined(_WIN64)
            localtime_s(&tm, &currentTime);
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", &tm);
#else
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", localtime_r(&currentTime, &tm));
#endif
            dateString.erase(dateString.find('\0'));
            const std::string targetDirectory {mMediaDirectory + "CLEANUP/" + dateString + "/"};
#if defined(_WIN64)
            LOG(LogInfo) << "Moving orphaned files to \""
                         << Utils::String::replace(targetDirectory, "/", "\\") + system->getName()
                         << "\\\"";
#else
            LOG(LogInfo) << "Moving orphaned files to \"" << targetDirectory + system->getName()
                         << "/\"";
#endif

            for (auto& file : cleanupFiles) {
                const std::string fileDirectory {
                    targetDirectory +
                    Utils::FileSystem::getParent(file.substr(mMediaDirectory.length()))};
                const std::string fileName {Utils::FileSystem::getFileName(file)};
                if (!Utils::FileSystem::isDirectory(fileDirectory) &&
                    !Utils::FileSystem::createDirectory(fileDirectory)) {
                    LOG(LogError) << "Couldn't create target directory \"" << fileDirectory << "\"";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage = _("Couldn't create target directory, permission problems?");
                    }
                    mFailed = true;
                    mIsProcessing = false;
                    return;
                }
                if (Utils::FileSystem::renameFile(file, fileDirectory + "/" + fileName, false)) {
                    LOG(LogError) << "Couldn't move file \"" << file << "\"";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage = _("Couldn't move media file, permission problems?");
                    }
                    mFailed = true;
                    mIsProcessing = false;
                    return;
                }
                ++mProcessedCount;
                ++systemProcessedCount;
            }
        }

        int directoryDeleteCounter {0};
        const Utils::FileSystem::StringList& emptyDirCheck {
            Utils::FileSystem::getDirContent(systemMediaDir, true)};

        for (auto& entry : emptyDirCheck) {
            if (!Utils::FileSystem::isDirectory(entry))
                continue;
            std::string path {entry};
            while (path != systemMediaDir) {
                if (Utils::FileSystem::getDirContent(path).size() == 0) {

#if defined(_WIN64)
                    LOG(LogInfo) << "Deleting empty directory \""
                                 << Utils::String::replace(path, "/", "\\") << "\"";
#else
                    LOG(LogInfo) << "Deleting empty directory \"" << path << "\"";
#endif
                    if (Utils::FileSystem::removeDirectory(path, false))
                        ++directoryDeleteCounter;
                    path = Utils::FileSystem::getParent(path);
                }
                else {
                    break;
                }
            }
        }

        LOG(LogInfo) << "Removed " << systemProcessedCount << " file"
                     << (systemProcessedCount == 1 ? " " : "s ") << "and " << directoryDeleteCounter
                     << (directoryDeleteCounter == 1 ? " directory " : " directories ")
                     << "for system \"" << currentSystem << "\"";

        SDL_Delay(500);
    }

    mIsProcessing = false;
    mCompleted = true;
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Completed cleanup of game media, processed "
                 << systemCounter << (systemCounter == 1 ? " system" : " systems") << ", removed "
                 << mProcessedCount << (mProcessedCount == 1 ? " file" : " files");
}

void GuiOrphanedDataCleanup::cleanupGamelists()
{
#if defined(_WIN64)
    // Workaround for a bug in the libintl library.
    Utils::Localization::setThreadLocale();
#endif

    LOG(LogInfo) << "GuiOrphanedDataCleanup: Starting cleanup of gamelist.xml files";

    if (!Settings::getInstance()->getBool("ShowHiddenGames")) {
        LOG(LogWarning)
            << "The \"Show hidden games\" setting is disabled, this may lead to some orphaned "
               "folder entries not getting purged";
    }

    const std::time_t currentTime {
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};

    int systemCounter {0};

    for (auto system : SystemData::sSystemVector) {
        if (system->isCollection())
            continue;

        if (mStopProcessing) {
            LOG(LogInfo) << "Stop signal received, aborting...";
            break;
        }

        const std::string currentSystem {system->getFullName() + " (" + system->getName() + ")"};
        LOG(LogInfo) << "Processing system \"" << currentSystem << "\"";

        {
            std::unique_lock<std::mutex> lock {mMutex};
            mCurrentSystem = currentSystem;
        }

        if (system->getFlattenFolders()) {
            LOG(LogError) << "A flatten.txt file was found, skipping system \"" << currentSystem
                          << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage = Utils::String::format(
                    _("A flatten.txt file was found, skipping \"%s\""), currentSystem.c_str());
            }
            continue;
        }

        ++systemCounter;

        const std::string gamelistFile {system->getGamelistPath(false)};

        if (gamelistFile == "") {
            LOG(LogInfo) << "System \"" << currentSystem << "\" does not have a gamelist.xml file";
            SDL_Delay(500);
            continue;
        }

        pugi::xml_document sourceDoc;
#if defined(_WIN64)
        const pugi::xml_parse_result& fileContents {
            sourceDoc.load_file(Utils::String::stringToWideString(gamelistFile).c_str())};
#else
        const pugi::xml_parse_result& fileContents {sourceDoc.load_file(gamelistFile.c_str())};
#endif

        if (!fileContents) {
            LOG(LogError) << "Couldn't parse file \"" << gamelistFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage = Utils::String::format(
                    _("Couldn't parse gamelist.xml file for \"%s\""), system->getName().c_str());
            }
            SDL_Delay(500);
            continue;
        }
#if defined(_WIN64)
        LOG(LogDebug) << "GuiOrphanedDataCleanup::cleanupGamelists(): Parsing file \""
                      << Utils::String::replace(gamelistFile, "/", "\\") << "\"";
#else
        LOG(LogDebug) << "GuiOrphanedDataCleanup::cleanupGamelists(): Parsing file \""
                      << gamelistFile << "\"";
#endif

        const pugi::xml_node& alternativeEmulator {sourceDoc.child("alternativeEmulator")};
        if (alternativeEmulator) {
            LOG(LogDebug)
                << "GuiOrphanedDataCleanup::cleanupGamelists(): Found an alternativeEmulator tag ";
        }

        const pugi::xml_node& sourceRoot {sourceDoc.child("gameList")};
        if (!sourceRoot) {
            LOG(LogError) << "Couldn't find a gameList tag in \"" << gamelistFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage =
                    Utils::String::format(_("Couldn't find a gamelist tag in file for \"%s\""),
                                          system->getName().c_str());
            }
            SDL_Delay(500);
            continue;
        }

        const std::string tempFile {Utils::FileSystem::getParent(gamelistFile) +
                                    "/gamelist.xml_CLEANUP.tmp"};

        if (Utils::FileSystem::exists(tempFile)) {
            LOG(LogWarning) << "Found existing temporary file \"" << tempFile << "\", deleting it";
            if (!Utils::FileSystem::removeFile(tempFile)) {
                LOG(LogError) << "Couldn't remove temporary file \"" << tempFile << "\"";
                {
                    std::unique_lock<std::mutex> lock {mMutex};
                    mErrorMessage =
                        _("Couldn't delete temporary gamelist file, permission problems?");
                }
                mFailed = true;
                mIsProcessing = false;
                return;
            }
        }

        const std::string startPath {system->getSystemEnvData()->mStartPath};
        int removeCount {0};

        pugi::xml_document targetDoc;
        pugi::xml_node targetRoot;

        bool saveFailure {false};

        if (alternativeEmulator) {
            targetDoc.prepend_copy(alternativeEmulator);
            if (!targetDoc.save_file(tempFile.c_str()))
                saveFailure = true;
        }

        if (!saveFailure) {
            targetRoot = targetDoc.append_child("gameList");
            if (!targetDoc.save_file(tempFile.c_str()))
                saveFailure = true;
        }

        if (saveFailure) {
            LOG(LogError) << "Couldn't write to temporary file \"" << tempFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage =
                    _("Couldn't write to temporary gamelist file, permission problems?");
            }
            // If we couldn't write to the file this will probably fail as well.
            Utils::FileSystem::removeFile(tempFile);
            mFailed = true;
            mIsProcessing = false;
            return;
        }

        const std::vector<std::string> knownTags {"game", "folder"};
        const std::vector<std::string>& extensions {system->getSystemEnvData()->mSearchExtensions};

        // Step through every game and folder element so that the order of entries will remain
        // in the target gamelist.xml file.
        for (auto it = sourceRoot.begin(); it != sourceRoot.end(); ++it) {
            const std::string tag {(*it).name()};
            if (tag == knownTags[0] || tag == knownTags[1]) {
                const std::string path {(*it).child("path").text().get()};
                if (path == "") {
                    LOG(LogInfo) << "Found invalid " << tag << " entry with missing path tag";
                    ++removeCount;
                }
                else if (path.substr(0, 2) != "./") {
                    LOG(LogInfo) << "Found invalid " << tag << " entry \"" << path << "\"";
                    ++removeCount;
                }
                else if (Utils::FileSystem::exists(startPath + "/" + path)) {
                    if (tag == "game") {
                        // Remove entries with extensions not defined in es_systems.xml.
                        if (std::find(extensions.cbegin(), extensions.cend(),
                                      Utils::FileSystem::getExtension(path)) != extensions.cend()) {
                            targetRoot.append_copy((*it));
                        }
                        else {
                            LOG(LogInfo) << "Found orphaned " << tag << " entry \"" << path << "\"";
                            ++removeCount;
                        }
                    }
                    else if (!Settings::getInstance()->getBool("ShowHiddenGames")) {
                        // Don't remove entries for existing folders if not displaying hidden games.
                        targetRoot.append_copy((*it));
                    }
                    else {
                        bool folderExists {false};
                        for (auto child : system->getRootFolder()->getChildrenRecursive()) {
                            if (child->getType() == FOLDER &&
                                child->getPath() ==
                                    system->getRootFolder()->getPath() + path.substr(1)) {
                                folderExists = true;
                                break;
                            }
                        }
                        if (folderExists) {
                            targetRoot.append_copy((*it));
                        }
                        else {
                            LOG(LogInfo) << "Found orphaned " << tag << " entry \"" << path << "\"";
                            ++removeCount;
                        }
                    }
                }
                else {
                    LOG(LogInfo) << "Found orphaned " << tag << " entry \"" << path << "\"";
                    ++removeCount;
                }
            }
            else {
                LOG(LogInfo) << "Retaining unknown tag \"" << tag << "\"";
                targetRoot.append_copy((*it));
            }
        }

        if (!targetDoc.save_file(tempFile.c_str())) {
            LOG(LogError) << "Couldn't write to temporary file \"" << tempFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage =
                    _("Couldn't write to temporary gamelist file, permission problems?");
            }
            Utils::FileSystem::removeFile(tempFile);
            mFailed = true;
            mIsProcessing = false;
            return;
        }

        if (removeCount > 0) {
            struct tm tm;
            std::string dateString(20, '\0');
#if defined(_WIN64)
            localtime_s(&tm, &currentTime);
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", &tm);
#else
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", localtime_r(&currentTime, &tm));
#endif
            dateString.erase(dateString.find('\0'));
            const std::string targetDirectory {
                Utils::FileSystem::getParent(
                    Utils::FileSystem::getParent(system->getGamelistPath(false))) +
                "/CLEANUP/" + dateString + "/" + system->getName()};

            if (!Utils::FileSystem::isDirectory(targetDirectory) &&
                !Utils::FileSystem::createDirectory(targetDirectory)) {
                LOG(LogError) << "Couldn't create backup directory \"" << targetDirectory << "\"";
                {
                    std::unique_lock<std::mutex> lock {mMutex};
                    mErrorMessage = _("Couldn't create backup directory, permission problems?");
                }
                mFailed = true;
            }

            if (!mFailed) {
#if defined(_WIN64)
                LOG(LogInfo) << "Moving old gamelist.xml file to \""
                             << Utils::String::replace(targetDirectory, "/", "\\") << "\\\"";
#else
                LOG(LogInfo) << "Moving old gamelist.xml file to \"" << targetDirectory << "/\"";
#endif
                if (Utils::FileSystem::renameFile(gamelistFile, targetDirectory + "/gamelist.xml",
                                                  true)) {
                    LOG(LogError) << "Couldn't move file \"" << gamelistFile << "\"";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage = _("Couldn't move old gamelist file, permission problems?");
                    }
                    mFailed = true;
                }
                else if (Utils::FileSystem::renameFile(tempFile, gamelistFile, true)) {
                    LOG(LogError) << "Couldn't move file \"" << tempFile << "\"";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage =
                            _("Couldn't move temporary gamelist file, permission problems?");
                    }
                    mFailed = true;
                    // Attempt to move back the old gamelist.xml file.
                    Utils::FileSystem::renameFile(targetDirectory + "/gamelist.xml", gamelistFile,
                                                  true);
                }
                if (!mFailed)
                    mNeedsReloading = true;
            }
        }

        LOG(LogInfo) << "Removed " << removeCount << (removeCount == 1 ? " entry " : " entries ")
                     << "for system \"" << currentSystem << "\"";

        if (!mFailed)
            mProcessedCount += removeCount;

        if (Utils::FileSystem::exists(tempFile) && !Utils::FileSystem::removeFile(tempFile)) {
            LOG(LogError) << "Couldn't remove temporary file \"" << tempFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage = _("Couldn't delete temporary gamelist file, permission problems?");
            }
            mFailed = true;
        }

        SDL_Delay(500);

        if (mFailed)
            break;
    }

    if (!mFailed) {
        mCompleted = true;
        LOG(LogInfo)
            << "GuiOrphanedDataCleanup: Completed cleanup of gamelist.xml files, processed "
            << systemCounter << (systemCounter == 1 ? " system" : " systems") << ", removed "
            << mProcessedCount << (mProcessedCount == 1 ? " entry" : " entries");
    }

    mIsProcessing = false;
}

void GuiOrphanedDataCleanup::cleanupCollections()
{
#if defined(_WIN64)
    // Workaround for a bug in the libintl library.
    Utils::Localization::setThreadLocale();
#endif

    LOG(LogInfo)
        << "GuiOrphanedDataCleanup: Starting cleanup of custom collections configuration files";

    const std::time_t currentTime {
        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())};

    int systemCounter {0};

    for (auto& collection : CollectionSystemsManager::getInstance()->getCustomCollectionSystems()) {
        if (!collection.second.isEnabled)
            continue;

        if (mStopProcessing) {
            LOG(LogInfo) << "Stop signal received, aborting...";
            break;
        }

        ++systemCounter;

        const std::string collectionName {collection.second.system->getName()};
        LOG(LogInfo) << "Processing collection system \"" << collectionName << "\"";

        {
            std::unique_lock<std::mutex> lock {mMutex};
            mCurrentSystem = collectionName;
        }

        const std::string collectionFile {
            CollectionSystemsManager::getInstance()->getCustomCollectionConfigPath(collectionName)};

        if (!Utils::FileSystem::exists(collectionFile)) {
            LOG(LogError) << "Couldn't find custom collection configuration file \""
                          << collectionFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage = _("Couldn't find custom collection configuration file");
            }
            mFailed = true;
            mIsProcessing = false;
            return;
        }
#if defined(_WIN64)
        LOG(LogDebug) << "GuiOrphanedDataCleanup::cleanupCollections(): Parsing file \""
                      << Utils::String::replace(collectionFile, "/", "\\") << "\"";
#else
        LOG(LogDebug) << "GuiOrphanedDataCleanup::cleanupCollections(): Parsing file \""
                      << collectionFile << "\"";
#endif
        // Get configuration for this custom collection.
        std::vector<std::string> validEntries;
        int removeCount {0};
        std::ifstream configFileSource;

#if defined(_WIN64)
        configFileSource.open(Utils::String::stringToWideString(collectionFile).c_str());
#else
        configFileSource.open(collectionFile);
#endif
        if (!configFileSource.good()) {
            LOG(LogError) << "Couldn't open custom collection configuration file \""
                          << collectionFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage = _("Couldn't open custom collection configuration file");
            }
            mFailed = true;
            mIsProcessing = false;
            return;
        }

        for (std::string gameKey; getline(configFileSource, gameKey);) {
            // If there is a %ROMPATH% variable set for the game, expand it. By doing this
            // it's possible to use either absolute ROM paths in the collection files or using
            // the path variable. The absolute ROM paths are only used for backward compatibility
            // with old custom collections. All custom collections saved by ES-DE will use the
            // %ROMPATH% variable instead.
            std::string expandedKey {
                Utils::String::replace(gameKey, "%ROMPATH%", FileData::getROMDirectory())};
            expandedKey = Utils::String::replace(expandedKey, "//", "/");
            if (Utils::FileSystem::exists(expandedKey)) {
                validEntries.emplace_back(gameKey);
            }
            else {
                LOG(LogInfo) << "Found orphaned collection entry \"" << gameKey << "\"";
                ++removeCount;
            }
        }

        if (configFileSource.is_open())
            configFileSource.close();

        const std::string tempFile {collectionFile + "_CLEANUP.tmp"};

        if (Utils::FileSystem::exists(tempFile)) {
            LOG(LogWarning) << "Found existing temporary file \"" << tempFile << "\", deleting it";
            if (!Utils::FileSystem::removeFile(tempFile)) {
                LOG(LogError) << "Couldn't remove temporary file";
                {
                    std::unique_lock<std::mutex> lock {mMutex};
                    mErrorMessage =
                        _("Couldn't delete temporary collection file, permission problems?");
                }
                mFailed = true;
                mIsProcessing = false;
                return;
            }
        }

        if (removeCount > 0) {
            struct tm tm;
            std::string dateString(20, '\0');
#if defined(_WIN64)
            localtime_s(&tm, &currentTime);
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", &tm);
#else
            std::strftime(&dateString[0], 20, "%Y-%m-%d_%H%M%S", localtime_r(&currentTime, &tm));
#endif
            dateString.erase(dateString.find('\0'));
            const std::string targetDirectory {Utils::FileSystem::getParent(collectionFile) +
                                               "/CLEANUP/" + dateString + "/"};
            if (!Utils::FileSystem::isDirectory(targetDirectory) &&
                !Utils::FileSystem::createDirectory(targetDirectory)) {
                LOG(LogError) << "Couldn't create backup directory \"" << targetDirectory << "\"";
                {
                    std::unique_lock<std::mutex> lock {mMutex};
                    mErrorMessage = _("Couldn't create backup directory, permission problems?");
                }
                mFailed = true;
                mIsProcessing = false;
                return;
            }
            else {
                std::ofstream configFileTarget;
#if defined(_WIN64)
                configFileTarget.open(Utils::String::stringToWideString(tempFile).c_str(),
                                      std::ios::binary);
#else
                configFileTarget.open(tempFile, std::ios::binary);
#endif
                if (!configFileTarget.good()) {
                    LOG(LogError) << "Couldn't write to temporary collection configuration file \""
                                  << tempFile << "\"";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage =
                            _("Couldn't write to temporary collection configuration file");
                    }
                    mFailed = true;
                    mIsProcessing = false;
                    return;
                }

                for (auto& entry : validEntries)
                    configFileTarget << entry << std::endl;

                if (configFileTarget.is_open())
                    configFileTarget.close();
#if defined(_WIN64)
                LOG(LogInfo) << "Moving old \"" << Utils::FileSystem::getFileName(collectionFile)
                             << "\" file to \""
                             << Utils::String::replace(targetDirectory, "/", "\\") << "\"";
#else
                LOG(LogInfo) << "Moving old \"" << Utils::FileSystem::getFileName(collectionFile)
                             << "\" file to \"" << targetDirectory << "\"";
#endif

                if (Utils::FileSystem::renameFile(
                        collectionFile,
                        targetDirectory + "/" + Utils::FileSystem::getFileName(collectionFile),
                        true)) {
                    LOG(LogError) << "Couldn't move file \"" << collectionFile
                                  << "\" to backup directory";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage =
                            _("Couldn't move old collection file, permission problems?");
                    }
                    // Attempt to move back the old collection file.
                    Utils::FileSystem::renameFile(
                        targetDirectory + Utils::FileSystem::getFileName(collectionFile),
                        collectionFile, false);
                    mFailed = true;
                }
                else if (Utils::FileSystem::renameFile(tempFile, collectionFile, true)) {
                    LOG(LogError) << "Couldn't move file \"" << tempFile << "\"";
                    {
                        std::unique_lock<std::mutex> lock {mMutex};
                        mErrorMessage =
                            _("Couldn't move temporary collection file, permission problems?");
                    }
                    // Attempt to move back the old collection file.
                    Utils::FileSystem::renameFile(
                        targetDirectory + Utils::FileSystem::getFileName(collectionFile),
                        collectionFile, true);
                    mFailed = true;
                }
                if (!mFailed)
                    mNeedsReloading = true;
            }
        }

        LOG(LogInfo) << "Removed " << removeCount << (removeCount == 1 ? " entry " : " entries ")
                     << "from collection system \"" << collectionName << "\"";

        if (!mFailed)
            mProcessedCount += removeCount;

        if (Utils::FileSystem::exists(tempFile) && !Utils::FileSystem::removeFile(tempFile)) {
            LOG(LogError) << "Couldn't remove temporary file \"" << tempFile << "\"";
            {
                std::unique_lock<std::mutex> lock {mMutex};
                mErrorMessage =
                    _("Couldn't delete temporary collection file, permission problems?");
            }
            mFailed = true;
        }

        SDL_Delay(500);

        if (mFailed)
            return;
    }

    mIsProcessing = false;
    mCompleted = true;
    LOG(LogInfo) << "GuiOrphanedDataCleanup: Completed cleanup of custom collections configuration "
                    "files, processed "
                 << systemCounter << (systemCounter == 1 ? " system" : " systems") << ", removed "
                 << mProcessedCount << (mProcessedCount == 1 ? " entry" : " entries");
}

void GuiOrphanedDataCleanup::update(int deltaTime)
{
    if (mIsProcessing) {
        mBusyAnim.update(deltaTime);
        if (mEntryCount->getValue() != std::to_string(mProcessedCount))
            mEntryCount->setValue(std::to_string(mProcessedCount));
        std::unique_lock<std::mutex> lock {mMutex};
        if (mSystemProcessing->getValue() != mCurrentSystem)
            mSystemProcessing->setValue(mCurrentSystem);
        if (mError->getValue() != mErrorMessage)
            mError->setValue(mErrorMessage);
    }
    else if (mCompleted) {
        std::string message;
        if (mCleanupType == CleanupType::MEDIA) {
            mButton1->setText(_("MEDIA"), _("start processing"));
            if (mStopProcessing)
                message = _("ABORTED MEDIA CLEANUP");
            else
                message = _("COMPLETED MEDIA CLEANUP");
        }
        else if (mCleanupType == CleanupType::GAMELISTS) {
            mButton2->setText(_("GAMELISTS"), _("start processing"));
            if (mStopProcessing)
                message = _("ABORTED GAMELIST CLEANUP");
            else
                message = _("COMPLETED GAMELIST CLEANUP");
        }
        else {
            mButton3->setText(_("COLLECTIONS"), _("start processing"));
            if (mStopProcessing)
                message = _("ABORTED COLLECTIONS CLEANUP");
            else
                message = _("COMPLETED COLLECTIONS CLEANUP");
        }
        mStatus->setValue(message);
        if (mError->getValue() != mErrorMessage)
            mError->setValue(mErrorMessage);
        mCompleted = false;
    }
    else if (mFailed) {
        std::string message;
        if (mCleanupType == CleanupType::MEDIA) {
            mButton1->setText(_("MEDIA"), _("start processing"));
            message.append(_("MEDIA CLEANUP FAILED"));
        }
        else if (mCleanupType == CleanupType::GAMELISTS) {
            mButton2->setText(_("GAMELISTS"), _("start processing"));
            message.append(_("GAMELISTS CLEANUP FAILED"));
        }
        else {
            mButton3->setText(_("COLLECTIONS"), _("start processing"));
            message.append(_("COLLECTIONS CLEANUP FAILED"));
        }
        mStatus->setValue(message);
        {
            std::unique_lock<std::mutex> lock {mMutex};
            mError->setValue(mErrorMessage);
        }
        mFailed = false;
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
    float descSizeY;
    float col1Size;

    // Some additional size adjustments are required for different aspect ratios.
    if (mRenderer->getScreenAspectRatio() <= 1.0f) {
        descSizeY = 12.0f;
        col1Size = 0.36f;
    }
    else if (mRenderer->getScreenAspectRatio() < 1.6f) {
        descSizeY = 9.2f;
        col1Size = 0.28f;
    }
    else {
        descSizeY = 8.9f;
        col1Size = 0.25f;
    }

    mGrid.setRowHeightPerc(0, (mTitle->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y /
                                  2.0f);
    mGrid.setRowHeightPerc(1, (mStatus->getFont()->getLetterHeight() + 2.0f) / mSize.y, false);
    mGrid.setRowHeightPerc(2, (mStatus->getFont()->getLetterHeight() * 0.5f) / mSize.y, false);
    mGrid.setRowHeightPerc(
        3, (mDescriptionHeader->getFont()->getLetterHeight() + screenSize * 0.2f) / mSize.y / 4.0f);

    mGrid.setRowHeightPerc(4, (mDescription->getFont()->getLetterHeight() * descSizeY) / mSize.y);

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
    mGrid.setColWidthPerc(1, col1Size);
    mGrid.setColWidthPerc(3, 0.01f);

    mGrid.setSize(mSize);
    mBackground.fitTo(mSize);
}

bool GuiOrphanedDataCleanup::input(InputConfig* config, Input input)
{
    if (mIsProcessing && input.value &&
        (config->isMappedLike("left", input) || config->isMappedLike("right", input)))
        return true;

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
                    mNeedsReloading ? _("THE APPLICATION WILL RELOAD WHEN CLOSING THIS UTILITY.") :
                                      "");
            }
        }
    }

    return GuiComponent::input(config, input);
}

std::vector<HelpPrompt> GuiOrphanedDataCleanup::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts {mGrid.getHelpPrompts()};
    if (mIsProcessing) {
        prompts.pop_back();
        prompts.pop_back();
    }
    return prompts;
}

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileData.cpp
//
//  Provides game file data structures and functions to access and sort this information.
//  Also provides functions to look up paths to media files and for launching games
//  (launching initiated in ViewController).
//

#include "FileData.h"

#include "CollectionSystemsManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Log.h"
#include "MameNames.h"
#include "Scripting.h"
#include "SystemData.h"
#include "UIModeController.h"
#include "Window.h"
#include "utils/FileSystemUtil.h"
#include "utils/PlatformUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "views/GamelistView.h"
#include "views/ViewController.h"

#include <assert.h>

FileData::FileData(FileType type,
                   const std::string& path,
                   SystemEnvironmentData* envData,
                   SystemData* system)
    : metadata {type == GAME ? GAME_METADATA : FOLDER_METADATA}
    , mSourceFileData {nullptr}
    , mParent {nullptr}
    , mType {type}
    , mPath {path}
    , mEnvData {envData}
    , mSystem {system}
    , mOnlyFolders {false}
    , mUpdateChildrenLastPlayed {false}
    , mUpdateChildrenMostPlayed {false}
    , mDeletionFlag {false}
{
    // Metadata needs at least a name field (since that's what getName() will return).
    if (metadata.get("name").empty()) {
        if ((system->hasPlatformId(PlatformIds::ARCADE) ||
             system->hasPlatformId(PlatformIds::SNK_NEO_GEO)) &&
            metadata.getType() != FOLDER_METADATA) {
            // If it's a MAME or Neo Geo game, expand the game name accordingly.
            metadata.set("name", MameNames::getInstance().getCleanName(getCleanName()));
        }
        else {
            if (metadata.getType() == FOLDER_METADATA && Utils::FileSystem::isHidden(mPath)) {
                metadata.set("name", Utils::FileSystem::getFileName(mPath));
            }
            else {
                metadata.set("name", getDisplayName());
            }
        }
    }
    mSystemName = system->getName();
    metadata.resetChangedFlag();
}

FileData::~FileData()
{
    while (mChildren.size() > 0)
        delete (mChildren.front());

    if (mParent)
        mParent->removeChild(this);
}

std::string FileData::getDisplayName() const
{
    std::string stem {Utils::FileSystem::getStem(mPath)};
    return stem;
}

std::string FileData::getCleanName() const
{
    return Utils::String::removeParenthesis(this->getDisplayName());
}

const std::string& FileData::getName()
{
    // Return metadata name.
    return metadata.get("name");
}

const std::string& FileData::getSortName()
{
    if (mSystem->isCustomCollection() && mType == GAME) {
        if (!metadata.get("collectionsortname").empty())
            return metadata.get("collectionsortname");
        else if (!metadata.get("sortname").empty())
            return metadata.get("sortname");
        else
            return metadata.get("name");
    }

    if (metadata.get("sortname").empty())
        return metadata.get("name");
    else
        return metadata.get("sortname");
}

const bool FileData::getFavorite()
{
    if (metadata.get("favorite") == "true")
        return true;
    else
        return false;
}

const bool FileData::getKidgame()
{
    if (metadata.get("kidgame") == "true")
        return true;
    else
        return false;
}

const bool FileData::getHidden()
{
    if (metadata.get("hidden") == "true")
        return true;
    else
        return false;
}

const bool FileData::getCountAsGame()
{
    if (metadata.get("nogamecount") == "true")
        return false;
    else
        return true;
}

const bool FileData::getExcludeFromScraper()
{
    if (metadata.get("nomultiscrape") == "true")
        return true;
    else
        return false;
}

const std::vector<FileData*> FileData::getChildrenRecursive() const
{
    std::vector<FileData*> childrenRecursive;

    for (auto it = mChildrenByFilename.cbegin(); it != mChildrenByFilename.cend(); ++it) {
        childrenRecursive.emplace_back((*it).second);
        // Recurse through any subdirectories.
        if ((*it).second->getType() == FOLDER) {
            std::vector<FileData*> childrenSubdirectory {(*it).second->getChildrenRecursive()};
            childrenRecursive.insert(childrenRecursive.end(), childrenSubdirectory.begin(),
                                     childrenSubdirectory.end());
        }
    }

    return childrenRecursive;
}

const std::string FileData::getROMDirectory()
{
    std::string romDirSetting {Settings::getInstance()->getString("ROMDirectory")};
    std::string romDirPath;

    if (romDirSetting == "") {
        romDirPath = Utils::FileSystem::getHomePath() + "/ROMs/";
    }
    else {
        romDirPath = romDirSetting;
        // Expand home path if ~ is used.
        romDirPath = Utils::FileSystem::expandHomePath(romDirPath);

#if defined(_WIN64)
        if (romDirPath.back() != '\\')
            romDirPath = romDirPath + "\\";
#else
        if (romDirPath.back() != '/')
            romDirPath = romDirPath + "/";
#endif
    }

    // If %ESPATH% is used for the ROM path configuration, then expand it to the binary
    // directory of ES-DE.
    romDirPath = Utils::String::replace(romDirPath, "%ESPATH%", Utils::FileSystem::getExePath());

    return romDirPath;
}

const std::string FileData::getMediaDirectory()
{
    std::string mediaDirSetting {Settings::getInstance()->getString("MediaDirectory")};
    std::string mediaDirPath;

    if (mediaDirSetting == "") {
        mediaDirPath = Utils::FileSystem::getHomePath() + "/.emulationstation/downloaded_media/";
    }
    else {
        mediaDirPath = mediaDirSetting;
        // Expand home path if ~ is used.
        mediaDirPath = Utils::FileSystem::expandHomePath(mediaDirPath);

        // If %ESPATH% is used for the media directory configuration, then expand it to the
        // binary directory of ES-DE.
        mediaDirPath =
            Utils::String::replace(mediaDirPath, "%ESPATH%", Utils::FileSystem::getExePath());

        if (mediaDirPath.back() != '/')
            mediaDirPath = mediaDirPath + "/";
    }

    return mediaDirPath;
}

const std::string FileData::getMediafilePath(const std::string& subdirectory) const
{
    const std::vector<std::string> extList {".png", ".jpg"};
    std::string subFolders;

    // Extract possible subfolders from the path.
    if (mEnvData->mStartPath != "")
        subFolders =
            Utils::String::replace(Utils::FileSystem::getParent(mPath), mEnvData->mStartPath, "");

    const std::string tempPath {getMediaDirectory() + mSystemName + "/" + subdirectory +
                                subFolders + "/" + getDisplayName()};

    // Look for an image file in the media directory.
    for (size_t i = 0; i < extList.size(); ++i) {
        std::string mediaPath {tempPath + extList[i]};
        if (Utils::FileSystem::exists(mediaPath))
            return mediaPath;
    }

    return "";
}

const std::string FileData::getImagePath() const
{
    // Look for a mix image (a combination of screenshot, 2D/3D box and marquee).
    std::string image {getMediafilePath("miximages")};

    if (image != "")
        return image;

    // If no mix image was found, try screenshot instead.
    image = getMediafilePath("screenshots");
    if (image != "")
        return image;

    // If no screenshot image was found, try title screen instead.
    image = getMediafilePath("titlescreens");
    if (image != "")
        return image;

    // If no screenshot was found either, try cover.
    return getMediafilePath("covers");
}

const std::string FileData::get3DBoxPath() const
{
    // Return path to the 3D box image.
    return getMediafilePath("3dboxes");
}

const std::string FileData::getBackCoverPath() const
{
    // Return path to the box back cover image.
    return getMediafilePath("backcovers");
}

const std::string FileData::getCoverPath() const
{
    // Return path to the box cover image.
    return getMediafilePath("covers");
}

const std::string FileData::getFanArtPath() const
{
    // Return path to the fan art image.
    return getMediafilePath("fanart");
}

const std::string FileData::getMarqueePath() const
{
    // Return path to the marquee image.
    return getMediafilePath("marquees");
}

const std::string FileData::getPhysicalMediaPath() const
{
    // Return path to the physical media image.
    return getMediafilePath("physicalmedia");
}

const std::string FileData::getMiximagePath() const
{
    // Return path to the miximage.
    return getMediafilePath("miximages");
}

const std::string FileData::getScreenshotPath() const
{
    // Return path to the screenshot image.
    return getMediafilePath("screenshots");
}

const std::string FileData::getTitleScreenPath() const
{
    // Return path to the title screen image.
    return getMediafilePath("titlescreens");
}

const std::string FileData::getVideoPath() const
{
    const std::vector<std::string> extList {".avi", ".mkv", ".mov", ".mp4", ".wmv"};
    std::string subFolders;

    // Extract possible subfolders from the path.
    if (mEnvData->mStartPath != "")
        subFolders =
            Utils::String::replace(Utils::FileSystem::getParent(mPath), mEnvData->mStartPath, "");

    const std::string tempPath {getMediaDirectory() + mSystemName + "/videos" + subFolders + "/" +
                                getDisplayName()};

    // Look for media in the media directory.
    for (size_t i = 0; i < extList.size(); ++i) {
        std::string mediaPath {tempPath + extList[i]};
        if (Utils::FileSystem::exists(mediaPath))
            return mediaPath;
    }

    return "";
}

const std::vector<FileData*>& FileData::getChildrenListToDisplay()
{
    FileFilterIndex* idx {mSystem->getIndex()};
    if (idx->isFiltered() || UIModeController::getInstance()->isUIModeKid()) {
        mFilteredChildren.clear();
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
            if (idx->showFile((*it))) {
                mFilteredChildren.emplace_back(*it);
            }
        }
        return mFilteredChildren;
    }
    else {
        return mChildren;
    }
}

std::vector<FileData*> FileData::getFilesRecursive(unsigned int typeMask,
                                                   bool displayedOnly,
                                                   bool countAllGames) const
{
    std::vector<FileData*> out;
    FileFilterIndex* idx {mSystem->getIndex()};

    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
        if ((*it)->getType() & typeMask) {
            if (!displayedOnly || !idx->isFiltered() || idx->showFile(*it)) {
                if (countAllGames)
                    out.emplace_back(*it);
                else if ((*it)->getCountAsGame())
                    out.emplace_back(*it);
            }
        }
        if ((*it)->getChildren().size() > 0) {
            std::vector<FileData*> subChildren {(*it)->getFilesRecursive(typeMask, displayedOnly)};
            if (countAllGames) {
                out.insert(out.cend(), subChildren.cbegin(), subChildren.cend());
            }
            else {
                for (auto it2 = subChildren.cbegin(); it2 != subChildren.cend(); ++it2) {
                    if ((*it2)->getCountAsGame())
                        out.emplace_back(*it2);
                }
            }
        }
    }

    return out;
}

std::vector<FileData*> FileData::getScrapeFilesRecursive(bool includeFolders,
                                                         bool excludeRecursively,
                                                         bool respectExclusions) const
{
    std::vector<FileData*> out;

    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
        if (includeFolders && (*it)->getType() == FOLDER) {
            if (!(respectExclusions && (*it)->getExcludeFromScraper()))
                out.emplace_back(*it);
        }
        else if ((*it)->getType() == GAME) {
            if (!(respectExclusions && (*it)->getExcludeFromScraper()))
                out.emplace_back(*it);
        }

        // If the flag has been passed to exclude directories recursively, then skip the entire
        // folder at this point if the folder is marked for scrape exclusion.
        if (excludeRecursively && (*it)->getType() == FOLDER && (*it)->getExcludeFromScraper())
            continue;

        if ((*it)->getChildren().size() > 0) {
            std::vector<FileData*> subChildren {(*it)->getScrapeFilesRecursive(
                includeFolders, excludeRecursively, respectExclusions)};
            out.insert(out.cend(), subChildren.cbegin(), subChildren.cend());
        }
    }

    return out;
}

const bool FileData::isArcadeAsset() const
{
    const std::string stem {Utils::FileSystem::getStem(mPath)};
    return ((mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) ||
                         mSystem->hasPlatformId(PlatformIds::SNK_NEO_GEO))) &&
            (MameNames::getInstance().isBios(stem) || MameNames::getInstance().isDevice(stem)));
}

const bool FileData::isArcadeGame() const
{
    const std::string stem {Utils::FileSystem::getStem(mPath)};
    return ((mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) ||
                         mSystem->hasPlatformId(PlatformIds::SNK_NEO_GEO))) &&
            (!MameNames::getInstance().isBios(stem) && !MameNames::getInstance().isDevice(stem)));
}

void FileData::addChild(FileData* file)
{
    assert(mType == FOLDER);
    if (!mSystem->getFlattenFolders())
        assert(file->getParent() == nullptr);

    const std::string key = file->getKey();
    if (mChildrenByFilename.find(key) == mChildrenByFilename.cend()) {
        mChildrenByFilename[key] = file;
        mChildren.emplace_back(file);
        file->mParent = this;
    }
}

void FileData::removeChild(FileData* file)
{
    assert(mType == FOLDER);
    assert(file->getParent() == this);
    mChildrenByFilename.erase(file->getKey());
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
        if (*it == file) {
            file->mParent = nullptr;
            mChildren.erase(it);
            return;
        }
    }

    // File somehow wasn't in our children.
    assert(false);
}

void FileData::sort(ComparisonFunction& comparator,
                    std::pair<unsigned int, unsigned int>& gameCount)
{
    mOnlyFolders = true;
    mHasFolders = false;
    bool foldersOnTop {Settings::getInstance()->getBool("FoldersOnTop")};
    bool showHiddenGames {Settings::getInstance()->getBool("ShowHiddenGames")};
    bool isKidMode {UIModeController::getInstance()->isUIModeKid()};
    std::vector<FileData*> mChildrenFolders;
    std::vector<FileData*> mChildrenOthers;

    if (mSystem->isGroupedCustomCollection())
        gameCount = {};

    if (!showHiddenGames) {
        for (auto it = mChildren.begin(); it != mChildren.end();) {
            // If the option to hide hidden games has been set and the game is hidden,
            // then skip it. Normally games are hidden during loading of the gamelists in
            // Gamelist::parseGamelist() and this code should only run when a user has marked
            // an entry manually as hidden. So upon the next application startup, this game
            // should be filtered already at that earlier point.
            if ((*it)->getHidden())
                it = mChildren.erase(it);
            // Also hide folders where all its entries have been hidden, unless it's a
            // grouped custom collection.
            else if ((*it)->getType() == FOLDER && (*it)->getChildren().size() == 0 &&
                     !(*it)->getSystem()->isGroupedCustomCollection())
                it = mChildren.erase(it);
            else
                ++it;
        }
    }

    // The main custom collections view is sorted during startup in CollectionSystemsManager.
    // The individual collections are however sorted as any normal systems/folders.
    if (mSystem->isCollection() && mSystem->getFullName() == "collections") {
        std::pair<unsigned int, unsigned int> tempGameCount {};
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
            if ((*it)->getChildren().size() > 0)
                (*it)->sort(comparator, gameCount);
            tempGameCount.first += gameCount.first;
            tempGameCount.second += gameCount.second;
            gameCount = {};
        }
        gameCount = tempGameCount;
        return;
    }

    if (foldersOnTop) {
        for (unsigned int i = 0; i < mChildren.size(); ++i) {
            if (mChildren[i]->getType() == FOLDER) {
                mChildrenFolders.emplace_back(mChildren[i]);
            }
            else {
                mChildrenOthers.emplace_back(mChildren[i]);
                mOnlyFolders = false;
            }
        }

        // If the requested sorting is not by filename, then sort in ascending filename order
        // as a first step, in order to get a correct secondary sorting.
        if (getSortTypeFromString("filename, ascending").comparisonFunction != comparator &&
            getSortTypeFromString("filename, descending").comparisonFunction != comparator) {
            std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(),
                             getSortTypeFromString("filename, ascending").comparisonFunction);
            std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(),
                             getSortTypeFromString("filename, ascending").comparisonFunction);
        }

        if (foldersOnTop && mOnlyFolders)
            std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(), comparator);

        std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(), comparator);

        mChildren.erase(mChildren.begin(), mChildren.end());
        mChildren.reserve(mChildrenFolders.size() + mChildrenOthers.size());
        mChildren.insert(mChildren.end(), mChildrenFolders.begin(), mChildrenFolders.end());
        mChildren.insert(mChildren.end(), mChildrenOthers.begin(), mChildrenOthers.end());
    }
    else {
        // If the requested sorting is not by filename, then sort in ascending filename order
        // as a first step, in order to get a correct secondary sorting.
        if (getSortTypeFromString("filename, ascending").comparisonFunction != comparator &&
            getSortTypeFromString("filename, descending").comparisonFunction != comparator)
            std::stable_sort(mChildren.begin(), mChildren.end(),
                             getSortTypeFromString("filename, ascending").comparisonFunction);

        std::stable_sort(mChildren.begin(), mChildren.end(), comparator);
    }

    for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
        // Game count, which will be displayed in the system view.
        if ((*it)->getType() == GAME && (*it)->getCountAsGame()) {
            if (!isKidMode || (isKidMode && (*it)->getKidgame())) {
                ++gameCount.first;
                if ((*it)->getFavorite())
                    ++gameCount.second;
            }
        }

        if ((*it)->getType() != FOLDER)
            mOnlyFolders = false;
        else
            mHasFolders = true;

        // Iterate through any child folders.
        if ((*it)->getChildren().size() > 0)
            (*it)->sort(comparator, gameCount);
    }

    if (mSystem->isGroupedCustomCollection())
        mGameCount = gameCount;
}

void FileData::sortFavoritesOnTop(ComparisonFunction& comparator,
                                  std::pair<unsigned int, unsigned int>& gameCount)
{
    mOnlyFolders = true;
    mHasFolders = false;
    bool foldersOnTop {Settings::getInstance()->getBool("FoldersOnTop")};
    bool showHiddenGames {Settings::getInstance()->getBool("ShowHiddenGames")};
    bool isKidMode {UIModeController::getInstance()->isUIModeKid()};
    std::vector<FileData*> mChildrenFolders;
    std::vector<FileData*> mChildrenFavoritesFolders;
    std::vector<FileData*> mChildrenFavorites;
    std::vector<FileData*> mChildrenOthers;

    if (mSystem->isGroupedCustomCollection())
        gameCount = {};

    // The main custom collections view is sorted during startup in CollectionSystemsManager.
    // The individual collections are however sorted as any normal systems/folders.
    if (mSystem->isCollection() && mSystem->getFullName() == "collections") {
        std::pair<unsigned int, unsigned int> tempGameCount = {};
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
            if ((*it)->getChildren().size() > 0)
                (*it)->sortFavoritesOnTop(comparator, gameCount);
            tempGameCount.first += gameCount.first;
            tempGameCount.second += gameCount.second;
            gameCount = {};
        }
        gameCount = tempGameCount;
        return;
    }

    for (unsigned int i = 0; i < mChildren.size(); ++i) {
        // If the option to hide hidden games has been set and the game is hidden,
        // then skip it. Normally games are hidden during loading of the gamelists in
        // Gamelist::parseGamelist() and this code should only run when a user has marked
        // an entry manually as hidden. So upon the next application startup, this game
        // should be filtered already at that earlier point.
        if (!showHiddenGames && mChildren[i]->getHidden())
            continue;
        // Also hide folders where all its entries have been hidden.
        else if (mChildren[i]->getType() == FOLDER && mChildren[i]->getChildren().size() == 0)
            continue;

        // Game count, which will be displayed in the system view.
        if (mChildren[i]->getType() == GAME && mChildren[i]->getCountAsGame()) {
            if (!isKidMode || (isKidMode && mChildren[i]->getKidgame())) {
                ++gameCount.first;
                if (mChildren[i]->getFavorite())
                    ++gameCount.second;
            }
        }

        if (foldersOnTop && mChildren[i]->getType() == FOLDER) {
            if (!mChildren[i]->getFavorite())
                mChildrenFolders.emplace_back(mChildren[i]);
            else
                mChildrenFavoritesFolders.emplace_back(mChildren[i]);
        }
        else if (mChildren[i]->getFavorite()) {
            mChildrenFavorites.emplace_back(mChildren[i]);
        }
        else {
            mChildrenOthers.emplace_back(mChildren[i]);
        }

        if (mChildren[i]->getType() != FOLDER)
            mOnlyFolders = false;
        else
            mHasFolders = true;
    }

    if (mSystem->isGroupedCustomCollection())
        mGameCount = gameCount;

    // If there are favorite folders and this is a mixed list, then don't handle these
    // separately but instead merge them into the same vector. This is a quite wasteful
    // approach but the scenario where a user has a mixed folder and files list and marks
    // some folders as favorites is probably a rare situation.
    if (!mOnlyFolders && mChildrenFavoritesFolders.size() > 0) {
        mChildrenFolders.insert(mChildrenFolders.end(), mChildrenFavoritesFolders.begin(),
                                mChildrenFavoritesFolders.end());
        mChildrenFavoritesFolders.erase(mChildrenFavoritesFolders.begin(),
                                        mChildrenFavoritesFolders.end());
        std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(),
                         getSortTypeFromString("filename, ascending").comparisonFunction);
    }

    // If the requested sorting is not by filename, then sort in ascending filename order
    // as a first step, in order to get a correct secondary sorting.
    if (getSortTypeFromString("filename, ascending").comparisonFunction != comparator &&
        getSortTypeFromString("filename, descending").comparisonFunction != comparator) {
        std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(),
                         getSortTypeFromString("filename, ascending").comparisonFunction);
        std::stable_sort(mChildrenFavoritesFolders.begin(), mChildrenFavoritesFolders.end(),
                         getSortTypeFromString("filename, ascending").comparisonFunction);
        std::stable_sort(mChildrenFavorites.begin(), mChildrenFavorites.end(),
                         getSortTypeFromString("filename, ascending").comparisonFunction);
        std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(),
                         getSortTypeFromString("filename, ascending").comparisonFunction);
    }

    // Sort favorite games and the other games separately.
    if (foldersOnTop && mOnlyFolders) {
        std::stable_sort(mChildrenFavoritesFolders.begin(), mChildrenFavoritesFolders.end(),
                         comparator);
        std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(), comparator);
    }
    std::stable_sort(mChildrenFavorites.begin(), mChildrenFavorites.end(), comparator);
    std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(), comparator);

    // Iterate through any child favorite folders.
    for (auto it = mChildrenFavoritesFolders.cbegin(); // Line break.
         it != mChildrenFavoritesFolders.cend(); ++it) {
        if ((*it)->getChildren().size() > 0)
            (*it)->sortFavoritesOnTop(comparator, gameCount);
    }

    // Iterate through any child folders.
    for (auto it = mChildrenFolders.cbegin(); it != mChildrenFolders.cend(); ++it) {
        if ((*it)->getChildren().size() > 0)
            (*it)->sortFavoritesOnTop(comparator, gameCount);
    }

    // If folders are not sorted on top, mChildrenFavoritesFolders and mChildrenFolders
    // could be empty. So due to this, step through all mChildren and see if there are
    // any folders that we need to iterate.
    if (mChildrenFavoritesFolders.size() == 0 && mChildrenFolders.size() == 0) {
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); ++it) {
            if ((*it)->getChildren().size() > 0)
                (*it)->sortFavoritesOnTop(comparator, gameCount);
        }
    }

    // Combine the individually sorted favorite games and other games vectors.
    mChildren.erase(mChildren.begin(), mChildren.end());
    mChildren.reserve(mChildrenFavoritesFolders.size() + mChildrenFolders.size() +
                      mChildrenFavorites.size() + mChildrenOthers.size());
    mChildren.insert(mChildren.end(), mChildrenFavoritesFolders.begin(),
                     mChildrenFavoritesFolders.end());
    mChildren.insert(mChildren.end(), mChildrenFolders.begin(), mChildrenFolders.end());
    mChildren.insert(mChildren.end(), mChildrenFavorites.begin(), mChildrenFavorites.end());
    mChildren.insert(mChildren.end(), mChildrenOthers.begin(), mChildrenOthers.end());
}

void FileData::sort(const SortType& type, bool mFavoritesOnTop)
{
    mGameCount = std::make_pair(0, 0);

    if (mFavoritesOnTop)
        sortFavoritesOnTop(*type.comparisonFunction, mGameCount);
    else
        sort(*type.comparisonFunction, mGameCount);

    updateLastPlayedList();
    updateMostPlayedList();
}

void FileData::countGames(std::pair<unsigned int, unsigned int>& gameCount)
{
    bool isKidMode {(Settings::getInstance()->getString("UIMode") == "kid" ||
                     Settings::getInstance()->getBool("ForceKid"))};

    for (unsigned int i = 0; i < mChildren.size(); ++i) {
        if (mChildren[i]->getType() == GAME && mChildren[i]->getCountAsGame()) {
            if (!isKidMode || (isKidMode && mChildren[i]->getKidgame())) {
                ++gameCount.first;
                if (mChildren[i]->getFavorite())
                    ++gameCount.second;
            }
        }
        // Iterate through any folders.
        else if (mChildren[i]->getType() == FOLDER)
            mChildren[i]->countGames(gameCount);
    }
    mGameCount = gameCount;
}

void FileData::updateLastPlayedList()
{
    if (mUpdateListCallback)
        mUpdateListCallback();

    if (!mUpdateChildrenLastPlayed)
        return;

    mChildrenLastPlayed.clear();
    mChildrenLastPlayed = getChildrenRecursive();

    std::stable_sort(mChildrenLastPlayed.begin(), mChildrenLastPlayed.end());
    std::sort(std::begin(mChildrenLastPlayed), std::end(mChildrenLastPlayed),
              [](FileData* a, FileData* b) {
                  return a->metadata.get("lastplayed") > b->metadata.get("lastplayed");
              });
}

void FileData::updateMostPlayedList()
{
    if (mUpdateListCallback)
        mUpdateListCallback();

    if (!mUpdateChildrenMostPlayed)
        return;

    mChildrenMostPlayed.clear();
    mChildrenMostPlayed = getChildrenRecursive();

    std::stable_sort(mChildrenMostPlayed.begin(), mChildrenMostPlayed.end());
    std::sort(std::begin(mChildrenMostPlayed), std::end(mChildrenMostPlayed),
              [](FileData* a, FileData* b) {
                  return a->metadata.getInt("playcount") > b->metadata.getInt("playcount");
              });
}

const FileData::SortType& FileData::getSortTypeFromString(const std::string& desc) const
{
    std::vector<FileData::SortType> SortTypes = FileSorts::SortTypes;

    for (unsigned int i = 0; i < FileSorts::SortTypes.size(); ++i) {
        const FileData::SortType& sort {FileSorts::SortTypes.at(i)};
        if (sort.description == desc)
            return sort;
    }
    // If no type was found then default to "filename, ascending".
    return FileSorts::SortTypes.at(0);
}

void FileData::launchGame()
{
    Window* window {Window::getInstance()};

    LOG(LogInfo) << "Launching game \"" << this->metadata.get("name") << "\"...";

    SystemData* gameSystem {nullptr};
    std::string command;
    std::string alternativeEmulator;

    if (mSystem->isCollection())
        gameSystem = SystemData::getSystemByName(mSystemName);
    else
        gameSystem = mSystem;

    // This is just a precaution as getSystemByName() should always return a valid result.
    if (gameSystem == nullptr)
        gameSystem = mSystem;

    alternativeEmulator = gameSystem->getAlternativeEmulator();

    // Check if there is a game-specific alternative emulator configured.
    // This takes precedence over any system-wide alternative emulator configuration.
    if (Settings::getInstance()->getBool("AlternativeEmulatorPerGame") &&
        !metadata.get("altemulator").empty()) {
        command = gameSystem->getLaunchCommandFromLabel(metadata.get("altemulator"));
        if (command == "") {
            LOG(LogWarning) << "Invalid alternative emulator \"" << metadata.get("altemulator")
                            << "\" configured for game";
        }
        else {
            LOG(LogDebug) << "FileData::launchGame(): Using alternative emulator \""
                          << metadata.get("altemulator")
                          << "\" as configured for the specific game";
        }
    }

    // Check if there is a system-wide alternative emulator configured.
    if (command == "" && alternativeEmulator != "") {
        command = gameSystem->getLaunchCommandFromLabel(alternativeEmulator);
        if (command == "") {
            LOG(LogWarning) << "Invalid alternative emulator \""
                            << alternativeEmulator.substr(9, alternativeEmulator.length() - 9)
                            << "\" configured for system \"" << gameSystem->getName() << "\"";
        }
        else {
            LOG(LogDebug) << "FileData::launchGame(): Using alternative emulator \""
                          << gameSystem->getAlternativeEmulator() << "\""
                          << " as configured for system \"" << gameSystem->getName() << "\"";
        }
    }

    if (command.empty())
        command = mEnvData->mLaunchCommands.front().first;

    std::string commandRaw {command};
    std::string romPath {Utils::FileSystem::getEscapedPath(mPath)};
    std::string baseName {Utils::FileSystem::getStem(mPath)};

    // For the special case where a directory has a supported file extension and is therefore
    // interpreted as a file, check if there is a matching filename inside the directory.
    // This is used as a shortcut to be able to launch games directly inside folders.
    if (mType == GAME && Utils::FileSystem::isDirectory(mPath)) {
        for (std::string& file : Utils::FileSystem::getDirContent(mPath)) {
            if (Utils::FileSystem::getFileName(file) == Utils::FileSystem::getFileName(mPath) &&
                (Utils::FileSystem::isRegularFile(file) || Utils::FileSystem::isSymlink(file))) {
                romPath = Utils::FileSystem::getEscapedPath(file);
                baseName = baseName.substr(0, baseName.find("."));
                break;
            }
        }
    }

    const std::string romRaw {Utils::FileSystem::getPreferredPath(mPath)};
    const std::string esPath {Utils::FileSystem::getExePath()};
    bool runInBackground {false};

    // In addition to the global RunInBackground setting it's possible to define this flag
    // per launch command in es_systems.xml.
    size_t inBackgroundPos {command.find("%RUNINBACKGROUND%")};

    if (inBackgroundPos != std::string::npos) {
        runInBackground = true;
        command = Utils::String::replace(command, "%RUNINBACKGROUND%", "");
        // Trim any leading whitespaces as they could cause the script execution to fail.
        command.erase(command.begin(), std::find_if(command.begin(), command.end(), [](char c) {
                          return !std::isspace(static_cast<unsigned char>(c));
                      }));
    }

    // The global setting always applies.
    if (Settings::getInstance()->getBool("RunInBackground"))
        runInBackground = true;

#if !defined(_WIN64)
    // Whether to parse .desktop files on Unix or open apps or alias files on macOS.
    bool isShortcut {false};
    size_t enableShortcutsPos {command.find("%ENABLESHORTCUTS%")};

    if (enableShortcutsPos != std::string::npos) {
#if defined(__APPLE__)
        if (Utils::FileSystem::getExtension(romRaw) == ".app")
#else
        if (Utils::FileSystem::getExtension(romRaw) == ".desktop")
#endif
            isShortcut = true;

        command = Utils::String::replace(command, "%ENABLESHORTCUTS%", "");
        // Trim any leading whitespaces as they could cause the script execution to fail.
        command.erase(command.begin(), std::find_if(command.begin(), command.end(), [](char c) {
                          return !std::isspace(static_cast<unsigned char>(c));
                      }));
    }
#endif

    std::string coreEntry;
    std::string coreName;
    size_t coreEntryPos {0};
    size_t coreFilePos {0};
    bool foundCoreFile {false};
    std::vector<std::string> emulatorCorePaths;

#if defined(_WIN64)
    bool hideWindow {false};

    // If the %HIDEWINDOW% variable is defined, we pass a flag to launchGameWindows() to
    // hide the window. This is intended primarily for hiding console windows when launching
    // scripts (used for example by Steam games and source ports).
    if (command.find("%HIDEWINDOW%") != std::string::npos) {
        hideWindow = true;
        command = Utils::String::replace(command, "%HIDEWINDOW%", "");
        // Trim any leading whitespaces as they could cause the script execution to fail.
        command.erase(command.begin(), std::find_if(command.begin(), command.end(), [](char c) {
                          return !std::isspace(static_cast<unsigned char>(c));
                      }));
    }

    bool escapeSpecials {false};

    // If calling scripts and links using some binaries like cmd.exe then the special characters
    // &()^=;, must be escaped.
    if (command.find("%ESCAPESPECIALS%") != std::string::npos) {
        escapeSpecials = true;
        command = Utils::String::replace(command, "%ESCAPESPECIALS%", "");
        // Trim any leading whitespaces as they could cause the script execution to fail.
        command.erase(command.begin(), std::find_if(command.begin(), command.end(), [](char c) {
                          return !std::isspace(static_cast<unsigned char>(c));
                      }));
    }
#endif

    // If there's a quotation mark before the %CORE_ variable, then remove it.
    // The closing quotation mark will be removed later below.
    bool hasCoreQuotation {false};
    if (command.find("\"%CORE_") != std::string::npos) {
        command = Utils::String::replace(command, "\"%CORE_", "%CORE_");
        hasCoreQuotation = true;
    }

    coreEntryPos = command.find("%CORE_");
    if (coreEntryPos != std::string::npos) {
        coreFilePos = command.find("%", coreEntryPos + 6);
        if (coreFilePos != std::string::npos)
            coreEntry = command.substr(coreEntryPos + 6, coreFilePos - (coreEntryPos + 6));
    }

    if (coreEntry != "")
        emulatorCorePaths = SystemData::sFindRules.get()->mCores[coreEntry].corePaths;

    // Expand home path if ~ is used.
    command = Utils::FileSystem::expandHomePath(command);

    // Check that the emulator binary actually exists, and if so, get its path.
    std::string binaryPath {findEmulatorPath(command)};

    // Hack to show an error message if there was no emulator entry in es_find_rules.xml.
    if (binaryPath.substr(0, 18) == "NO EMULATOR RULE: ") {
        std::string emulatorEntry {binaryPath.substr(18, binaryPath.size() - 18)};
        LOG(LogError) << "Couldn't launch game, either there is no emulator entry for \""
                      << emulatorEntry << "\" in es_find_rules.xml or there are no rules defined";
        LOG(LogError) << "Raw emulator launch command:";
        LOG(LogError) << commandRaw;

        window->queueInfoPopup(
            "ERROR: MISSING EMULATOR FIND RULES CONFIGURATION FOR '" + emulatorEntry + "'", 6000);
        window->setAllowTextScrolling(true);
        window->setAllowFileAnimation(true);
        return;
    }
    else if (binaryPath.empty()) {
        LOG(LogError) << "Couldn't launch game, emulator binary not found";
        LOG(LogError) << "Raw emulator launch command:";
        LOG(LogError) << commandRaw;

        std::string emulatorName;
        size_t startPos {0};
        size_t endPos {0};

        if ((startPos = command.find("%EMULATOR_")) != std::string::npos) {
            endPos = command.find("%", startPos + 1);
            if (endPos != std::string::npos)
                emulatorName = command.substr(startPos + 10, endPos - startPos - 10);
        }

        if (emulatorName == "")
            window->queueInfoPopup("ERROR: COULDN'T FIND EMULATOR, HAS IT BEEN PROPERLY INSTALLED?",
                                   6000);
        else
            window->queueInfoPopup("ERROR: COULDN'T FIND EMULATOR '" + emulatorName +
                                       "', HAS IT BEEN PROPERLY INSTALLED?",
                                   6000);

        window->setAllowTextScrolling(true);
        window->setAllowFileAnimation(true);
        return;
    }
#if defined(_WIN64)
    else {
        std::string binaryLogPath {Utils::String::replace(
            Utils::String::replace(binaryPath, "%ESPATH%", esPath), "/", "\\")};
        if (binaryLogPath.front() != '\"' && binaryLogPath.back() != '\"')
            binaryLogPath = "\"" + binaryLogPath + "\"";
        LOG(LogDebug) << "FileData::launchGame(): Found emulator binary " << binaryLogPath;
#else
    else if (!isShortcut) {
        LOG(LogDebug) << "FileData::launchGame(): Found emulator binary \""
                      << Utils::String::replace(binaryPath, "%ESPATH%", esPath) << "\"";
#endif
    }

    // If %EMUPATH% is used in es_systems.xml for this system, then check that the core
    // file actually exists.
    size_t emuPathPos {command.find("%EMUPATH%")};
    if (emuPathPos != std::string::npos) {
        bool hasQuotationMark {false};
        unsigned int quotationMarkPos {0};
        if (command.find("\"%EMUPATH%", emuPathPos - 1) != std::string::npos) {
            hasQuotationMark = true;
            quotationMarkPos =
                static_cast<unsigned int>(command.find("\"", emuPathPos + 9) - emuPathPos);
        }
        size_t spacePos {command.find(" ", emuPathPos + quotationMarkPos)};
        std::string coreRaw;
        std::string coreFile;
        if (spacePos != std::string::npos) {
            coreRaw = command.substr(emuPathPos, spacePos - emuPathPos);
#if defined(_WIN64)
            coreFile = Utils::FileSystem::getParent(Utils::String::replace(binaryPath, "\"", "")) +
                       command.substr(emuPathPos + 9, spacePos - emuPathPos - 9);
            coreFile = Utils::String::replace(coreFile, "/", "\\");
#else
            coreFile = Utils::FileSystem::getParent(binaryPath) +
                       command.substr(emuPathPos + 9, spacePos - emuPathPos - 9);
#endif
            if (hasQuotationMark) {
                coreRaw.pop_back();
                coreFile.pop_back();
            }
            if (!Utils::FileSystem::isRegularFile(coreFile) &&
                !Utils::FileSystem::isSymlink(coreFile)) {
                LOG(LogError) << "Couldn't launch game, emulator core file \""
                              << Utils::FileSystem::getFileName(coreFile) << "\" not found";
                LOG(LogError) << "Raw emulator launch command:";
                LOG(LogError) << commandRaw;

                window->queueInfoPopup(
                    "ERROR: COULDN'T FIND EMULATOR CORE FILE '" +
                        Utils::String::toUpper(Utils::FileSystem::getFileName(coreFile)) + "'",
                    6000);
                window->setAllowTextScrolling(true);
                window->setAllowFileAnimation(true);
                return;
            }
            else {
                if (hasQuotationMark) {
                    command = command.replace(emuPathPos + quotationMarkPos, 1, "");
                    --emuPathPos;
                    command = command.replace(emuPathPos, 1, "");
                }
                coreFile = Utils::FileSystem::getEscapedPath(coreFile);
                command = command.replace(emuPathPos, coreRaw.size(), coreFile);
            }
        }
        else {
            LOG(LogError) << "Invalid entry in systems configuration file es_systems.xml";
            LOG(LogError) << "Raw emulator launch command:";
            LOG(LogError) << commandRaw;

            window->queueInfoPopup("ERROR: INVALID ENTRY IN SYSTEMS CONFIGURATION FILE", 6000);
            window->setAllowTextScrolling(true);
            window->setAllowFileAnimation(true);
            return;
        }
    }

    // Error handling in case of no core find rule.
    if (coreEntry != "" && emulatorCorePaths.empty()) {
        LOG(LogError) << "Couldn't launch game, either there is no core entry for \"" << coreEntry
                      << "\" in es_find_rules.xml or there are no corepath rules defined";
        LOG(LogError) << "Raw emulator launch command:";
        LOG(LogError) << commandRaw;

        window->queueInfoPopup("ERROR: MISSING CORE CONFIGURATION FOR '" + coreEntry + "'", 6000);
        window->setAllowTextScrolling(true);
        window->setAllowFileAnimation(true);
        return;
    }

    // If a %CORE_ find rule entry is used in es_systems.xml for this system, then try to find
    // the emulator core using the rules defined in es_find_rules.xml.
    for (std::string path : emulatorCorePaths) {
        // The position of the %CORE_ variable could have changed as there may have been an
        // %EMULATOR_ variable that was substituted for the actual emulator binary.
        coreEntryPos = command.find("%CORE_");
        coreFilePos = command.find("%", coreEntryPos + 6);

        size_t separatorPos;
        size_t quotePos {command.find("\"", coreFilePos)};
        if (quotePos == std::string::npos)
            separatorPos = command.find(" ", coreFilePos);
        else
            separatorPos = quotePos;

        if (separatorPos != std::string::npos) {
            coreName = command.substr(coreFilePos + 2, separatorPos - (coreFilePos + 2));

#if defined(_WIN64)
            std::string coreFile {Utils::FileSystem::expandHomePath(path + "\\" + coreName)};
#else
            std::string coreFile {Utils::FileSystem::expandHomePath(path + "/" + coreName)};
#endif

            // Expand %EMUPATH% if it has been used in the %CORE_ variable.
            size_t stringPos {coreFile.find("%EMUPATH%")};
            if (stringPos != std::string::npos) {
#if defined(_WIN64)
                coreFile = coreFile.replace(
                    stringPos, 9,
                    Utils::FileSystem::getParent(Utils::String::replace(binaryPath, "\"", "")));
                coreFile = Utils::String::replace(coreFile, "/", "\\");
#else
                coreFile = coreFile.replace(stringPos, 9, Utils::FileSystem::getParent(binaryPath));
#endif
            }

            // Expand %ESPATH% if it has been used in the %CORE_ variable.
            stringPos = coreFile.find("%ESPATH%");
            if (stringPos != std::string::npos) {
                coreFile = coreFile.replace(stringPos, 8, esPath);
#if defined(_WIN64)
                coreFile = Utils::String::replace(coreFile, "/", "\\");
#endif
            }

            if (Utils::FileSystem::isRegularFile(coreFile) ||
                Utils::FileSystem::isSymlink(coreFile)) {
                foundCoreFile = true;
                // Escape any blankspaces.
                if (coreFile.find(" ") != std::string::npos)
                    coreFile = Utils::FileSystem::getEscapedPath(coreFile);
                command.replace(coreEntryPos,
                                separatorPos - coreEntryPos + (hasCoreQuotation ? 1 : 0), coreFile);
#if !defined(_WIN64)
                // Remove any quotation marks as it would make the launch function fail.
                if (command.find("\"") != std::string::npos)
                    command = Utils::String::replace(command, "\"", "");
#endif
                break;
            }
        }
        else {
            LOG(LogError) << "Invalid entry in systems configuration file es_systems.xml";
            LOG(LogError) << "Raw emulator launch command:";
            LOG(LogError) << commandRaw;

            window->queueInfoPopup("ERROR: INVALID ENTRY IN SYSTEMS CONFIGURATION FILE", 6000);
            window->setAllowTextScrolling(true);
            window->setAllowFileAnimation(true);
            return;
        }
    }
    if (!foundCoreFile && coreName.size() > 0) {
        LOG(LogError) << "Couldn't launch game, emulator core file \""
                      << coreName.substr(0, coreName.size()) << "\" not found";
        LOG(LogError) << "Raw emulator launch command:";
        LOG(LogError) << commandRaw;
        LOG(LogError)
            << "Tried to find the core file using these paths as defined by es_find_rules.xml:";
        LOG(LogError) << Utils::String::vectorToDelimitedString(emulatorCorePaths, ", ");

        window->queueInfoPopup(
            "ERROR: COULDN'T FIND EMULATOR CORE FILE '" +
                Utils::String::toUpper(coreName.substr(0, coreName.size()) + "'"),
            6000);
        window->setAllowTextScrolling(true);
        window->setAllowFileAnimation(true);
        return;
    }

    std::string startDirectory;
    size_t startDirPos {command.find("%STARTDIR%")};

    if (startDirPos != std::string::npos) {
        bool invalidEntry {false};

        if (startDirPos + 12 >= command.size())
            invalidEntry = true;
        else if (command[startDirPos + 10] != '=')
            invalidEntry = true;

        if (!invalidEntry && command[startDirPos + 11] == '\"') {
            size_t closingQuotation {command.find("\"", startDirPos + 12)};

            if (closingQuotation != std::string::npos) {
                startDirectory =
                    command.substr(startDirPos + 12, closingQuotation - startDirPos - 12);
                command = command.replace(startDirPos, closingQuotation - startDirPos + 2, "");
            }
            else {
                invalidEntry = true;
            }
        }
        else if (!invalidEntry) {
            size_t spacePos {command.find(" ", startDirPos)};
            if (spacePos != std::string::npos) {
                startDirectory = command.substr(startDirPos + 11, spacePos - startDirPos - 11);
                command = command.replace(startDirPos, spacePos - startDirPos + 1, "");
            }
            else {
                startDirectory = command.substr(startDirPos + 11, command.size() - startDirPos);
                command = command.replace(startDirPos, command.size() - startDirPos, "");
            }
        }

        if (invalidEntry) {
            LOG(LogError) << "Couldn't launch game, invalid %STARTDIR% entry";
            LOG(LogError) << "Raw emulator launch command:";
            LOG(LogError) << commandRaw;

            window->queueInfoPopup("ERROR: INVALID %STARTDIR% VARIABLE ENTRY", 6000);
            window->setAllowTextScrolling(true);
            window->setAllowFileAnimation(true);
            return;
        }

        if (startDirectory != "") {
            startDirectory = Utils::FileSystem::expandHomePath(startDirectory);
#if defined(_WIN64)
            startDirectory = Utils::String::replace(
                startDirectory, "%EMUDIR%",
                Utils::FileSystem::getParent(Utils::String::replace(binaryPath, "\"", "")));

            startDirectory = Utils::String::replace(
                startDirectory, "%GAMEDIR%",
                Utils::FileSystem::getParent(Utils::String::replace(romPath, "\"", "")));
#else
            startDirectory = Utils::String::replace(
                startDirectory, "%EMUDIR%",
                Utils::FileSystem::getParent(Utils::String::replace(binaryPath, "\\", "")));

            startDirectory = Utils::String::replace(
                startDirectory, "%GAMEDIR%",
                Utils::FileSystem::getParent(Utils::String::replace(romPath, "\\", "")));
#endif
            if (!Utils::FileSystem::isDirectory(startDirectory)) {
                Utils::FileSystem::createDirectory(startDirectory);

                if (!Utils::FileSystem::isDirectory(startDirectory)) {
                    LOG(LogError)
                        << "Couldn't launch game, directory defined by %STARTDIR% could not be "
                           "created, permission problems?";
                    LOG(LogError) << "Raw emulator launch command:";
                    LOG(LogError) << commandRaw;

                    window->queueInfoPopup("ERROR: DIRECTORY DEFINED BY %STARTDIR% COULD NOT BE "
                                           "CREATED, PERMISSION PROBLEMS?",
                                           6000);
                    window->setAllowTextScrolling(true);
                    window->setAllowFileAnimation(true);
                    return;
                }
            }
#if defined(_WIN64)
            startDirectory = Utils::String::replace(startDirectory, "/", "\\");
#else
            startDirectory = Utils::FileSystem::getEscapedPath(startDirectory);
#endif
            LOG(LogDebug) << "FileData::launchGame(): Setting start directory to \""
                          << startDirectory << "\"";
        }
    }

    std::string injectFile;
    const size_t injectPos {command.find("%INJECT%")};

    if (injectPos != std::string::npos) {
        bool invalidEntry {false};

        if (injectPos + 10 >= command.size())
            invalidEntry = true;
        else if (command[injectPos + 8] != '=')
            invalidEntry = true;

        if (!invalidEntry && command[injectPos + 9] == '\"') {
            size_t closingQuotation {command.find("\"", injectPos + 10)};

            if (closingQuotation != std::string::npos) {
                injectFile = command.substr(injectPos + 10, closingQuotation - injectPos - 10);
                command = command.replace(injectPos, closingQuotation - injectPos + 2, "");
            }
            else {
                invalidEntry = true;
            }
        }
        else if (!invalidEntry) {
            size_t spacePos {command.find(" ", injectPos)};
            if (spacePos != std::string::npos) {
                injectFile = command.substr(injectPos + 9, spacePos - injectPos - 9);
                command = command.replace(injectPos, spacePos - injectPos + 1, "");
            }
            else {
                injectFile = command.substr(injectPos + 9, command.size() - injectPos);
                command = command.replace(injectPos, command.size() - injectPos, "");
            }
        }

        if (invalidEntry) {
            LOG(LogError) << "Couldn't launch game, invalid %INJECT% entry";
            LOG(LogError) << "Raw emulator launch command:";
            LOG(LogError) << commandRaw;

            window->queueInfoPopup("ERROR: INVALID %INJECT% VARIABLE ENTRY", 6000);
            window->setAllowTextScrolling(true);
            window->setAllowFileAnimation(true);
            return;
        }
    }

    if (injectFile != "") {
#if defined(_WIN64)
        injectFile = Utils::String::replace(injectFile, "\\", "/");
        injectFile = Utils::String::replace(injectFile, "%BASENAME%",
                                            Utils::String::replace(baseName, "\"", ""));
        if (injectFile.size() < 3 || !(injectFile[1] == ':' && injectFile[2] == '/'))
            injectFile = Utils::FileSystem::getParent(Utils::String::replace(romPath, "\"", "")) +
                         "/" + injectFile;
        injectFile = Utils::String::replace(injectFile, "/", "\\");
#else
        injectFile = Utils::String::replace(injectFile, "%BASENAME%",
                                            Utils::String::replace(baseName, "\\", ""));
        if (injectFile.front() != '/')
            injectFile = Utils::FileSystem::getParent(Utils::String::replace(romPath, "\\", "")) +
                         "/" + injectFile;
#endif
        if (Utils::FileSystem::isRegularFile(injectFile) ||
            Utils::FileSystem::isSymlink(injectFile)) {
            LOG(LogDebug) << "FileData::launchGame(): Injecting arguments from file \""
                          << injectFile << "\"";
            std::string arguments;
            std::ifstream injectFileStream;
            injectFileStream.open(injectFile);
            for (std::string line; getline(injectFileStream, line);)
                arguments += line;
            injectFileStream.close();

            if (arguments.empty()) {
                LOG(LogDebug) << "FileData::launchGame(): File empty or insufficient permissions, "
                                 "nothing to inject";
            }
            else if (arguments.size() > 4096) {
                LOG(LogWarning)
                    << "FileData::launchGame(): Arguments file exceeding maximum allowed size of "
                       "4096 bytes, skipping injection";
            }
            else {
                command.insert(injectPos, arguments + " ");
            }
        }
        else {
            LOG(LogDebug) << "FileData::launchGame(): Arguments file \"" << injectFile
                          << "\" does not exist, skipping injection";
        }
    }

#if defined(_WIN64)
    if (escapeSpecials) {
        bool foundSpecial {false};

        // The special characters need to be procesed in this order.
        std::string specialCharacters {"^&()=;,"};

        for (size_t i = 0; i < specialCharacters.size(); ++i) {
            std::string special(1, specialCharacters[i]);
            if (romPath.find(special) != std::string::npos) {
                romPath = Utils::String::replace(romPath, special, "^" + special);
                foundSpecial = true;
            }
        }

        if (foundSpecial)
            romPath = Utils::String::replace(romPath, " ", "^ ");
    }
#endif

#if !defined(_WIN64)
#if defined(__APPLE__)
    if (isShortcut) {
        if (Utils::FileSystem::exists(Utils::String::replace(romPath, "\\", ""))) {
            LOG(LogInfo) << "Opening app or alias file \""
                         << Utils::String::replace(romPath, "\\", "") << "\"";
            command = Utils::String::replace(command, binaryPath, "open -W -a");
        }
        else {
            LOG(LogError) << "App or alias file \"" << romPath
                          << "\" doesn't exist or is unreadable";
            window->queueInfoPopup("ERROR: APP OR ALIAS FILE DOESN'T EXIST OR IS UNREADABLE", 6000);
            window->setAllowTextScrolling(true);
            window->setAllowFileAnimation(true);
            return;
        }
    }
#else
    if (isShortcut) {
        if (Utils::FileSystem::exists(Utils::String::replace(romPath, "\\", "")) &&
            !Utils::FileSystem::isDirectory(Utils::String::replace(romPath, "\\", ""))) {
            LOG(LogInfo) << "Parsing desktop file \"" << Utils::String::replace(romPath, "\\", "")
                         << "\"";
            bool validFile {false};
            bool execEntry {false};
            std::ifstream desktopFileStream;
            desktopFileStream.open(Utils::String::replace(romPath, "\\", ""));
            for (std::string line; getline(desktopFileStream, line);) {
                if (line.find("[Desktop Entry]") != std::string::npos)
                    validFile = true;
                if (line.substr(0, 5) == "Exec=") {
                    romPath = {line.substr(5, line.size() - 5)};
                    romPath = Utils::String::replace(romPath, "%F", "");
                    romPath = Utils::String::replace(romPath, "%f", "");
                    romPath = Utils::String::replace(romPath, "%U", "");
                    romPath = Utils::String::replace(romPath, "%u", "");
                    romPath = Utils::String::trim(romPath);
                    command = Utils::String::replace(command, binaryPath, "");
                    execEntry = true;
                    break;
                }
            }
            desktopFileStream.close();
            if (!validFile || !execEntry) {
                LOG(LogError) << "File is invalid or unreadable";
                window->queueInfoPopup("ERROR: DESKTOP FILE IS INVALID OR UNREADABLE", 6000);
                window->setAllowTextScrolling(true);
                window->setAllowFileAnimation(true);
                return;
            }
        }
        else {
            LOG(LogError) << "Desktop file \"" << romPath << "\" doesn't exist or is unreadable";
            window->queueInfoPopup("ERROR: DESKTOP FILE DOESN'T EXIST OR IS UNREADABLE", 6000);
            window->setAllowTextScrolling(true);
            window->setAllowFileAnimation(true);
            return;
        }
    }
#endif
#endif

    // Replace the remaining variables with their actual values.
    command = Utils::String::replace(command, "%ROM%", romPath);
    command = Utils::String::replace(command, "%BASENAME%", baseName);
    command = Utils::String::replace(command, "%ROMRAW%", romRaw);
    command = Utils::String::replace(command, "%ROMPATH%",
                                     Utils::FileSystem::getEscapedPath(getROMDirectory()));
#if defined(_WIN64)
    command = Utils::String::replace(
        command, "%ESPATH%", Utils::String::replace(Utils::FileSystem::getExePath(), "/", "\\"));
    command = Utils::String::replace(command, "%EMUDIR%",
                                     Utils::FileSystem::getEscapedPath(Utils::FileSystem::getParent(
                                         Utils::String::replace(binaryPath, "\"", ""))));
    command = Utils::String::replace(command, "%GAMEDIR%",
                                     Utils::FileSystem::getEscapedPath(Utils::FileSystem::getParent(
                                         Utils::String::replace(romPath, "\"", ""))));
#else
    command = Utils::String::replace(command, "%ESPATH%", Utils::FileSystem::getExePath());
    command = Utils::String::replace(command, "%EMUDIR%",
                                     Utils::FileSystem::getEscapedPath(Utils::FileSystem::getParent(
                                         Utils::String::replace(binaryPath, "\\", ""))));
    command = Utils::String::replace(command, "%GAMEDIR%",
                                     Utils::FileSystem::getEscapedPath(Utils::FileSystem::getParent(
                                         Utils::String::replace(romPath, "\\", ""))));
#endif

    // Trim any leading and trailing whitespace characters as they could cause launch issues.
    command = Utils::String::trim(command);

#if defined(_WIN64)
    // Hack to be able to surround paths with quotation marks when using the %ROMPATH% and
    // %EMUDIR% variables.
    command = Utils::String::replace(command, "\"\"", "");
#endif

    // swapBuffers() is called here to turn the screen black to eliminate some potential
    // flickering and to avoid showing the game launch message briefly when returning
    // from the game.
    if (!runInBackground)
        Renderer::getInstance()->swapBuffers();

    Scripting::fireEvent("game-start", romPath, getSourceFileData()->metadata.get("name"),
                         getSourceFileData()->getSystem()->getName(),
                         getSourceFileData()->getSystem()->getFullName());
    int returnValue {0};

    LOG(LogDebug) << "Raw emulator launch command:";
    LOG(LogDebug) << commandRaw;
    LOG(LogInfo) << "Expanded emulator launch command:";
    LOG(LogInfo) << command;

#if defined(FLATPAK_BUILD)
    // Break out of the sandbox.
    command = "flatpak-spawn --host " + command;
#endif

    // Possibly keep ES-DE running in the background while the game is launched.

#if defined(_WIN64)
    returnValue = Utils::Platform::launchGameWindows(
        Utils::String::stringToWideString(command),
        Utils::String::stringToWideString(startDirectory), runInBackground, hideWindow);
#else
    returnValue = Utils::Platform::launchGameUnix(command, startDirectory, runInBackground);
#endif
    // Notify the user in case of a failed game launch using a popup window.
    if (returnValue != 0) {
        LOG(LogWarning) << "...launch terminated with nonzero return value " << returnValue;

        window->queueInfoPopup("ERROR LAUNCHING GAME '" +
                                   Utils::String::toUpper(metadata.get("name")) + "' (ERROR CODE " +
                                   Utils::String::toUpper(std::to_string(returnValue) + ")"),
                               6000);
        window->setAllowTextScrolling(true);
        window->setAllowFileAnimation(true);
    }
    else {
        // Stop showing the game launch notification.
        window->stopInfoPopup();
#if defined(_WIN64)
        // If the RunInBackground setting has been enabled or if the %RUNINBACKGROUND% variable has
        // been set for the specific launch command, then block the video player, stop scrolling
        // game names and descriptions and keep the screensaver from getting activated.
        if (runInBackground)
            window->setLaunchedGame(true);
        else
            // Normalize deltaTime so that the screensaver does not start immediately
            // when returning from the game.
            window->normalizeNextUpdate();
#else
        // For some game systems we need to keep ES-DE running while the game is launched.
        // This blocks the video player, stops the scrolling of game names and descriptions and
        // keeps the screensaver from getting activated.
        if (runInBackground)
            window->setLaunchedGame(true);
        // Normalize deltaTime so that the screensaver does not start immediately
        // when returning from the game.
        window->normalizeNextUpdate();
#endif
    }

    // If running in the background then don't trigger the game-end event, which will instead be
    // triggered in ViewController when manually waking up the application.
    if (!runInBackground) {
        Scripting::fireEvent("game-end", romPath, getSourceFileData()->metadata.get("name"),
                             getSourceFileData()->getSystem()->getName(),
                             getSourceFileData()->getSystem()->getFullName());
    }
    else {
        std::vector<std::string>& gameEndParams {
            ViewController::getInstance()->getGameEndEventParams()};
        gameEndParams.emplace_back("game-end");
        gameEndParams.emplace_back(romPath);
        gameEndParams.emplace_back(getSourceFileData()->metadata.get("name"));
        gameEndParams.emplace_back(getSourceFileData()->getSystem()->getName());
        gameEndParams.emplace_back(getSourceFileData()->getSystem()->getFullName());
    }

    // Unless we're running in the background while the game is launched, re-enable the text
    // scrolling that was disabled in ViewController.
    if (!runInBackground) {
        window->setAllowTextScrolling(true);
        window->setAllowFileAnimation(true);
    }

    // Update number of times the game has been launched.
    FileData* gameToUpdate {getSourceFileData()};

    int timesPlayed {gameToUpdate->metadata.getInt("playcount") + 1};
    gameToUpdate->metadata.set("playcount", std::to_string(static_cast<long long>(timesPlayed)));

    // Update last played time.
    gameToUpdate->metadata.set("lastplayed", Utils::Time::DateTime(Utils::Time::now()));

    // If the cursor is on a folder then a folder link must have been configured, so set the
    // lastplayed timestamp for this folder to the same as the launched game.
    FileData* cursor {
        ViewController::getInstance()->getGamelistView(gameToUpdate->getSystem())->getCursor()};
    if (cursor->getType() == FOLDER)
        cursor->metadata.set("lastplayed", gameToUpdate->metadata.get("lastplayed"));

    // If the parent is a folder and it's not the root of the system, then update its lastplayed
    // timestamp to the same time as the game that was just launched.
    if (gameToUpdate->getParent()->getType() == FOLDER &&
        gameToUpdate->getParent()->getName() != gameToUpdate->getSystem()->getFullName()) {
        gameToUpdate->getParent()->metadata.set("lastplayed",
                                                gameToUpdate->metadata.get("lastplayed"));
    }

    CollectionSystemsManager::getInstance()->refreshCollectionSystems(gameToUpdate);

    gameToUpdate->mSystem->onMetaDataSavePoint();
}

const std::string FileData::findEmulatorPath(std::string& command)
{
    // Extract the emulator executable from the launch command string. There are two ways
    // that the emulator can be defined in es_systems.xml, either using the find rules in
    // es_find_rules.xml or via the explicit emulator binary name. In the former case, we
    // need to process any configured systempath and staticpath rules (and for Windows also
    // winregistrypath and winregistryvalue rules), and in the latter case we simply search
    // for the emulator binary in the system path.

    std::string emuExecutable;
    std::string exePath;

    // Method 1, emulator binary is defined using find rules:

#if defined(_WIN64)
    std::vector<std::string> emulatorWinRegistryPaths;
    std::vector<std::string> emulatorWinRegistryValues;
#endif
    std::vector<std::string> emulatorSystemPaths;
    std::vector<std::string> emulatorStaticPaths;
    std::string emulatorEntry;
    size_t startPos {0};
    size_t endPos {0};

    if ((startPos = command.find("%EMULATOR_")) != std::string::npos) {
        endPos = command.find("%", startPos + 1);
        if (endPos != std::string::npos)
            emulatorEntry = command.substr(startPos + 10, endPos - startPos - 10);
    }

    if (emulatorEntry != "") {
#if defined(_WIN64)
        emulatorWinRegistryPaths =
            SystemData::sFindRules.get()->mEmulators[emulatorEntry].winRegistryPaths;
        emulatorWinRegistryValues =
            SystemData::sFindRules.get()->mEmulators[emulatorEntry].winRegistryValues;
#endif
        emulatorSystemPaths = SystemData::sFindRules.get()->mEmulators[emulatorEntry].systemPaths;
        emulatorStaticPaths = SystemData::sFindRules.get()->mEmulators[emulatorEntry].staticPaths;
    }

    // Error handling in case of no emulator find rule.
#if defined(_WIN64)
    if (emulatorEntry != "" && emulatorWinRegistryPaths.empty() &&
        emulatorWinRegistryValues.empty() && emulatorSystemPaths.empty() &&
        emulatorStaticPaths.empty())
#else
    if (emulatorEntry != "" && emulatorSystemPaths.empty() && emulatorStaticPaths.empty())
#endif
        return "NO EMULATOR RULE: " + emulatorEntry;

#if defined(_WIN64)
    for (std::string path : emulatorWinRegistryPaths) {
        // Search for the emulator using the App Paths keys in the Windows Registry.
        std::string registryKeyPath {"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\" +
                                     path};

        HKEY registryKey;
        LSTATUS keyStatus {-1};
        LSTATUS pathStatus {-1};
        char registryPath[1024] {};
        DWORD pathSize {1024};

        // First look in HKEY_CURRENT_USER.
        keyStatus = RegOpenKeyEx(HKEY_CURRENT_USER, registryKeyPath.c_str(), 0, KEY_QUERY_VALUE,
                                 &registryKey);

        // If not found, then try in HKEY_LOCAL_MACHINE.
        if (keyStatus != ERROR_SUCCESS) {
            keyStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, registryKeyPath.c_str(), 0,
                                     KEY_QUERY_VALUE, &registryKey);
        }

        // If the key exists, then try to retrieve its default value.
        if (keyStatus == ERROR_SUCCESS) {
            pathStatus = RegGetValue(registryKey, nullptr, nullptr, RRF_RT_REG_SZ, nullptr,
                                     &registryPath, &pathSize);
        }
        else {
            RegCloseKey(registryKey);
            continue;
        }

        // That a value was found does not guarantee that the emulator binary actually exists,
        // so check for that as well.
        if (pathStatus == ERROR_SUCCESS) {
            if (Utils::FileSystem::isRegularFile(registryPath) ||
                Utils::FileSystem::isSymlink(registryPath)) {
                exePath = Utils::FileSystem::getEscapedPath(registryPath);
                command.replace(startPos, endPos - startPos + 1, exePath);
                RegCloseKey(registryKey);
                return exePath;
            }
        }
        RegCloseKey(registryKey);
    }

    for (std::string value : emulatorWinRegistryValues) {
        // If the pipe character is found, then the string following this should be appended
        // to the key value, assuming the key is found.
        std::string appendString;
        size_t pipePos {value.find('|')};

        if (pipePos != std::string::npos) {
            appendString = value.substr(pipePos + 1, std::string::npos);
            value = value.substr(0, pipePos);
        }

        // Search for the defined value in the Windows Registry.
        std::string registryValueKey {
            Utils::String::replace(Utils::FileSystem::getParent(value), "/", "\\")};
        std::string registryValue {Utils::FileSystem::getFileName(value)};

        HKEY registryKey;
        LSTATUS keyStatus {-1};
        LSTATUS pathStatus {-1};
        char path[1024] {};
        DWORD pathSize {1024};

        // First look in HKEY_CURRENT_USER.
        keyStatus = RegOpenKeyEx(HKEY_CURRENT_USER, registryValueKey.c_str(), 0, KEY_QUERY_VALUE,
                                 &registryKey);

        // If not found, then try in HKEY_LOCAL_MACHINE.
        if (keyStatus != ERROR_SUCCESS) {
            keyStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, registryValueKey.c_str(), 0,
                                     KEY_QUERY_VALUE, &registryKey);
        }

        // If the key exists, then try to retrieve the defined value.
        if (keyStatus == ERROR_SUCCESS) {
            pathStatus =
                RegGetValue(registryKey, nullptr, reinterpret_cast<LPCSTR>(registryValue.c_str()),
                            RRF_RT_REG_SZ, nullptr, &path, &pathSize);
        }
        else {
            RegCloseKey(registryKey);
            continue;
        }

        if (strlen(path) == 0) {
            RegCloseKey(registryKey);
            continue;
        }

        if (!appendString.empty())
            strncat_s(path, 1024, appendString.c_str(), appendString.length());

        // That a value was found does not guarantee that the emulator binary actually exists,
        // so check for that as well.
        if (pathStatus == ERROR_SUCCESS) {
            if (Utils::FileSystem::isRegularFile(path) || Utils::FileSystem::isSymlink(path)) {
                exePath = Utils::FileSystem::getEscapedPath(path);
                command.replace(startPos, endPos - startPos + 1, exePath);
                RegCloseKey(registryKey);
                return exePath;
            }
        }
        RegCloseKey(registryKey);
    }
#endif

    for (std::string path : emulatorSystemPaths) {
#if defined(_WIN64)
        std::wstring pathWide {Utils::String::stringToWideString(path)};
        // Search for the emulator using the PATH environment variable.
        DWORD size {SearchPathW(nullptr, pathWide.c_str(), L".exe", 0, nullptr, nullptr)};

        if (size) {
            std::vector<wchar_t> pathBuffer(static_cast<size_t>(size) + 1);
            wchar_t* fileName {nullptr};

            SearchPathW(nullptr, pathWide.c_str(), L".exe", size + 1, pathBuffer.data(), &fileName);
            std::wstring pathString {pathBuffer.data()};

            if (pathString.length()) {
                exePath = Utils::String::wideStringToString(
                    pathString.substr(0, pathString.size() - std::wstring(fileName).size()));
                exePath.pop_back();
            }
        }
        if (exePath != "") {
            exePath += "\\" + path;
            exePath = Utils::FileSystem::getEscapedPath(exePath);
            command.replace(startPos, endPos - startPos + 1, exePath);
            return exePath;
        }
#else
        exePath = Utils::FileSystem::getPathToBinary(path);
        if (exePath != "") {
            exePath += "/" + path;
            command.replace(startPos, endPos - startPos + 1, exePath);
            return exePath;
        }
#endif
    }

    for (std::string path : emulatorStaticPaths) {
        path = Utils::FileSystem::expandHomePath(path);
        // If %ESPATH% is used for the rule, then expand it to the binary directory of ES-DE.
        path = Utils::String::replace(path, "%ESPATH%", Utils::FileSystem::getExePath());
        // Likewise for the %ROMPATH% variable which expands to the configured ROM directory.
        path = Utils::String::replace(path, "%ROMPATH%", getROMDirectory());

        // Find the first matching file if a wildcard was used for the emulator entry.
        if (path.find('*') != std::string::npos) {
#if defined(_WIN64)
            Utils::FileSystem::StringList files {
                Utils::FileSystem::getMatchingFiles(Utils::String::replace(path, "\\", "/"))};
            if (files.size() > 0)
                path = Utils::String::replace(files.front(), "/", "\\");
#else
            Utils::FileSystem::StringList files {Utils::FileSystem::getMatchingFiles(path)};
            if (files.size() > 0)
                path = files.front();
#endif
        }

        if (Utils::FileSystem::isRegularFile(path) || Utils::FileSystem::isSymlink(path)) {
            exePath = Utils::FileSystem::getEscapedPath(path);
            command.replace(startPos, endPos - startPos + 1, exePath);
            return exePath;
        }
    }

    // Method 2, exact emulator binary name:

    // If %ESPATH% is used, then expand it to the binary directory of ES-DE.
    command = Utils::String::replace(command, "%ESPATH%", Utils::FileSystem::getExePath());

    // If the first character is a quotation mark, then we need to extract up to the
    // next quotation mark, otherwise we'll only extract up to the first space character.
    if (command.front() == '\"') {
        std::string emuTemp {command.substr(1, std::string::npos)};
        emuExecutable = emuTemp.substr(0, emuTemp.find('"'));
    }
    else {
        emuExecutable = command.substr(0, command.find(' '));
    }

    if (emuExecutable.find('*') != std::string::npos) {
        Utils::FileSystem::StringList files {Utils::FileSystem::getMatchingFiles(emuExecutable)};
        if (files.size() > 0) {
            command = Utils::String::replace(command, emuExecutable, files.front());
            emuExecutable = files.front();
        }
    }

#if defined(_WIN64)
    std::wstring emuExecutableWide {Utils::String::stringToWideString(emuExecutable)};
    // Search for the emulator using the PATH environment variable.
    DWORD size {SearchPathW(nullptr, emuExecutableWide.c_str(), L".exe", 0, nullptr, nullptr)};

    if (size) {
        std::vector<wchar_t> pathBuffer(static_cast<size_t>(size) + 1);
        wchar_t* fileName {nullptr};

        SearchPathW(nullptr, emuExecutableWide.c_str(), L".exe", size + 1, pathBuffer.data(),
                    &fileName);

        exePath = Utils::String::wideStringToString(pathBuffer.data());
    }
#else
    if (Utils::FileSystem::isRegularFile(emuExecutable) ||
        Utils::FileSystem::isSymlink(emuExecutable)) {
        exePath = Utils::FileSystem::getEscapedPath(emuExecutable);
    }
    else {
        exePath =
            Utils::FileSystem::getEscapedPath(Utils::FileSystem::getPathToBinary(emuExecutable));
        if (exePath != "")
            exePath += "/" + emuExecutable;
    }
#endif

    return exePath;
}

CollectionFileData::CollectionFileData(FileData* file, SystemData* system)
    : FileData(file->getSourceFileData()->getType(),
               file->getSourceFileData()->getPath(),
               file->getSourceFileData()->getSystemEnvData(),
               system)
{
    // We use this constructor to create a clone of the filedata, and change its system.
    mSourceFileData = file->getSourceFileData();
    refreshMetadata();
    mParent = nullptr;
    metadata = mSourceFileData->metadata;
    mSystemName = mSourceFileData->getSystem()->getName();
}

CollectionFileData::~CollectionFileData()
{
    // Need to remove collection file data at the collection object destructor.
    if (mParent)
        mParent->removeChild(this);
    mParent = nullptr;
}

void CollectionFileData::refreshMetadata()
{
    metadata = mSourceFileData->metadata;
    mDirty = true;
}

const std::string& CollectionFileData::getName()
{
    if (mDirty) {
        mCollectionFileName = mSourceFileData->metadata.get("name");
        mCollectionFileName.append(" [")
            .append(Utils::String::toUpper(mSourceFileData->getSystem()->getName()))
            .append("]");
        mDirty = false;
    }

    if (Settings::getInstance()->getBool("CollectionShowSystemInfo"))
        return mCollectionFileName;

    return mSourceFileData->metadata.get("name");
}

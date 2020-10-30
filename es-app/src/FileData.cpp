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

#include "guis/GuiInfoPopup.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "AudioManager.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Log.h"
#include "MameNames.h"
#include "Platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "Window.h"

#include <assert.h>

FileData::FileData(
        FileType type,
        const std::string& path,
        SystemEnvironmentData* envData,
        SystemData* system)
        : mType(type),
        mPath(path),
        mSystem(system),
        mEnvData(envData),
        mSourceFileData(nullptr),
        mParent(nullptr),
        mOnlyFolders(false),
        mDeletionFlag(false),
        // Metadata is set in the constructor.
        metadata(type == GAME ? GAME_METADATA : FOLDER_METADATA)
{
    // Metadata needs at least a name field (since that's what getName() will return).
    if (metadata.get("name").empty()) {
        if ((system->hasPlatformId(PlatformIds::ARCADE) ||
                system->hasPlatformId(PlatformIds::NEOGEO)) &&
                metadata.getType() != FOLDER_METADATA) {
            // If it's a MAME or Neo Geo game, expand the game name accordingly.
            metadata.set("name",
                    MameNames::getInstance()->getCleanName(getCleanName()));
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
    if (mType == GAME)
        mSystem->getIndex()->removeFromIndex(this);

    while (mChildren.size() > 0)
        delete (mChildren.front());

    if (mParent)
        mParent->removeChild(this);
}

std::string FileData::getDisplayName() const
{
    std::string stem = Utils::FileSystem::getStem(mPath);
    return stem;
}

std::string FileData::getCleanName() const
{
    return Utils::String::removeParenthesis(this->getDisplayName());
}

const std::string& FileData::getName()
{
    return metadata.get("name");
}

const std::string& FileData::getSortName()
{
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

    for (auto it = mChildrenByFilename.cbegin();
            it != mChildrenByFilename.cend(); it++) {
        childrenRecursive.push_back((*it).second);
        // Recurse through any subdirectories.
        if ((*it).second->getType() == FOLDER) {
            std::vector<FileData*> childrenSubdirectory = (*it).second->getChildrenRecursive();
            childrenRecursive.insert(childrenRecursive.end(),
                    childrenSubdirectory.begin(), childrenSubdirectory.end());
        }
    }

    return childrenRecursive;
}

bool FileData::viewHasOnlyFolders()
{
    bool onlyFolders = true;
    std::vector<FileData*> entrySiblings = this->getParent()->getChildren();
    for (auto it = entrySiblings.cbegin(); it != entrySiblings.cend(); it++) {
        if ((*it)->getType() != FOLDER)
            onlyFolders = false;
    }
    return onlyFolders;
}

const std::string FileData::getROMDirectory()
{
    std::string romDirSetting = Settings::getInstance()->getString("ROMDirectory");
    std::string romDirPath = "";

    if (romDirSetting == "") {
        romDirPath = Utils::FileSystem::getHomePath() + "/ROMs/";
    }
    else {
        romDirPath = romDirSetting;
        // Expand home path if ~ is used.
        romDirPath = Utils::FileSystem::expandHomePath(romDirPath);

        if (romDirPath.back() !=  '/')
            romDirPath = romDirPath + "/";
    }

    // If %ESPATH% is used for the ROM path configuration, then expand it to the executable
    // directory of ES. This is useful for a portable emulator installation, for instance on
    // a USB memory stick.
    romDirPath = Utils::String::replace(romDirPath, "%ESPATH%", Utils::FileSystem::getExePath());

    return romDirPath;
}

const std::string FileData::getMediaDirectory()
{
    std::string mediaDirSetting = Settings::getInstance()->getString("MediaDirectory");
    std::string mediaDirPath = "";

    if (mediaDirSetting == "") {
        mediaDirPath = Utils::FileSystem::getHomePath() + "/.emulationstation/downloaded_media/";
    }
    else {
        mediaDirPath = mediaDirSetting;
        // Expand home path if ~ is used.
        mediaDirPath = Utils::FileSystem::expandHomePath(mediaDirPath);
        // Expand home symbol if the path starts with ~

        if (mediaDirPath.back() !=  '/')
            mediaDirPath = mediaDirPath + "/";
    }

    return mediaDirPath;
}

const std::string FileData::getMediafilePath(std::string subdirectory, std::string mediatype) const
{
    const char* extList[2] = { ".png", ".jpg" };

    // Look for an image file in the media directory.
    std::string tempPath = getMediaDirectory() + mSystemName + "/" +
            subdirectory + "/" + getDisplayName();
    for (int i = 0; i < 2; i++) {
        std::string mediaPath = tempPath + extList[i];
        if (Utils::FileSystem::exists(mediaPath))
            return mediaPath;
    }

    // No media found in the media directory, so look
    // for local art as well (if configured to do so).
    if (Settings::getInstance()->getBool("LocalArt")) {
        for (int i = 0; i < 2; i++) {
            std::string localMediaPath = mEnvData->mStartPath + "/images/" +
                    getDisplayName() + "-" + mediatype + extList[i];
            if (Utils::FileSystem::exists(localMediaPath))
                return localMediaPath;
        }
    }

    return "";
}

const std::string FileData::getImagePath() const
{
    // Look for a mix image (a combination of screenshot, 2D/3D box and marquee).
    std::string image = getMediafilePath("miximages", "miximage");
    if (image != "")
        return image;

    // If no mix image was found, try screenshot instead.
    image = getMediafilePath("screenshots", "screenshot");
    if (image != "")
        return image;

    // If no screenshot was found either, try cover.
    return getMediafilePath("covers", "cover");
}

const std::string FileData::get3DBoxPath() const
{
    return getMediafilePath("3dboxes", "3dbox");
}

const std::string FileData::getCoverPath() const
{
    return getMediafilePath("covers", "cover");
}

const std::string FileData::getMarqueePath() const
{
    return getMediafilePath("marquees", "marquee");
}

const std::string FileData::getMiximagePath() const
{
    return getMediafilePath("miximages", "miximage");
}

const std::string FileData::getScreenshotPath() const
{
    return getMediafilePath("screenshots", "screenshot");
}

const std::string FileData::getThumbnailPath() const
{
    return getMediafilePath("thumbnails", "thumbnail");
}

const std::string FileData::getVideoPath() const
{
    const char* extList[5] = { ".avi", ".mkv", ".mov", ".mp4", ".wmv" };
    std::string tempPath = getMediaDirectory() + mSystemName + "/videos/" + getDisplayName();

    // Look for media in the media directory.
    for (int i = 0; i < 5; i++) {
        std::string mediaPath = tempPath + extList[i];
        if (Utils::FileSystem::exists(mediaPath))
            return mediaPath;
    }

    // No media found in the media directory, so look
    // for local art as well (if configured to do so).
    if (Settings::getInstance()->getBool("LocalArt"))
    {
        for (int i = 0; i < 5; i++) {
            std::string localMediaPath = mEnvData->mStartPath + "/videos/" +
                    getDisplayName() + "-video" + extList[i];
            if (Utils::FileSystem::exists(localMediaPath))
                return localMediaPath;
        }
    }

    return "";
}

const std::vector<FileData*>& FileData::getChildrenListToDisplay()
{
    FileFilterIndex* idx = mSystem->getIndex();
    if (idx->isFiltered()) {
        mFilteredChildren.clear();
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
            if (idx->showFile((*it))) {
                mFilteredChildren.push_back(*it);
            }
        }
        return mFilteredChildren;
    }
    else {
        return mChildren;
    }
}

std::vector<FileData*> FileData::getFilesRecursive(unsigned int typeMask,
        bool displayedOnly, bool countAllGames) const
{
    std::vector<FileData*> out;
    FileFilterIndex* idx = mSystem->getIndex();

    for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        if ((*it)->getType() & typeMask) {
            if (!displayedOnly || !idx->isFiltered() || idx->showFile(*it)) {
                if (countAllGames)
                    out.push_back(*it);
                else if ((*it)->getCountAsGame())
                    out.push_back(*it);
            }
        }
        if ((*it)->getChildren().size() > 0) {
            std::vector<FileData*> subChildren = (*it)->getFilesRecursive(typeMask, displayedOnly);
            if (countAllGames) {
                out.insert(out.cend(), subChildren.cbegin(), subChildren.cend());
            }
            else {
                for (auto it = subChildren.cbegin(); it != subChildren.cend(); it++) {
                    if ((*it)->getCountAsGame())
                        out.push_back(*it);
                }
            }
        }
    }

    return out;
}

std::vector<FileData*> FileData::getScrapeFilesRecursive(bool includeFolders,
        bool excludeRecursively, bool respectExclusions) const
{
    std::vector<FileData*> out;

    for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        if (includeFolders && (*it)->getType() == FOLDER) {
            if (!(respectExclusions && (*it)->getExcludeFromScraper()))
                out.push_back(*it);
        }
        else if ((*it)->getType() == GAME) {
            if (!(respectExclusions && (*it)->getExcludeFromScraper()))
                out.push_back(*it);
        }

        // If the flag has been passed to exclude directories recursively, then skip the entire
        // folder at this point if the folder is marked for scrape exclusion.
        if (excludeRecursively && (*it)->getType() == FOLDER && (*it)->getExcludeFromScraper())
            continue;

        if ((*it)->getChildren().size() > 0) {
            std::vector<FileData*> subChildren = (*it)->getScrapeFilesRecursive(
                    includeFolders, excludeRecursively, respectExclusions);
            out.insert(out.cend(), subChildren.cbegin(), subChildren.cend());
        }
    }

    return out;
}

std::string FileData::getKey() {
    return getFileName();
}

const bool FileData::isArcadeAsset()
{
    const std::string stem = Utils::FileSystem::getStem(mPath);
    return ((mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) ||
            mSystem->hasPlatformId(PlatformIds::NEOGEO))) &&
            (MameNames::getInstance()->isBios(stem) ||
            MameNames::getInstance()->isDevice(stem)));
}

FileData* FileData::getSourceFileData()
{
    return this;
}

void FileData::addChild(FileData* file)
{
    assert(mType == FOLDER);
    assert(file->getParent() == nullptr);

    const std::string key = file->getKey();
    if (mChildrenByFilename.find(key) == mChildrenByFilename.cend()) {
        mChildrenByFilename[key] = file;
        mChildren.push_back(file);
        file->mParent = this;
    }
}

void FileData::removeChild(FileData* file)
{
    assert(mType == FOLDER);
    assert(file->getParent() == this);
    mChildrenByFilename.erase(file->getKey());
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        if (*it == file) {
            file->mParent = nullptr;
            mChildren.erase(it);
            return;
        }
    }

    // File somehow wasn't in our children.
    assert(false);
}

void FileData::sort(ComparisonFunction& comparator, bool ascending,
        std::pair<unsigned int, unsigned int>& gameCount)
{
    mFirstLetterIndex.clear();
    mOnlyFolders = true;
    bool foldersOnTop = Settings::getInstance()->getBool("FoldersOnTop");
    bool hasFolders = false;
    std::vector<FileData*> mChildrenFolders;
    std::vector<FileData*> mChildrenOthers;

    if (mSystem->isGroupedCustomCollection())
        gameCount = {};

    // The main custom collections view is sorted during startup in CollectionSystemManager.
    // The individual collections are however sorted as any normal systems/folders.
    if (mSystem->isCollection() && mSystem->getFullName() == "collections") {
        std::pair<unsigned int, unsigned int> tempGameCount = {};
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
            // Build mFirstLetterIndex.
            const char firstChar = toupper((*it)->getSortName().front());
            mFirstLetterIndex.push_back(std::string(1, firstChar));
            if ((*it)->getChildren().size() > 0)
                (*it)->sort(comparator, ascending, gameCount);
            tempGameCount.first += gameCount.first;
            tempGameCount.second += gameCount.second;
            gameCount = {};
        }
        gameCount = tempGameCount;
        // Sort and make each entry unique in mFirstLetterIndex.
        std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
        auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
        mFirstLetterIndex.erase(last, mFirstLetterIndex.end());
        return;
    }

    if (foldersOnTop) {
        for (unsigned int i = 0; i < mChildren.size(); i++) {
            if (mChildren[i]->getType() == FOLDER) {
                mChildrenFolders.push_back(mChildren[i]);
                hasFolders = true;
            }
            else {
                mChildrenOthers.push_back(mChildren[i]);
                mOnlyFolders = false;
            }
        }

        // If descending sorting is requested, always perform a ascending sort by filename first.
        // This adds a slight (probably negligible) overhead but it will avoid strange sorting
        // issues where the secondary sorting is reversed for some sort types.
        if (!ascending)
            std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(),
                    getSortTypeFromString("filename, ascending").comparisonFunction);

        if (foldersOnTop && mOnlyFolders)
            std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(), comparator);
        std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(), comparator);

        if (!ascending) {
            if (foldersOnTop && mOnlyFolders)
                std::reverse(mChildrenFolders.begin(), mChildrenFolders.end());
            std::reverse(mChildrenOthers.begin(), mChildrenOthers.end());
        }

        mChildren.erase(mChildren.begin(), mChildren.end());
        mChildren.reserve(mChildrenFolders.size() + mChildrenOthers.size());
        mChildren.insert(mChildren.end(), mChildrenFolders.begin(), mChildrenFolders.end());
        mChildren.insert(mChildren.end(), mChildrenOthers.begin(), mChildrenOthers.end());
    }
    else {
        if (!ascending)
            std::stable_sort(mChildren.begin(), mChildren.end(),
                    getSortTypeFromString("filename, ascending").comparisonFunction);

        std::stable_sort(mChildren.begin(), mChildren.end(), comparator);
        if (!ascending)
            std::reverse(mChildren.begin(), mChildren.end());
    }

    for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        // Game count, which will be displayed in the system view.
        if ((*it)->getType() == GAME && (*it)->getCountAsGame()) {
            if (!(*it)->getFavorite())
                gameCount.first++;
            else
                gameCount.second++;
        }

        if ((*it)->getType() != FOLDER)
            mOnlyFolders = false;

        if (!(foldersOnTop && (*it)->getType() == FOLDER)) {
            // Build mFirstLetterIndex.
            const char firstChar = toupper((*it)->getSortName().front());
            mFirstLetterIndex.push_back(std::string(1, firstChar));
        }
        // Iterate through any child folders.
        if ((*it)->getChildren().size() > 0)
            (*it)->sort(comparator, ascending, gameCount);
    }

    if (mSystem->isGroupedCustomCollection())
        mGameCount = gameCount;

    // If there are only folders in the gamelist, then it makes sense to still
    // generate a letter index.
    if (mOnlyFolders) {
        for (unsigned int i = 0; i < mChildrenFolders.size(); i++) {
            const char firstChar = toupper(mChildrenFolders[i]->getSortName().front());
            mFirstLetterIndex.push_back(std::string(1, firstChar));
        }
        mOnlyFolders = true;
    }

    // Sort and make each entry unique in mFirstLetterIndex.
    std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    mFirstLetterIndex.erase(last, mFirstLetterIndex.end());

    // If it's a mixed list and folders are sorted on top, add a folder icon to the index.
    if (foldersOnTop && hasFolders && !mOnlyFolders)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), FOLDER_CHAR);
}

void FileData::sortFavoritesOnTop(ComparisonFunction& comparator, bool ascending,
        std::pair<unsigned int, unsigned int>& gameCount)
{
    mFirstLetterIndex.clear();
    mOnlyFolders = true;
    std::vector<FileData*> mChildrenFolders;
    std::vector<FileData*> mChildrenFavoritesFolders;
    std::vector<FileData*> mChildrenFavorites;
    std::vector<FileData*> mChildrenOthers;
    bool foldersOnTop = Settings::getInstance()->getBool("FoldersOnTop");
    bool hasFolders = false;

    if (mSystem->isGroupedCustomCollection())
        gameCount = {};

    // The main custom collections view is sorted during startup in CollectionSystemManager.
    // The individual collections are however sorted as any normal systems/folders.
    if (mSystem->isCollection() && mSystem->getFullName() == "collections") {
        std::pair<unsigned int, unsigned int> tempGameCount = {};
        for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
            // Build mFirstLetterIndex.
            const char firstChar = toupper((*it)->getSortName().front());
            mFirstLetterIndex.push_back(std::string(1, firstChar));
            if ((*it)->getChildren().size() > 0)
                (*it)->sortFavoritesOnTop(comparator, ascending, gameCount);
            tempGameCount.first += gameCount.first;
            tempGameCount.second += gameCount.second;
            gameCount = {};
        }
        gameCount = tempGameCount;
        // Sort and make each entry unique in mFirstLetterIndex.
        std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
        auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
        mFirstLetterIndex.erase(last, mFirstLetterIndex.end());
        return;
    }

    for (unsigned int i = 0; i < mChildren.size(); i++) {
        // Game count, which will be displayed in the system view.
        if (mChildren[i]->getType() == GAME && mChildren[i]->getCountAsGame()) {
            if (!mChildren[i]->getFavorite())
                gameCount.first++;
            else
                gameCount.second++;
        }

        if (foldersOnTop && mChildren[i]->getType() == FOLDER) {
            if (!mChildren[i]->getFavorite())
                mChildrenFolders.push_back(mChildren[i]);
            else
                mChildrenFavoritesFolders.push_back(mChildren[i]);

            hasFolders = true;
        }
        else if (mChildren[i]->getFavorite()) {
            mChildrenFavorites.push_back(mChildren[i]);
        }
        else {
            mChildrenOthers.push_back(mChildren[i]);
            // Build mFirstLetterIndex.
            const char firstChar = toupper(mChildren[i]->getSortName().front());
            mFirstLetterIndex.push_back(std::string(1, firstChar));
        }

        if (mChildren[i]->getType() != FOLDER)
            mOnlyFolders = false;
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

    // If there are only favorites in the gamelist, it makes sense to still generate
    // a letter index. For instance to be able to quick jump in the 'favorites'
    // collection. Doing this additional work here only for the applicable gamelists is
    // probably faster than building a redundant index for all gamelists during sorting.
    if (mChildrenOthers.size() == 0 && mChildrenFavorites.size() > 0) {
        for (unsigned int i = 0; i < mChildren.size(); i++) {
            if (foldersOnTop && mChildren[i]->getType() == FOLDER) {
                continue;
            }
            else {
                const char firstChar = toupper(mChildren[i]->getSortName().front());
                mFirstLetterIndex.push_back(std::string(1, firstChar));
            }
        }
    }
    // If there are only folders in the gamelist, then it also makes sense to generate
    // a letter index.
    else if (mOnlyFolders) {
        for (unsigned int i = 0; i < mChildrenFolders.size(); i++) {
            const char firstChar = toupper(mChildrenFolders[i]->getSortName().front());
            mFirstLetterIndex.push_back(std::string(1, firstChar));
        }
    }

    // Sort and make each entry unique in mFirstLetterIndex.
    std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
    mFirstLetterIndex.erase(last, mFirstLetterIndex.end());

    // If there were at least one favorite folder in the gamelist, insert the favorite
    // unicode character in the first position.
    if (foldersOnTop && mOnlyFolders && mChildrenFavoritesFolders.size() > 0)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), FAVORITE_CHAR);
    // If there were at least one favorite in the gamelist, insert the favorite
    // unicode character in the first position.
    else if (mChildrenOthers.size() > 0 && mChildrenFavorites.size() > 0)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), FAVORITE_CHAR);

    // If it's a mixed list and folders are sorted on top, add a folder icon to the index.
    if (foldersOnTop && hasFolders && !mOnlyFolders)
        mFirstLetterIndex.insert(mFirstLetterIndex.begin(), FOLDER_CHAR);

    // If descending sorting is requested, always perform a ascending sort by filename first.
    // This adds a slight (probably negligible) overhead but it will avoid strange sorting
    // issues where the secondary sorting is reversed for some sort types.
    if (!ascending) {
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
        std::stable_sort(mChildrenFavoritesFolders.begin(),
                mChildrenFavoritesFolders.end(), comparator);
        std::stable_sort(mChildrenFolders.begin(), mChildrenFolders.end(), comparator);
    }
    std::stable_sort(mChildrenFavorites.begin(), mChildrenFavorites.end(), comparator);
    std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(), comparator);

    // Iterate through any child favorite folders.
    for (auto it = mChildrenFavoritesFolders.cbegin(); it !=
            mChildrenFavoritesFolders.cend(); it++) {
        if ((*it)->getChildren().size() > 0)
            (*it)->sortFavoritesOnTop(comparator, ascending, gameCount);
    }

    // Iterate through any child folders.
    for (auto it = mChildrenFolders.cbegin(); it != mChildrenFolders.cend(); it++) {
        if ((*it)->getChildren().size() > 0)
            (*it)->sortFavoritesOnTop(comparator, ascending, gameCount);
    }

    if (!ascending) {
        if (foldersOnTop && mOnlyFolders) {
            std::reverse(mChildrenFavoritesFolders.begin(), mChildrenFavoritesFolders.end());
            std::reverse(mChildrenFolders.begin(), mChildrenFolders.end());
        }
        std::reverse(mChildrenFavorites.begin(), mChildrenFavorites.end());
        std::reverse(mChildrenOthers.begin(), mChildrenOthers.end());
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
        sortFavoritesOnTop(*type.comparisonFunction, type.ascending, mGameCount);
    else
        sort(*type.comparisonFunction, type.ascending, mGameCount);
}

FileData::SortType FileData::getSortTypeFromString(std::string desc) {
    std::vector<FileData::SortType> SortTypes = FileSorts::SortTypes;

    for (unsigned int i = 0; i < FileSorts::SortTypes.size(); i++) {
        const FileData::SortType& sort = FileSorts::SortTypes.at(i);
        if (sort.description == desc)
            return sort;
    }
    // If no type was found then default to "filename, ascending".
    return FileSorts::SortTypes.at(0);
}

void FileData::launchGame(Window* window)
{
    LOG(LogInfo) << "Attempting to launch game...";

    std::string command = "";

    // Check if there is a launch command override for the game
    // and the corresponding option to use it has been set.
    if (Settings::getInstance()->getBool("LaunchCommandOverride") &&
            !metadata.get("launchcommand").empty())
        command = metadata.get("launchcommand");
    else
        command = mEnvData->mLaunchCommand;

    std::string commandRaw = command;

    const std::string rom = Utils::FileSystem::getEscapedPath(getPath());
    const std::string basename = Utils::FileSystem::getStem(getPath());
    const std::string rom_raw = Utils::FileSystem::getPreferredPath(getPath());
    const std::string emupath = Utils::FileSystem::getExePath();

    command = Utils::String::replace(command, "%ROM%", rom);
    command = Utils::String::replace(command, "%BASENAME%", basename);
    command = Utils::String::replace(command, "%ROM_RAW%", rom_raw);
    command = Utils::String::replace(command, "%ESPATH%", emupath);

    // Expand home path if ~ is used.
    command = Utils::FileSystem::expandHomePath(command);

    #if defined(_WIN64)
    std::wstring commandWide = Utils::String::stringToWideString(command);
    #endif

    Scripting::fireEvent("game-start", rom, getSourceFileData()->metadata.get("name"));
    int returnValue = 0;

    if (command.find("%EMUPATH%") != std::string::npos) {
        // Extract the emulator executable from the launch command string. This could either be
        // just the program name, assuming the binary is in the PATH variable of the operating
        // system, or it could be an absolute path to the emulator. (In the latter case, if
        // there is a space in the the path, it needs to be enclosed by quotation marks in
        // es_systems.cfg.)
        std::string emuExecutable;

        // If the first character is a quotation mark, then we need to extract up to the
        // next quotation mark, otherwise we'll extract up to the first space character.
        if (command.front() == '\"') {
            std::string emuTemp = command.substr(1, std::string::npos);
            emuExecutable = emuTemp.substr(0, emuTemp.find('"'));
        }
        else {
            emuExecutable = command.substr(0, command.find(' '));
        }

        // For Windows, we need to handle UTF-16 encoding.
        #if defined(_WIN64)
        std::wstring emuExecutableWide;
        std::wstring emuPathWide;

        emuExecutableWide = Utils::String::stringToWideString(emuExecutable);

        // Search for the emulator using the PATH environmental variable.
        DWORD size = SearchPathW(nullptr, emuExecutableWide.c_str(), L".exe", 0, nullptr, nullptr);

        if (size) {
            std::vector<wchar_t> pathBuffer(static_cast<size_t>(size) + 1 );
            wchar_t* fileName = nullptr;

            SearchPathW(nullptr, emuExecutableWide.c_str(), L".exe", size + 1 ,
                    pathBuffer.data(), &fileName);
            std::wstring pathString = pathBuffer.data();

            if (pathString.length()) {
                emuPathWide = pathString.substr(0, pathString.size() -
                        std::wstring(fileName).size());
                emuPathWide.pop_back();
                auto stringPos = commandWide.find(L"%EMUPATH%");
                commandWide = commandWide.replace(stringPos, 9, emuPathWide);
            }
        }
        #else
        std::string exePath;
        if (Utils::FileSystem::isRegularFile(emuExecutable) ||
                Utils::FileSystem::isSymlink(emuExecutable))
            exePath = Utils::FileSystem::getParent(emuExecutable);
        else
            exePath = Utils::FileSystem::getPathToBinary(emuExecutable);

        command = Utils::String::replace(command, "%EMUPATH%", exePath);
        #endif
    }

    LOG(LogDebug) << "Raw emulator launch command:";
    LOG(LogDebug) << commandRaw;
    LOG(LogInfo) << "Expanded emulator launch command:";

    #if defined(_WIN64)
    LOG(LogInfo) << Utils::String::wideStringToString(commandWide);
    returnValue = launchEmulatorWindows(commandWide);
    #else
    LOG(LogInfo) << command;
    returnValue = launchEmulatorUnix(command);
    #endif

    // Notify the user in case of a failed game launch using a popup window.
    if (returnValue != 0) {
        LOG(LogWarning) << "...launch terminated with nonzero return value " << returnValue << "!";

        GuiInfoPopup* s = new GuiInfoPopup(window, "ERROR LAUNCHING GAME '" +
                Utils::String::toUpper(metadata.get("name")) + "' (ERROR CODE " +
                Utils::String::toUpper(std::to_string(returnValue) + ")"), 6000);
        window->setInfoPopup(s);
    }
    else {
        // Stop showing the game launch notification.
        window->stopInfoPopup();
        #if  defined(_WIN64)
        // This code is only needed for Windows, where we may need to keep ES running while
        // the game/emulator is in use. It's basically used to pause any playing game video
        // and to keep the screensaver from activating.
        if (Settings::getInstance()->getBool("RunInBackground"))
            window->setLaunchedGame();
        else
            // Normalize deltaTime so that the screensaver does not start immediately
            // when returning from the game.
            window->normalizeNextUpdate();
        #else
        // Normalize deltaTime so that the screensaver does not start immediately
        // when returning from the game.
        window->normalizeNextUpdate();
        #endif
    }

    Scripting::fireEvent("game-end", rom, getSourceFileData()->metadata.get("name"));

    // Re-enable the text scrolling that was disabled in ViewController on game launch.
    window->setAllowTextScrolling(true);

    // Update number of times the game has been launched.
    FileData* gameToUpdate = getSourceFileData();

    int timesPlayed = gameToUpdate->metadata.getInt("playcount") + 1;
    gameToUpdate->metadata.set("playcount", std::to_string(static_cast<long long>(timesPlayed)));

    // Update last played time.
    gameToUpdate->metadata.set("lastplayed", Utils::Time::DateTime(Utils::Time::now()));

    // If the parent is a folder and it's not the root of the system, then update its lastplayed
    // timestamp to the same time as the game that was just launched.
    if (gameToUpdate->getParent()->getType() == FOLDER && gameToUpdate->getParent()->getName() !=
            gameToUpdate->getSystem()->getFullName()) {
        gameToUpdate->getParent()->metadata.set("lastplayed",
                gameToUpdate->metadata.get("lastplayed"));
    }

    CollectionSystemManager::get()->refreshCollectionSystems(gameToUpdate);

    gameToUpdate->mSystem->onMetaDataSavePoint();
}

CollectionFileData::CollectionFileData(FileData* file, SystemData* system)
    : FileData(file->getSourceFileData()->getType(), file->getSourceFileData()->getPath(),
            file->getSourceFileData()->getSystemEnvData(), system)
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

std::string CollectionFileData::getKey() {
    return getFullPath();
}

FileData* CollectionFileData::getSourceFileData()
{
    return mSourceFileData;
}

void CollectionFileData::refreshMetadata()
{
    metadata = mSourceFileData->metadata;
    mDirty = true;
}

const std::string& CollectionFileData::getName()
{
    if (mDirty) {
        mCollectionFileName =
                Utils::String::removeParenthesis(mSourceFileData->metadata.get("name"));
        mCollectionFileName +=
                " [" + Utils::String::toUpper(mSourceFileData->getSystem()->getName()) + "]";
        mDirty = false;
    }

    if (Settings::getInstance()->getBool("CollectionShowSystemInfo"))
        return mCollectionFileName;

    return mSourceFileData->metadata.get("name");
}

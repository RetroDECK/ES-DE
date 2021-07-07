//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileFilterIndex.cpp
//
//  Gamelist filters.
//

#include "FileFilterIndex.h"

#include "FileData.h"
#include "Log.h"
#include "Settings.h"
#include "math/Misc.h"
#include "utils/StringUtil.h"
#include "views/UIModeController.h"

#include <cmath>

#define UNKNOWN_LABEL "UNKNOWN"
#define INCLUDE_UNKNOWN false;

FileFilterIndex::FileFilterIndex()
    : mFilterByText(false)
    , mFilterByFavorites(false)
    , mFilterByGenre(false)
    , mFilterByPlayers(false)
    , mFilterByPubDev(false)
    , mFilterByRatings(false)
    , mFilterByKidGame(false)
    , mFilterByCompleted(false)
    , mFilterByBroken(false)
    , mFilterByHidden(false)
{
    clearAllFilters();

    // clang-format off
    FilterDataDecl filterDecls[] = {
        //type              //allKeys                //filteredBy         //filteredKeys                //primaryKey    //hasSecondaryKey   //secondaryKey  //menuLabel
        { FAVORITES_FILTER, &mFavoritesIndexAllKeys, &mFilterByFavorites, &mFavoritesIndexFilteredKeys, "favorite",     false,              "",             "FAVORITES" },
        { GENRE_FILTER,     &mGenreIndexAllKeys,     &mFilterByGenre,     &mGenreIndexFilteredKeys,     "genre",        true,               "genre",        "GENRE" },
        { PLAYER_FILTER,    &mPlayersIndexAllKeys,   &mFilterByPlayers,   &mPlayersIndexFilteredKeys,   "players",      false,              "",             "PLAYERS" },
        { PUBDEV_FILTER,    &mPubDevIndexAllKeys,    &mFilterByPubDev,    &mPubDevIndexFilteredKeys,    "developer",    true,               "publisher",    "PUBLISHER / DEVELOPER" },
        { RATINGS_FILTER,   &mRatingsIndexAllKeys,   &mFilterByRatings,   &mRatingsIndexFilteredKeys,   "rating",       false,              "",             "RATING" },
        { KIDGAME_FILTER,   &mKidGameIndexAllKeys,   &mFilterByKidGame,   &mKidGameIndexFilteredKeys,   "kidgame",      false,              "",             "KIDGAME" },
        { COMPLETED_FILTER, &mCompletedIndexAllKeys, &mFilterByCompleted, &mCompletedIndexFilteredKeys, "completed",    false,              "",             "COMPLETED" },
        { BROKEN_FILTER,    &mBrokenIndexAllKeys,    &mFilterByBroken,    &mBrokenIndexFilteredKeys,    "broken",       false,              "",             "BROKEN" },
        { HIDDEN_FILTER,    &mHiddenIndexAllKeys,    &mFilterByHidden,    &mHiddenIndexFilteredKeys,    "hidden",       false,              "",             "HIDDEN" }
    };
    // clang-format on

    filterDataDecl = std::vector<FilterDataDecl>(
        filterDecls, filterDecls + sizeof(filterDecls) / sizeof(filterDecls[0]));
}

FileFilterIndex::~FileFilterIndex()
{
    // Reset the index when destroyed.
    resetIndex();
}

void FileFilterIndex::importIndex(FileFilterIndex* indexToImport)
{
    struct IndexImportStructure {
        std::map<std::string, int>* destinationIndex;
        std::map<std::string, int>* sourceIndex;
    };

    IndexImportStructure indexStructDecls[] = {
        { &mFavoritesIndexAllKeys, &(indexToImport->mFavoritesIndexAllKeys) },
        { &mGenreIndexAllKeys, &(indexToImport->mGenreIndexAllKeys) },
        { &mPlayersIndexAllKeys, &(indexToImport->mPlayersIndexAllKeys) },
        { &mPubDevIndexAllKeys, &(indexToImport->mPubDevIndexAllKeys) },
        { &mRatingsIndexAllKeys, &(indexToImport->mRatingsIndexAllKeys) },
        { &mKidGameIndexAllKeys, &(indexToImport->mKidGameIndexAllKeys) },
        { &mCompletedIndexAllKeys, &(indexToImport->mCompletedIndexAllKeys) },
        { &mBrokenIndexAllKeys, &(indexToImport->mBrokenIndexAllKeys) },
        { &mHiddenIndexAllKeys, &(indexToImport->mHiddenIndexAllKeys) },
    };

    std::vector<IndexImportStructure> indexImportDecl = std::vector<IndexImportStructure>(
        indexStructDecls,
        indexStructDecls + sizeof(indexStructDecls) / sizeof(indexStructDecls[0]));

    for (std::vector<IndexImportStructure>::const_iterator indexesIt = indexImportDecl.cbegin();
         indexesIt != indexImportDecl.cend(); indexesIt++) {
        for (std::map<std::string, int>::const_iterator sourceIt =
                 (*indexesIt).sourceIndex->cbegin();
             sourceIt != (*indexesIt).sourceIndex->cend(); sourceIt++) {
            if ((*indexesIt).destinationIndex->find((*sourceIt).first) ==
                (*indexesIt).destinationIndex->cend()) {
                // Entry doesn't exist.
                (*((*indexesIt).destinationIndex))[(*sourceIt).first] = (*sourceIt).second;
            }
            else {
                (*((*indexesIt).destinationIndex))[(*sourceIt).first] += (*sourceIt).second;
            }
        }
    }
}

void FileFilterIndex::resetIndex()
{
    clearAllFilters();
    clearIndex(mFavoritesIndexAllKeys);
    clearIndex(mGenreIndexAllKeys);
    clearIndex(mPlayersIndexAllKeys);
    clearIndex(mPubDevIndexAllKeys);
    clearIndex(mRatingsIndexAllKeys);
    clearIndex(mKidGameIndexAllKeys);
    clearIndex(mCompletedIndexAllKeys);
    clearIndex(mBrokenIndexAllKeys);
    clearIndex(mHiddenIndexAllKeys);
}

std::string FileFilterIndex::getIndexableKey(FileData* game,
                                             FilterIndexType type,
                                             bool getSecondary)
{
    std::string key = "";
    switch (type) {
        case FAVORITES_FILTER: {
            if (game->getType() != GAME)
                return "FALSE";
            key = Utils::String::toUpper(game->metadata.get("favorite"));
            break;
        }
        case GENRE_FILTER: {
            key = Utils::String::toUpper(game->metadata.get("genre"));
            key = Utils::String::trim(key);
            if (getSecondary && !key.empty()) {
                std::istringstream f(key);
                std::string newKey;
                getline(f, newKey, '/');
                if (!newKey.empty() && newKey != key)
                    key = newKey;
                else
                    key = std::string();
            }
            break;
        }
        case PLAYER_FILTER: {
            if (getSecondary)
                break;

            key = Utils::String::toUpper(game->metadata.get("players"));
            break;
        }
        case PUBDEV_FILTER: {
            key = Utils::String::toUpper(game->metadata.get("publisher"));
            key = Utils::String::trim(key);

            if ((getSecondary && !key.empty()) || (!getSecondary && key.empty()))
                key = Utils::String::toUpper(game->metadata.get("developer"));
            else
                key = Utils::String::toUpper(game->metadata.get("publisher"));
            break;
        }
        case RATINGS_FILTER: {
            int ratingNumber = 0;
            if (!getSecondary) {
                std::string ratingString = game->metadata.get("rating");
                if (!ratingString.empty()) {
                    try {
                        // Round up fractional values such as 0.75 to 0.8.
                        // These values should only exist if a third party application has
                        // been used for scraping the ratings, or if the gamelist.xml file
                        // has been manually edited.
                        ratingNumber =
                            static_cast<int>((ceilf(stof(ratingString) / 0.1f) / 10.0f) * 5.0f);

                        if (ratingNumber < 0)
                            ratingNumber = 0;

                        if (ratingNumber == 5)
                            key = "5 STARS";
                        else
                            key = std::to_string(ratingNumber) + " - " +
                                  std::to_string(ratingNumber) + ".5 STARS";
                    }
                    catch (int e) {
                        LOG(LogError) << "Error parsing Rating (invalid value, exception nr.): "
                                      << ratingString << ", " << e;
                    }
                }
            }
            break;
        }
        case KIDGAME_FILTER: {
            if (game->getType() != GAME)
                return "FALSE";
            key = Utils::String::toUpper(game->metadata.get("kidgame"));
            break;
        }
        case COMPLETED_FILTER: {
            if (game->getType() != GAME)
                return "FALSE";
            key = Utils::String::toUpper(game->metadata.get("completed"));
            break;
        }
        case BROKEN_FILTER: {
            if (game->getType() != GAME)
                return "FALSE";
            key = Utils::String::toUpper(game->metadata.get("broken"));
            break;
        }
        case HIDDEN_FILTER: {
            if (game->getType() != GAME)
                return "FALSE";
            key = Utils::String::toUpper(game->metadata.get("hidden"));
            break;
        }
        default:
            break;
    }
    key = Utils::String::trim(key);
    if (key.empty() || (type == RATINGS_FILTER && key == "0 STARS")) {
        key = UNKNOWN_LABEL;
    }
    return key;
}

void FileFilterIndex::addToIndex(FileData* game)
{
    manageFavoritesEntryInIndex(game);
    manageGenreEntryInIndex(game);
    managePlayerEntryInIndex(game);
    managePubDevEntryInIndex(game);
    manageRatingsEntryInIndex(game);
    manageKidGameEntryInIndex(game);
    manageCompletedEntryInIndex(game);
    manageBrokenEntryInIndex(game);
    manageHiddenEntryInIndex(game);
}

void FileFilterIndex::removeFromIndex(FileData* game)
{
    manageFavoritesEntryInIndex(game, true);
    manageGenreEntryInIndex(game, true);
    managePlayerEntryInIndex(game, true);
    managePubDevEntryInIndex(game, true);
    manageRatingsEntryInIndex(game, true);
    manageKidGameEntryInIndex(game, true);
    manageCompletedEntryInIndex(game, true);
    manageBrokenEntryInIndex(game, true);
    manageHiddenEntryInIndex(game, true);
}

void FileFilterIndex::setFilter(FilterIndexType type, std::vector<std::string>* values)
{
    // Test if it exists before setting.
    if (type == NONE) {
        clearAllFilters();
    }
    else {
        for (std::vector<FilterDataDecl>::const_iterator it = filterDataDecl.cbegin();
             it != filterDataDecl.cend(); it++) {
            if ((*it).type == type) {
                FilterDataDecl filterData = (*it);
                *(filterData.filteredByRef) = values->size() > 0;
                filterData.currentFilteredKeys->clear();
                for (std::vector<std::string>::const_iterator vit = values->cbegin();
                     vit != values->cend(); vit++) {
                    // Check if it exists.
                    if (filterData.allIndexKeys->find(*vit) != filterData.allIndexKeys->cend()) {
                        filterData.currentFilteredKeys->push_back(std::string(*vit));
                    }
                }
            }
        }
    }
    return;
}

void FileFilterIndex::setTextFilter(std::string textFilter)
{
    mTextFilter = textFilter;

    if (textFilter == "")
        mFilterByText = false;
    else
        mFilterByText = true;
};

void FileFilterIndex::clearAllFilters()
{
    for (std::vector<FilterDataDecl>::const_iterator it = filterDataDecl.cbegin();
         it != filterDataDecl.cend(); it++) {
        FilterDataDecl filterData = (*it);
        *(filterData.filteredByRef) = false;
        filterData.currentFilteredKeys->clear();
    }
    setTextFilter("");
    return;
}

void FileFilterIndex::resetFilters()
{
    clearAllFilters();
    setKidModeFilters();
}

void FileFilterIndex::setKidModeFilters()
{
    if (UIModeController::getInstance()->isUIModeKid()) {
        mFilterByKidGame = true;
        std::vector<std::string> val = { "TRUE" };
        setFilter(KIDGAME_FILTER, &val);
    }
}

void FileFilterIndex::debugPrintIndexes()
{
    LOG(LogInfo) << "Printing Indexes...";
    for (auto x : mFavoritesIndexAllKeys) {
        LOG(LogInfo) << "Favorites Index: " << x.first << ": " << x.second;
    }
    for (auto x : mGenreIndexAllKeys) {
        LOG(LogInfo) << "Genre Index: " << x.first << ": " << x.second;
    }
    for (auto x : mPlayersIndexAllKeys) {
        LOG(LogInfo) << "Multiplayer Index: " << x.first << ": " << x.second;
    }
    for (auto x : mPubDevIndexAllKeys) {
        LOG(LogInfo) << "PubDev Index: " << x.first << ": " << x.second;
    }
    for (auto x : mRatingsIndexAllKeys) {
        LOG(LogInfo) << "Ratings Index: " << x.first << ": " << x.second;
    }
    for (auto x : mKidGameIndexAllKeys) {
        LOG(LogInfo) << "KidGames Index: " << x.first << ": " << x.second;
    }
    for (auto x : mCompletedIndexAllKeys) {
        LOG(LogInfo) << "Completed Index: " << x.first << ": " << x.second;
    }
    for (auto x : mBrokenIndexAllKeys) {
        LOG(LogInfo) << "Broken Index: " << x.first << ": " << x.second;
    }
    for (auto x : mHiddenIndexAllKeys) {
        LOG(LogInfo) << "Hidden Index: " << x.first << ": " << x.second;
    }
}

bool FileFilterIndex::showFile(FileData* game)
{
    // If folder, needs further inspection - i.e. see if folder contains at least one element
    // that should be shown.
    if (game->getType() == FOLDER) {
        std::vector<FileData*> children = game->getChildren();
        // Iterate through all of the children, until there's a match.
        for (std::vector<FileData*>::const_iterator it = children.cbegin(); it != children.cend();
             it++) {
            if (showFile(*it))
                return true;
        }
        return false;
    }

    bool nameMatch = false;
    bool keepGoing = false;

    // Name filters take precedence over all other filters, so if there is no match for
    // the game name, then always return false.
    if (mTextFilter != "" &&
        !(Utils::String::toUpper(game->getName()).find(mTextFilter) != std::string::npos)) {
        return false;
    }
    else if (mTextFilter != "") {
        nameMatch = true;
    }

    for (std::vector<FilterDataDecl>::const_iterator it = filterDataDecl.cbegin();
         it != filterDataDecl.cend(); it++) {
        FilterDataDecl filterData = (*it);
        if (filterData.primaryKey == "kidgame" && UIModeController::getInstance()->isUIModeKid()) {
            return (getIndexableKey(game, filterData.type, false) != "FALSE");
        }
        else if (*(filterData.filteredByRef)) {
            // Try to find a match.
            std::string key = getIndexableKey(game, filterData.type, false);
            keepGoing = isKeyBeingFilteredBy(key, filterData.type);

            // If we didn't find a match, try for secondary keys - i.e.
            // publisher and dev, or first genre.
            if (!keepGoing) {
                if (!filterData.hasSecondaryKey)
                    return false;
                std::string secKey = getIndexableKey(game, filterData.type, true);
                if (secKey != UNKNOWN_LABEL)
                    keepGoing = isKeyBeingFilteredBy(secKey, filterData.type);
            }
            // If still nothing, then it's not a match.
            if (!keepGoing)
                return false;
        }
    }

    // If there is a match for the game name, but not for any other filters, then return
    // true as it means that the name filter is the only applied filter.
    if (!keepGoing && nameMatch)
        return true;
    else
        return keepGoing;
}

bool FileFilterIndex::isFiltered()
{
    if (UIModeController::getInstance()->isUIModeKid()) {
        return (mFilterByText || mFilterByFavorites || mFilterByGenre || mFilterByPlayers ||
                mFilterByPubDev || mFilterByRatings || mFilterByCompleted || mFilterByBroken ||
                mFilterByHidden);
    }
    else {
        return (mFilterByText || mFilterByFavorites || mFilterByGenre || mFilterByPlayers ||
                mFilterByPubDev || mFilterByRatings || mFilterByKidGame || mFilterByCompleted ||
                mFilterByBroken || mFilterByHidden);
    }
}

bool FileFilterIndex::isKeyBeingFilteredBy(std::string key, FilterIndexType type)
{
    const FilterIndexType filterTypes[9] = { FAVORITES_FILTER, GENRE_FILTER,   PLAYER_FILTER,
                                             PUBDEV_FILTER,    RATINGS_FILTER, KIDGAME_FILTER,
                                             COMPLETED_FILTER, BROKEN_FILTER,  HIDDEN_FILTER };
    std::vector<std::string> filterKeysList[9] = {
        mFavoritesIndexFilteredKeys, mGenreIndexFilteredKeys,   mPlayersIndexFilteredKeys,
        mPubDevIndexFilteredKeys,    mRatingsIndexFilteredKeys, mKidGameIndexFilteredKeys,
        mCompletedIndexFilteredKeys, mBrokenIndexFilteredKeys,  mHiddenIndexFilteredKeys
    };

    for (int i = 0; i < 9; i++) {
        if (filterTypes[i] == type) {
            for (std::vector<std::string>::const_iterator it = filterKeysList[i].cbegin();
                 it != filterKeysList[i].cend(); it++) {
                if (key == (*it))
                    return true;
            }
            return false;
        }
    }
    return false;
}

void FileFilterIndex::manageFavoritesEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, FAVORITES_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid favorites info found.
        return;

    manageIndexEntry(&mFavoritesIndexAllKeys, key, remove);
}

void FileFilterIndex::manageGenreEntryInIndex(FileData* game, bool remove)
{
    std::string key = getIndexableKey(game, GENRE_FILTER, false);

    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;

    // Only add unknown in pubdev IF both dev and pub are empty.
    if (!includeUnknown && (key == UNKNOWN_LABEL || key == "BIOS"))
        // No valid genre info found.
        return;

    manageIndexEntry(&mGenreIndexAllKeys, key, remove);

    key = getIndexableKey(game, GENRE_FILTER, true);
    if (!includeUnknown && key == UNKNOWN_LABEL)
        manageIndexEntry(&mGenreIndexAllKeys, key, remove);
}

void FileFilterIndex::managePlayerEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, PLAYER_FILTER, false);

    // Only add unknown in pubdev IF both dev and pub are empty.
    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid player info found.
        return;

    manageIndexEntry(&mPlayersIndexAllKeys, key, remove);
}

void FileFilterIndex::managePubDevEntryInIndex(FileData* game, bool remove)
{
    std::string pub = getIndexableKey(game, PUBDEV_FILTER, false);
    std::string dev = getIndexableKey(game, PUBDEV_FILTER, true);

    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    bool unknownPub = false;
    bool unknownDev = false;

    if (pub == UNKNOWN_LABEL)
        unknownPub = true;

    if (dev == UNKNOWN_LABEL)
        unknownDev = true;

    if (!includeUnknown && unknownDev && unknownPub)
        // No valid rating info found.
        return;

    if (unknownDev && unknownPub) {
        // If no info at all.
        manageIndexEntry(&mPubDevIndexAllKeys, pub, remove);
    }
    else {
        if (!unknownDev) {
            // If no info at all.
            manageIndexEntry(&mPubDevIndexAllKeys, dev, remove);
        }
        if (!unknownPub) {
            // If no info at all.
            manageIndexEntry(&mPubDevIndexAllKeys, pub, remove);
        }
    }
}

void FileFilterIndex::manageRatingsEntryInIndex(FileData* game, bool remove)
{
    std::string key = getIndexableKey(game, RATINGS_FILTER, false);

    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid rating info found.
        return;

    manageIndexEntry(&mRatingsIndexAllKeys, key, remove);
}

void FileFilterIndex::manageKidGameEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, KIDGAME_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid kidgame info found.
        return;

    manageIndexEntry(&mKidGameIndexAllKeys, key, remove);
}

void FileFilterIndex::manageCompletedEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, COMPLETED_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid completed info found.
        return;

    manageIndexEntry(&mCompletedIndexAllKeys, key, remove);
}

void FileFilterIndex::manageBrokenEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, BROKEN_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid broken info found.
        return;

    manageIndexEntry(&mBrokenIndexAllKeys, key, remove);
}

void FileFilterIndex::manageHiddenEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, HIDDEN_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid hidden info found.
        return;

    manageIndexEntry(&mHiddenIndexAllKeys, key, remove);
}

void FileFilterIndex::manageIndexEntry(std::map<std::string, int>* index,
                                       std::string key,
                                       bool remove)
{
    bool includeUnknown = INCLUDE_UNKNOWN;
    if (!includeUnknown && key == UNKNOWN_LABEL)
        return;

    if (remove) {
        // Removing entry.
        if (index->find(key) == index->cend()) {
            // Disabled for now as this could happen because default values are assigned as
            // filters, for example 'FALSE' for favorites and kidgames for non-game entries.
            //            LOG(LogDebug) << "Couldn't find entry in index! " << key;
        }
        else {
            (index->at(key))--;
            if (index->at(key) <= 0) {
                index->erase(key);
            }
        }
    }
    else {
        // Adding entry.
        if (index->find(key) == index->cend())
            (*index)[key] = 1;
        else
            (index->at(key))++;
    }
}

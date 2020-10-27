//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileFilterIndex.cpp
//
//  Gamelist filters.
//

#include "FileFilterIndex.h"

#include "math/Misc.h"
#include "utils/StringUtil.h"
#include "views/UIModeController.h"
#include "FileData.h"
#include "Log.h"
#include "Settings.h"

#define UNKNOWN_LABEL "UNKNOWN"
#define INCLUDE_UNKNOWN false;

FileFilterIndex::FileFilterIndex()
        : filterByFavorites(false),
        filterByGenre(false),
        filterByPlayers(false),
        filterByPubDev(false),
        filterByRatings(false),
        filterByKidGame(false),
        filterByCompleted(false),
        filterByBroken(false),
        filterByHidden(false)
{
    clearAllFilters();

    FilterDataDecl filterDecls[] = {
        //type              //allKeys               //filteredBy        //filteredKeys               //primaryKey    //hasSecondaryKey   //secondaryKey  //menuLabel
        { FAVORITES_FILTER, &favoritesIndexAllKeys, &filterByFavorites, &favoritesIndexFilteredKeys, "favorite",     false,              "",             "FAVORITES" },
        { GENRE_FILTER,     &genreIndexAllKeys,     &filterByGenre,     &genreIndexFilteredKeys,     "genre",        true,               "genre",        "GENRE" },
        { PLAYER_FILTER,    &playersIndexAllKeys,   &filterByPlayers,   &playersIndexFilteredKeys,   "players",      false,              "",             "PLAYERS" },
        { PUBDEV_FILTER,    &pubDevIndexAllKeys,    &filterByPubDev,    &pubDevIndexFilteredKeys,    "developer",    true,               "publisher",    "PUBLISHER / DEVELOPER" },
        { RATINGS_FILTER,   &ratingsIndexAllKeys,   &filterByRatings,   &ratingsIndexFilteredKeys,   "rating",       false,              "",             "RATING" },
        { KIDGAME_FILTER,   &kidGameIndexAllKeys,   &filterByKidGame,   &kidGameIndexFilteredKeys,   "kidgame",      false,              "",             "KIDGAME" },
        { COMPLETED_FILTER, &completedIndexAllKeys, &filterByCompleted, &completedIndexFilteredKeys, "completed",    false,              "",             "COMPLETED" },
        { BROKEN_FILTER,    &brokenIndexAllKeys,    &filterByBroken,    &brokenIndexFilteredKeys,    "broken",       false,              "",             "BROKEN" },
        { HIDDEN_FILTER,    &hiddenIndexAllKeys,    &filterByHidden,    &hiddenIndexFilteredKeys,    "hidden",       false,              "",             "HIDDEN" }
    };

    filterDataDecl = std::vector<FilterDataDecl>(filterDecls, filterDecls +
            sizeof(filterDecls) / sizeof(filterDecls[0]));
}

FileFilterIndex::~FileFilterIndex()
{
    resetIndex();
}

std::vector<FilterDataDecl>& FileFilterIndex::getFilterDataDecls()
{
    return filterDataDecl;
}

void FileFilterIndex::importIndex(FileFilterIndex* indexToImport)
{
    struct IndexImportStructure {
        std::map<std::string, int>* destinationIndex;
        std::map<std::string, int>* sourceIndex;
    };

    IndexImportStructure indexStructDecls[] = {
        { &favoritesIndexAllKeys, &(indexToImport->favoritesIndexAllKeys) },
        { &genreIndexAllKeys, &(indexToImport->genreIndexAllKeys) },
        { &playersIndexAllKeys, &(indexToImport->playersIndexAllKeys) },
        { &pubDevIndexAllKeys, &(indexToImport->pubDevIndexAllKeys) },
        { &ratingsIndexAllKeys, &(indexToImport->ratingsIndexAllKeys) },
        { &kidGameIndexAllKeys, &(indexToImport->kidGameIndexAllKeys) },
        { &completedIndexAllKeys, &(indexToImport->completedIndexAllKeys) },
        { &brokenIndexAllKeys, &(indexToImport->brokenIndexAllKeys) },
        { &hiddenIndexAllKeys, &(indexToImport->hiddenIndexAllKeys) },
    };

    std::vector<IndexImportStructure> indexImportDecl =
            std::vector<IndexImportStructure>(indexStructDecls, indexStructDecls +
            sizeof(indexStructDecls) / sizeof(indexStructDecls[0]));

    for (std::vector<IndexImportStructure>::const_iterator indexesIt =
            indexImportDecl.cbegin(); indexesIt != indexImportDecl.cend(); ++indexesIt )
    {
        for (std::map<std::string, int>::const_iterator sourceIt =
                (*indexesIt).sourceIndex->cbegin(); sourceIt !=
                (*indexesIt).sourceIndex->cend(); ++sourceIt ) {
            if ((*indexesIt).destinationIndex->find((*sourceIt).first) ==
                    (*indexesIt).destinationIndex->cend())
                // Entry doesn't exist.
                (*((*indexesIt).destinationIndex))[(*sourceIt).first] = (*sourceIt).second;
            else
                (*((*indexesIt).destinationIndex))[(*sourceIt).first] += (*sourceIt).second;
        }
    }
}

void FileFilterIndex::resetIndex()
{
    clearAllFilters();
    clearIndex(favoritesIndexAllKeys);
    clearIndex(genreIndexAllKeys);
    clearIndex(playersIndexAllKeys);
    clearIndex(pubDevIndexAllKeys);
    clearIndex(ratingsIndexAllKeys);
    clearIndex(kidGameIndexAllKeys);
    clearIndex(completedIndexAllKeys);
    clearIndex(brokenIndexAllKeys);
    clearIndex(hiddenIndexAllKeys);
}

std::string FileFilterIndex::getIndexableKey(FileData* game,
        FilterIndexType type, bool getSecondary)
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
                        ratingNumber = static_cast<int>(
                                (Math::ceilf(stof(ratingString) / 0.1) / 10) * 5);

                        if (ratingNumber < 0)
                            ratingNumber = 0;

                        if (ratingNumber == 5)
                            key = "5 STARS";
                        else
                            key = std::to_string(ratingNumber) + " - " +
                                    std::to_string(ratingNumber) + ".5 STARS";
                    }
                    catch (int e) {
                        LOG(LogError) << "Error parsing Rating (invalid value, exception nr.): " <<
                                ratingString << ", " << e;
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
                it != filterDataDecl.cend(); ++it ) {
            if ((*it).type == type) {
                FilterDataDecl filterData = (*it);
                *(filterData.filteredByRef) = values->size() > 0;
                filterData.currentFilteredKeys->clear();
                for (std::vector<std::string>::const_iterator vit =
                        values->cbegin(); vit != values->cend(); ++vit ) {
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

void FileFilterIndex::clearAllFilters()
{
    for (std::vector<FilterDataDecl>::const_iterator it = filterDataDecl.cbegin();
            it != filterDataDecl.cend(); ++it ) {
        FilterDataDecl filterData = (*it);
        *(filterData.filteredByRef) = false;
        filterData.currentFilteredKeys->clear();
    }
    return;
}

void FileFilterIndex::resetFilters()
{
    clearAllFilters();
    setUIModeFilters();
}

void FileFilterIndex::setUIModeFilters()
{
    if (!Settings::getInstance()->getBool("ForceDisableFilters")){
        if (UIModeController::getInstance()->isUIModeKiosk()) {
            filterByHidden = true;
            std::vector<std::string> val = { "FALSE" };
            setFilter(HIDDEN_FILTER, &val);
        }
        if (UIModeController::getInstance()->isUIModeKid()) {
            filterByKidGame = true;
            std::vector<std::string> val = { "TRUE" };
            setFilter(KIDGAME_FILTER, &val);
        }
    }
}

void FileFilterIndex::debugPrintIndexes()
{
    LOG(LogInfo) << "Printing Indexes...";
    for (auto x: favoritesIndexAllKeys) {
        LOG(LogInfo) << "Favorites Index: " << x.first << ": " << x.second;
    }
    for (auto x: genreIndexAllKeys) {
        LOG(LogInfo) << "Genre Index: " << x.first << ": " << x.second;
    }
    for (auto x: playersIndexAllKeys) {
        LOG(LogInfo) << "Multiplayer Index: " << x.first << ": " << x.second;
    }
    for (auto x: pubDevIndexAllKeys) {
        LOG(LogInfo) << "PubDev Index: " << x.first << ": " << x.second;
    }
    for (auto x: ratingsIndexAllKeys) {
        LOG(LogInfo) << "Ratings Index: " << x.first << ": " << x.second;
    }
    for (auto x : kidGameIndexAllKeys) {
        LOG(LogInfo) << "KidGames Index: " << x.first << ": " << x.second;
    }
    for (auto x : completedIndexAllKeys) {
        LOG(LogInfo) << "Completed Index: " << x.first << ": " << x.second;
    }
    for (auto x : brokenIndexAllKeys) {
        LOG(LogInfo) << "Broken Index: " << x.first << ": " << x.second;
    }
    for (auto x : hiddenIndexAllKeys) {
        LOG(LogInfo) << "Hidden Index: " << x.first << ": " << x.second;
    }
}

bool FileFilterIndex::showFile(FileData* game)
{
    // This shouldn't happen, but just in case let's get it out of the way.
    if (!isFiltered())
        return true;

    // If folder, needs further inspection - i.e. see if folder contains at least one element
    // that should be shown.
    if (game->getType() == FOLDER) {
        std::vector<FileData*> children = game->getChildren();
        // Iterate through all of the children, until there's a match.
        for (std::vector<FileData*>::const_iterator it = children.cbegin();
                it != children.cend(); ++it ) {
            if (showFile(*it))
                return true;
        }
        return false;
    }

    bool keepGoing = false;

    for (std::vector<FilterDataDecl>::const_iterator it = filterDataDecl.cbegin();
            it != filterDataDecl.cend(); ++it ) {
        FilterDataDecl filterData = (*it);
        if (*(filterData.filteredByRef)) {
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
    return keepGoing;
}

bool FileFilterIndex::isKeyBeingFilteredBy(std::string key, FilterIndexType type)
{
    const FilterIndexType filterTypes[9] = { FAVORITES_FILTER, GENRE_FILTER,
            PLAYER_FILTER, PUBDEV_FILTER, RATINGS_FILTER, KIDGAME_FILTER,
            COMPLETED_FILTER, BROKEN_FILTER, HIDDEN_FILTER };
    std::vector<std::string> filterKeysList[9] = { favoritesIndexFilteredKeys,
            genreIndexFilteredKeys, playersIndexFilteredKeys, pubDevIndexFilteredKeys,
            ratingsIndexFilteredKeys, kidGameIndexFilteredKeys, completedIndexFilteredKeys,
            brokenIndexFilteredKeys, hiddenIndexFilteredKeys  };

    for (int i = 0; i < 9; i++) {
        if (filterTypes[i] == type) {
            for (std::vector<std::string>::const_iterator it = filterKeysList[i].cbegin();
                    it != filterKeysList[i].cend(); ++it ) {
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

    manageIndexEntry(&favoritesIndexAllKeys, key, remove);
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

    manageIndexEntry(&genreIndexAllKeys, key, remove);

    key = getIndexableKey(game, GENRE_FILTER, true);
    if (!includeUnknown && key == UNKNOWN_LABEL)
        manageIndexEntry(&genreIndexAllKeys, key, remove);
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

    manageIndexEntry(&playersIndexAllKeys, key, remove);
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
        manageIndexEntry(&pubDevIndexAllKeys, pub, remove);
    }
    else {
        if (!unknownDev) {
            // If no info at all.
            manageIndexEntry(&pubDevIndexAllKeys, dev, remove);
        }
        if (!unknownPub) {
            // If no info at all.
            manageIndexEntry(&pubDevIndexAllKeys, pub, remove);
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

    manageIndexEntry(&ratingsIndexAllKeys, key, remove);
}

void FileFilterIndex::manageKidGameEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, KIDGAME_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid kidgame info found.
        return;

    manageIndexEntry(&kidGameIndexAllKeys, key, remove);
}

void FileFilterIndex::manageCompletedEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, COMPLETED_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid completed info found.
        return;

    manageIndexEntry(&completedIndexAllKeys, key, remove);
}

void FileFilterIndex::manageBrokenEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, BROKEN_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid broken info found.
        return;

    manageIndexEntry(&brokenIndexAllKeys, key, remove);
}

void FileFilterIndex::manageHiddenEntryInIndex(FileData* game, bool remove)
{
    // Flag for including unknowns.
    bool includeUnknown = INCLUDE_UNKNOWN;
    std::string key = getIndexableKey(game, HIDDEN_FILTER, false);

    if (!includeUnknown && key == UNKNOWN_LABEL)
        // No valid hidden info found.
        return;

    manageIndexEntry(&hiddenIndexAllKeys, key, remove);
}

void FileFilterIndex::manageIndexEntry(std::map<std::string, int>* index,
        std::string key, bool remove)
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

void FileFilterIndex::clearIndex(std::map<std::string, int> indexMap)
{
    indexMap.clear();
}

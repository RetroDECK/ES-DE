//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileFilterIndex.h
//
//  Gamelist filters.
//

#ifndef ES_APP_FILE_FILTER_INDEX_H
#define ES_APP_FILE_FILTER_INDEX_H

#if defined(__APPLE__)
#include <sstream>
#endif

#include <map>
#include <vector>

class FileData;

enum FilterIndexType {
    NONE,
    FAVORITES_FILTER,
    GENRE_FILTER,
    PLAYER_FILTER,
    PUBDEV_FILTER,
    RATINGS_FILTER,
    KIDGAME_FILTER,
    COMPLETED_FILTER,
    BROKEN_FILTER,
    HIDDEN_FILTER
};

struct FilterDataDecl {
    FilterIndexType type; // Type of filter.
    std::map<std::string, int>* allIndexKeys; // All possible filters for this type.
    bool* filteredByRef; // Is it filtered by this type?
    std::vector<std::string>* currentFilteredKeys; // Current keys being filtered for.
    std::string primaryKey; // Primary key in metadata.
    bool hasSecondaryKey; // Has secondary key for comparison.
    std::string secondaryKey; // What's the secondary key.
    std::string menuLabel; // Text to show in menu.
};

class FileFilterIndex
{
public:
    FileFilterIndex();
    ~FileFilterIndex();
    void addToIndex(FileData* game);
    void removeFromIndex(FileData* game);
    void setFilter(FilterIndexType type, std::vector<std::string>* values);
    void clearAllFilters();
    void debugPrintIndexes();
    bool showFile(FileData* game);
    bool isFiltered() { return (filterByFavorites || filterByGenre || filterByPlayers ||
            filterByPubDev || filterByRatings || filterByKidGame || filterByCompleted ||
            filterByBroken || filterByHidden ); };
    bool isKeyBeingFilteredBy(std::string key, FilterIndexType type);
    std::vector<FilterDataDecl>& getFilterDataDecls();

    void importIndex(FileFilterIndex* indexToImport);
    void resetIndex();
    void resetFilters();
    void setUIModeFilters();

private:
    std::vector<FilterDataDecl> filterDataDecl;
    std::string getIndexableKey(FileData* game, FilterIndexType type, bool getSecondary);

    void manageFavoritesEntryInIndex(FileData* game, bool remove = false);
    void manageGenreEntryInIndex(FileData* game, bool remove = false);
    void managePlayerEntryInIndex(FileData* game, bool remove = false);
    void managePubDevEntryInIndex(FileData* game, bool remove = false);
    void manageRatingsEntryInIndex(FileData* game, bool remove = false);
    void manageKidGameEntryInIndex(FileData* game, bool remove = false);
    void manageCompletedEntryInIndex(FileData* game, bool remove = false);
    void manageBrokenEntryInIndex(FileData* game, bool remove = false);
    void manageHiddenEntryInIndex(FileData* game, bool remove = false);

    void manageIndexEntry(std::map<std::string, int>* index, std::string key, bool remove);

    void clearIndex(std::map<std::string, int> indexMap);

    bool filterByFavorites;
    bool filterByGenre;
    bool filterByPlayers;
    bool filterByPubDev;
    bool filterByRatings;
    bool filterByKidGame;
    bool filterByCompleted;
    bool filterByBroken;
    bool filterByHidden;

    std::map<std::string, int> favoritesIndexAllKeys;
    std::map<std::string, int> genreIndexAllKeys;
    std::map<std::string, int> playersIndexAllKeys;
    std::map<std::string, int> pubDevIndexAllKeys;
    std::map<std::string, int> ratingsIndexAllKeys;
    std::map<std::string, int> kidGameIndexAllKeys;
    std::map<std::string, int> completedIndexAllKeys;
    std::map<std::string, int> brokenIndexAllKeys;
    std::map<std::string, int> hiddenIndexAllKeys;

    std::vector<std::string> favoritesIndexFilteredKeys;
    std::vector<std::string> genreIndexFilteredKeys;
    std::vector<std::string> playersIndexFilteredKeys;
    std::vector<std::string> pubDevIndexFilteredKeys;
    std::vector<std::string> ratingsIndexFilteredKeys;
    std::vector<std::string> kidGameIndexFilteredKeys;
    std::vector<std::string> completedIndexFilteredKeys;
    std::vector<std::string> brokenIndexFilteredKeys;
    std::vector<std::string> hiddenIndexFilteredKeys;

    FileData* mRootFolder;

};

#endif // ES_APP_FILE_FILTER_INDEX_H

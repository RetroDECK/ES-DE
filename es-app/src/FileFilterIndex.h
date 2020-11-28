//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  FileFilterIndex.h
//
//  Gamelist filters.
//

#ifndef ES_APP_FILE_FILTER_INDEX_H
#define ES_APP_FILE_FILTER_INDEX_H

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <sstream>
#endif

#include <map>
#include <string>
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
    void setTextFilter(std::string textFilter);
    std::string getTextFilter() { return mTextFilter; };
    void clearAllFilters();
    void debugPrintIndexes();
    bool showFile(FileData* game);
    bool isFiltered() { return (mFilterByText || mFilterByFavorites || mFilterByGenre ||
            mFilterByPlayers || mFilterByPubDev || mFilterByRatings || mFilterByKidGame ||
            mFilterByCompleted || mFilterByBroken || mFilterByHidden ); };
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

    std::string mTextFilter;
    bool mFilterByText;

    bool mFilterByFavorites;
    bool mFilterByGenre;
    bool mFilterByPlayers;
    bool mFilterByPubDev;
    bool mFilterByRatings;
    bool mFilterByKidGame;
    bool mFilterByCompleted;
    bool mFilterByBroken;
    bool mFilterByHidden;

    std::map<std::string, int> mFavoritesIndexAllKeys;
    std::map<std::string, int> mGenreIndexAllKeys;
    std::map<std::string, int> mPlayersIndexAllKeys;
    std::map<std::string, int> mPubDevIndexAllKeys;
    std::map<std::string, int> mRatingsIndexAllKeys;
    std::map<std::string, int> mKidGameIndexAllKeys;
    std::map<std::string, int> mCompletedIndexAllKeys;
    std::map<std::string, int> mBrokenIndexAllKeys;
    std::map<std::string, int> mHiddenIndexAllKeys;

    std::vector<std::string> mFavoritesIndexFilteredKeys;
    std::vector<std::string> mGenreIndexFilteredKeys;
    std::vector<std::string> mPlayersIndexFilteredKeys;
    std::vector<std::string> mPubDevIndexFilteredKeys;
    std::vector<std::string> mRatingsIndexFilteredKeys;
    std::vector<std::string> mKidGameIndexFilteredKeys;
    std::vector<std::string> mCompletedIndexFilteredKeys;
    std::vector<std::string> mBrokenIndexFilteredKeys;
    std::vector<std::string> mHiddenIndexFilteredKeys;

    FileData* mRootFolder;

};

#endif // ES_APP_FILE_FILTER_INDEX_H

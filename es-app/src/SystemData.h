//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemData.h
//
//  Provides data structures for the game systems and populates and indexes them based
//  on the configuration in es_systems.xml as well as the presence of game ROM files.
//  Also provides functions to read and write to the gamelist files and to handle theme
//  loading.
//

#ifndef ES_APP_SYSTEM_DATA_H
#define ES_APP_SYSTEM_DATA_H

#include "PlatformId.h"

#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

class FileData;
class FileFilterIndex;
class ThemeData;

struct SystemEnvironmentData {
    std::string mStartPath;
    std::vector<std::string> mSearchExtensions;
    std::string mLaunchCommand;
    std::vector<PlatformIds::PlatformId> mPlatformIds;
};

class FindRules
{
public:
    FindRules();

    void loadFindRules();

private:
    struct EmulatorRules {
#if defined(_WIN64)
        std::vector<std::string> winRegistryPaths;
#endif
        std::vector<std::string> systemPaths;
        std::vector<std::string> staticPaths;
    };

    struct CoreRules {
        std::vector<std::string> corePaths;
    };

    std::map<std::string, struct EmulatorRules> mEmulators;
    std::map<std::string, struct CoreRules> mCores;

    friend FileData;
};

class SystemData
{
public:
    SystemData(const std::string& name,
               const std::string& fullName,
               SystemEnvironmentData* envData,
               const std::string& themeFolder,
               bool CollectionSystem = false,
               bool CustomCollectionSystem = false);

    ~SystemData();

    FileData* getRootFolder() const { return mRootFolder; }
    const std::string& getName() const { return mName; }
    const std::string& getFullName() const { return mFullName; }
    const std::string& getStartPath() const { return mEnvData->mStartPath; }
    const std::vector<std::string>& getExtensions() const { return mEnvData->mSearchExtensions; }
    const std::string& getThemeFolder() const { return mThemeFolder; }
    SystemEnvironmentData* getSystemEnvData() const { return mEnvData; }
    const std::vector<PlatformIds::PlatformId>& getPlatformIds() const
    {
        return mEnvData->mPlatformIds;
    }
    bool hasPlatformId(PlatformIds::PlatformId id)
    {
        if (!mEnvData)
            return false;
        return std::find(mEnvData->mPlatformIds.cbegin(), mEnvData->mPlatformIds.cend(), id) !=
               mEnvData->mPlatformIds.cend();
    }

    const std::shared_ptr<ThemeData>& getTheme() const { return mTheme; }

    std::string getGamelistPath(bool forWrite) const;
    std::string getThemePath() const;

    std::pair<unsigned int, unsigned int> getDisplayedGameCount() const;
    bool getScrapeFlag() { return mScrapeFlag; }
    void setScrapeFlag(bool scrapeflag) { mScrapeFlag = scrapeflag; }

    static void deleteSystems();
    // Loads the systems configuration file at getConfigPath() and creates the systems.
    static bool loadConfig();
    static std::string getConfigPath(bool legacyWarning);

    // Generates the game system directories and information files based on es_systems.xml.
    static bool createSystemDirectories();

    static std::vector<SystemData*> sSystemVector;
    static std::unique_ptr<FindRules> sFindRules;

    std::vector<SystemData*>::const_iterator getIterator() const
    {
        return std::find(sSystemVector.cbegin(), sSystemVector.cend(), this);
    }
    std::vector<SystemData*>::const_reverse_iterator getRevIterator() const
    {
        return std::find(sSystemVector.crbegin(), sSystemVector.crend(), this);
    }
    bool isCollection() { return mIsCollectionSystem; }
    bool isCustomCollection() { return mIsCustomCollectionSystem; }
    bool isGroupedCustomCollection() { return mIsGroupedCustomCollectionSystem; }
    void setIsGroupedCustomCollection(bool isGroupedCustom)
    {
        mIsGroupedCustomCollectionSystem = isGroupedCustom;
    };
    bool isGameSystem() { return mIsGameSystem; }

    bool isVisible();

    SystemData* getNext() const;
    SystemData* getPrev() const;
    static SystemData* getRandomSystem(const SystemData* currentSystem);
    FileData* getRandomGame(const FileData* currentGame = nullptr);
    FileData* getPlaceholder() { return mPlaceholder; }

    void sortSystem(bool reloadGamelist = true, bool jumpToFirstRow = false);

    // Load or re-load theme.
    void loadTheme();

    FileFilterIndex* getIndex() { return mFilterIndex; }
    void onMetaDataSavePoint();
    void writeMetaData();

    void setupSystemSortType(FileData* mRootFolder);

private:
    bool mIsCollectionSystem;
    bool mIsCustomCollectionSystem;
    bool mIsGroupedCustomCollectionSystem;
    bool mIsGameSystem;
    bool mScrapeFlag; // Only used by scraper GUI to remember which systems to scrape.
    std::string mName;
    std::string mFullName;
    SystemEnvironmentData* mEnvData;
    std::string mThemeFolder;
    std::shared_ptr<ThemeData> mTheme;

    bool populateFolder(FileData* folder);
    void indexAllGameFilters(const FileData* folder);
    void setIsGameSystemStatus();

    FileFilterIndex* mFilterIndex;

    FileData* mRootFolder;
    FileData* mPlaceholder;
};

#endif // ES_APP_SYSTEM_DATA_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CollectionSystemManager.h
//
//  Manages collections of the following two types:
//  1) Automatically populated (All games, Favorites and Recent/Last Played)
//  2) Custom/user-created (could be any number of these)
//
//  The automatic collections are basically virtual systems that have no
//  gamelist.xml files and that only exist in memory during the program session.
//  SystemData sets up the basic data structures and CollectionSystemManager
//  populates and manages the collections.
//
//  The custom collections have simple data files which are just lists of ROM files.
//
//  In addition to this, CollectionSystemManager also handles some logic for
//  normal systems such as adding and removing favorite games, including triggering
//  the required re-sort and refresh of the gamelists.
//

#ifndef ES_APP_COLLECTION_SYSTEM_MANAGER_H
#define ES_APP_COLLECTION_SYSTEM_MANAGER_H

#include "utils/StringUtil.h"

#include <map>
#include <string>
#include <vector>

class FileData;
class SystemData;
class Window;
struct SystemEnvironmentData;

enum CollectionSystemType {
    AUTO_ALL_GAMES,
    AUTO_LAST_PLAYED,
    AUTO_FAVORITES,
    CUSTOM_COLLECTION
};

struct CollectionSystemDecl {
    CollectionSystemType type;
    std::string name;
    std::string longName;
    std::string themeFolder;
    bool isCustom;
};

struct CollectionSystemData {
    SystemData* system;
    CollectionSystemDecl decl;
    bool isEnabled;
    bool isPopulated;
};

struct stringComparator {
    bool operator()(const std::string& a, const std::string& b) const
    {
        return Utils::String::toUpper(a) < Utils::String::toUpper(b);
    }
};

class CollectionSystemManager
{
public:
    CollectionSystemManager(Window* window);
    ~CollectionSystemManager();

    static CollectionSystemManager* get();
    static void init(Window* window);
    static void deinit();
    void saveCustomCollection(SystemData* sys);

    void loadCollectionSystems();
    void loadEnabledListFromSettings();
    void updateSystemsList();

    void refreshCollectionSystems(FileData* file);
    void updateCollectionSystem(FileData* file, CollectionSystemData sysData);
    void deleteCollectionFiles(FileData* file);

    inline std::map<std::string, CollectionSystemData, stringComparator>
            getAutoCollectionSystems() { return mAutoCollectionSystemsData; };
    inline std::map<std::string, CollectionSystemData, stringComparator>
            getCustomCollectionSystems() { return mCustomCollectionSystemsData; };
    inline SystemData* getCustomCollectionsBundle() { return mCustomCollectionsBundle; };
    std::vector<std::string> getUnusedSystemsFromTheme();
    SystemData* addNewCustomCollection(std::string name);

    bool isThemeGenericCollectionCompatible(bool genericCustomCollections);
    bool isThemeCustomCollectionCompatible(std::vector<std::string> stringVector);
    std::string getValidNewCollectionName(std::string name, int index = 0);

    void setEditMode(std::string collectionName);
    void exitEditMode();
    inline bool isEditing() { return mIsEditingCustom; };
    inline std::string getEditingCollection() { return mEditingCollection; };
    bool toggleGameInCollection(FileData* file);

    SystemData* getSystemToView(SystemData* sys);
    FileData* updateCollectionFolderMetadata(SystemData* sys);

    bool getIsCustomCollection(SystemData* system);

private:
    static CollectionSystemManager* sInstance;
    SystemEnvironmentData* mCollectionEnvData;
    std::map<std::string, CollectionSystemDecl, stringComparator> mCollectionSystemDeclsIndex;
    std::map<std::string, CollectionSystemData, stringComparator> mAutoCollectionSystemsData;
    std::map<std::string, CollectionSystemData, stringComparator> mCustomCollectionSystemsData;
    Window* mWindow;
    bool mIsEditingCustom;
    bool mHasEnabledCustomCollection;
    std::string mEditingCollection;
    CollectionSystemData* mEditingCollectionSystemData;
    SystemData* mCustomCollectionsBundle;

    void initAutoCollectionSystems();
    void initCustomCollectionSystems();
    SystemData* getAllGamesCollection();
    SystemData* createNewCollectionEntry(std::string name,
            CollectionSystemDecl sysDecl, bool index = true, bool custom = false);
    void populateAutoCollection(CollectionSystemData* sysData);
    void populateCustomCollection(CollectionSystemData* sysData);

    void removeCollectionsFromDisplayedSystems();
    void addEnabledCollectionsToDisplayedSystems(std::map<std::string,
            CollectionSystemData, stringComparator>* colSystemData);

    std::vector<std::string> getSystemsFromConfig();
    std::vector<std::string> getSystemsFromTheme();
    std::vector<std::string> getCollectionsFromConfigFolder();
    std::vector<std::string> getCollectionThemeFolders(bool custom);
    std::vector<std::string> getUserCollectionThemeFolders();

    void trimCollectionCount(FileData* rootFolder, int limit);
    bool themeFolderExists(std::string folder);
    bool includeFileInAutoCollections(FileData* file);

    std::string getCustomCollectionConfigPath(std::string collectionName);
    std::string getCollectionsFolder();
};

#endif // ES_APP_COLLECTION_SYSTEM_MANAGER_H

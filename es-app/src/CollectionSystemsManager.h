//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CollectionSystemsManager.h
//
//  Manages collections of the following two types:
//  1) Automatically populated (All games, Favorites and Recent/Last Played)
//  2) Custom/user-created (could be any number of these)
//
//  The automatic collections are basically virtual systems that have no
//  gamelist.xml files and that only exist in memory during the program session.
//  SystemData sets up the basic data structures and CollectionSystemsManager
//  populates and manages the collections.
//
//  The custom collections have simple data files which are just lists of ROM files.
//
//  In addition to this, CollectionSystemsManager also handles some logic for
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

class CollectionSystemsManager
{
public:
    CollectionSystemsManager(Window* window);
    ~CollectionSystemsManager();

    static CollectionSystemsManager* get();
    static void init(Window* window);
    static void deinit();
    void saveCustomCollection(SystemData* sys);

    // Functions to load all collections into memory, and enable the active ones:
    // Load all collection systems.
    void loadCollectionSystems();
    // Load settings.
    void loadEnabledListFromSettings();
    // Update enabled system list in System View.
    void updateSystemsList();

    // Functions to manage collection files related to a source FileData:
    // Update all collection files related to the source file.
    void refreshCollectionSystems(FileData* file);
    // Update the collections, such as when marking or unmarking a game as favorite.
    void updateCollectionSystem(FileData* file, CollectionSystemData sysData);
    // Delete all collection files from all collection systems related to the source file.
    void deleteCollectionFiles(FileData* file);

    // Return whether the current theme is compatible with Automatic or Custom Collections.
    bool isThemeGenericCollectionCompatible(bool genericCustomCollections);
    bool isThemeCustomCollectionCompatible(std::vector<std::string> stringVector);
    std::string getValidNewCollectionName(std::string name, int index = 0);

    void setEditMode(std::string collectionName, bool showPopup = true);
    void exitEditMode(bool showPopup = true);
    bool inCustomCollection(const std::string& collectionName, FileData* gameFile);
    // Add or remove a game from a specific collection.
    bool toggleGameInCollection(FileData* file);

    SystemData* getSystemToView(SystemData* sys);
    // Used to generate a description of the collection (all other metadata fields are hidden).
    FileData* updateCollectionFolderMetadata(SystemData* sys);
    // Return the unused folders from current theme path.
    std::vector<std::string> getUnusedSystemsFromTheme();

    SystemData* addNewCustomCollection(std::string name);
    void deleteCustomCollection(std::string collectionName);

    // Reactivate a game in all custom collections where it has an entry in the configuration file.
    void reactivateCustomCollectionEntry(FileData* game);

    // Repopulate the collection, which is basically a forced update of its complete content.
    void repopulateCollection(SystemData* sysData);

    inline std::map<std::string, CollectionSystemData, stringComparator>
            getAutoCollectionSystems() { return mAutoCollectionSystemsData; };
    inline std::map<std::string, CollectionSystemData, stringComparator>
            getCustomCollectionSystems() { return mCustomCollectionSystemsData; };
    inline SystemData* getCustomCollectionsBundle() { return mCustomCollectionsBundle; };
    inline bool isEditing() { return mIsEditingCustom; };
    inline std::string getEditingCollection() { return mEditingCollection; };

private:
    static CollectionSystemsManager* sInstance;
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

    // Functions to handle the initialization and loading of collection systems:
    // Loads Automatic Collection systems (All, Favorites, Last Played).
    void initAutoCollectionSystems();
    void initCustomCollectionSystems();
    SystemData* getAllGamesCollection();
    // Create a new empty collection system based on the name and declaration.
    SystemData* createNewCollectionEntry(std::string name,
            CollectionSystemDecl sysDecl, bool index = true, bool custom = false);
    // Populate an automatic collection system.
    void populateAutoCollection(CollectionSystemData* sysData);
    // Populate a custom collection system.
    void populateCustomCollection(CollectionSystemData* sysData);

    // Functions to handle System View removal and insertion of collections:
    void removeCollectionsFromDisplayedSystems();
    void addEnabledCollectionsToDisplayedSystems(std::map<std::string,
            CollectionSystemData, stringComparator>* colSystemData);

    // Auxiliary functions:
    std::vector<std::string> getSystemsFromConfig();
    std::vector<std::string> getSystemsFromTheme();
    // Return which collection config files exist in the user folder.
    std::vector<std::string> getCollectionsFromConfigFolder();
    // Return the theme folders for automatic collections (All, Favorites and Last Played)
    // or a generic custom collections folder.
    std::vector<std::string> getCollectionThemeFolders(bool custom);
    // Return the theme folders in use for the user-defined custom collections.
    std::vector<std::string> getUserCollectionThemeFolders();
    void trimCollectionCount(FileData* rootFolder, int limit);
    // Return whether a specific folder exists in the theme.
    bool themeFolderExists(std::string folder);
    bool includeFileInAutoCollections(FileData* file);

    std::string getCustomCollectionConfigPath(std::string collectionName);
    std::string getCollectionsFolder();
};

#endif // ES_APP_COLLECTION_SYSTEM_MANAGER_H

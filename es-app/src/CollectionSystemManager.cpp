//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CollectionSystemManager.cpp
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

#include "CollectionSystemManager.h"

#include "guis/GuiInfoPopup.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/gamelist/IGameListView.h"
#include "views/ViewController.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "ThemeData.h"

#include <fstream>
#include <pugixml.hpp>

std::string myCollectionsName = "collections";

#define LAST_PLAYED_MAX	50

// Handles the getting, initialization, deinitialization,
// saving and deletion of a CollectionSystemManager instance.
CollectionSystemManager* CollectionSystemManager::sInstance = nullptr;

CollectionSystemManager::CollectionSystemManager(Window* window) : mWindow(window)
{
    CollectionSystemDecl systemDecls[] = {
    //  Type                  Name                Long name       Theme folder           isCustom
        { AUTO_ALL_GAMES,     "all",              "all games",    "auto-allgames",       false },
        { AUTO_LAST_PLAYED,   "recent",           "last played",  "auto-lastplayed",     false },
        { AUTO_FAVORITES,     "favorites",        "favorites",    "auto-favorites",      false },
        { CUSTOM_COLLECTION,  myCollectionsName,  "collections",  "custom-collections",  true  }
    };

    // Create a map of the collections.
    std::vector<CollectionSystemDecl> tempSystemDecl = std::vector<CollectionSystemDecl>
            (systemDecls, systemDecls + sizeof(systemDecls) / sizeof(systemDecls[0]));

    for (std::vector<CollectionSystemDecl>::const_iterator it = tempSystemDecl.cbegin();
            it != tempSystemDecl.cend(); ++it )
        mCollectionSystemDeclsIndex[(*it).name] = (*it);

    // Setup the standard environment.
    mCollectionEnvData = new SystemEnvironmentData;
    mCollectionEnvData->mStartPath = "";
    std::vector<std::string> exts;
    mCollectionEnvData->mSearchExtensions = exts;
    mCollectionEnvData->mLaunchCommand = "";
    std::vector<PlatformIds::PlatformId> allPlatformIds;
    allPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);
    mCollectionEnvData->mPlatformIds = allPlatformIds;

    std::string path = getCollectionsFolder();
    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    mIsEditingCustom = false;
    mEditingCollection = "Favorites";
    mEditingCollectionSystemData = nullptr;
    mCustomCollectionsBundle = nullptr;
}

CollectionSystemManager::~CollectionSystemManager()
{
    assert(sInstance == this);

    // Don't attempt to remove any collections if no systems exist.
    if (SystemData::sSystemVector.size() > 0)
        removeCollectionsFromDisplayedSystems();

    // Delete all custom collections.
    for (std::map<std::string, CollectionSystemData>::const_iterator
            it = mCustomCollectionSystemsData.cbegin();
            it != mCustomCollectionSystemsData.cend() ; it++)
        delete it->second.system;

    // Delete the custom collections bundle.
    if (mCustomCollectionsBundle)
        delete mCustomCollectionsBundle;

    // Delete the auto collections systems.
    for (auto it = mAutoCollectionSystemsData.cbegin();
            it != mAutoCollectionSystemsData.cend(); it++) {
        delete (*it).second.system;
    }

    delete mCollectionEnvData;
    sInstance = nullptr;
}

CollectionSystemManager* CollectionSystemManager::get()
{
    assert(sInstance);
    return sInstance;
}

void CollectionSystemManager::init(Window* window)
{
    assert(!sInstance);
    sInstance = new CollectionSystemManager(window);
}

void CollectionSystemManager::deinit()
{
    if (sInstance)
        delete sInstance;
}

void CollectionSystemManager::saveCustomCollection(SystemData* sys)
{
    const std::string rompath  = FileData::getROMDirectory();
    std::string name = sys->getName();
    std::unordered_map<std::string, FileData*>
            games = sys->getRootFolder()->getChildrenByFilename();
    bool found = mCustomCollectionSystemsData.find(name) != mCustomCollectionSystemsData.cend();

    if (found) {
        CollectionSystemData sysData = mCustomCollectionSystemsData.at(name);
        // Read back any entries from the configuration file for game files that are
        // currently missing, and combine them with the active content. If we wouldn't do
        // this, they would be purged from the collection. Maybe a directory has been
        // temporarily moved or the files are not reachable for whatever reason. It would
        // be incredibly annoying to have entries purged from the collection in such
        // instances. Using the logic below, the handling of custom collections corresponds
        // to the handling of gamelist.xml files, i.e. it's up to the user to make a
        // conscious decision of what entries to remove.
        std::vector<std::string> fileGameEntries;
        std::vector<std::string> activeGameEntries;
        std::ifstream configFileIn;
        std::ofstream configFileOut;

        #if defined(_WIN64)
        configFileIn.open(Utils::String::
                stringToWideString(getCustomCollectionConfigPath(name)).c_str());
        #else
        configFileIn.open(getCustomCollectionConfigPath(name));
        #endif
        for (std::string gameEntry; getline(configFileIn, gameEntry); ) {
            std::string gamePath = Utils::String::replace(gameEntry, "%ROMPATH%", rompath);
            gamePath = Utils::String::replace(gamePath, "//", "/");
            // Only add the entry if it's not a regular file or a symlink, in other words
            // only add missing files.
            if (!Utils::FileSystem::isRegularFile(gamePath) &&
                    !Utils::FileSystem::isSymlink(gamePath))
                fileGameEntries.push_back(gameEntry);
        }
        configFileIn.close();

        for (std::unordered_map<std::string, FileData*>::const_iterator
                iter = games.cbegin(); iter != games.cend(); ++iter) {
            std::string path = iter->first;
            // If the ROM path of the game begins with the path from the setting
            // ROMDirectory (or the default ROM directory), then replace it with %ROMPATH%.
            if (path.find(rompath) == 0)
                path.replace(0, rompath.size(), "%ROMPATH%/");

            activeGameEntries.push_back(path);
        }

        fileGameEntries.insert(fileGameEntries.cend(), activeGameEntries.cbegin(),
                activeGameEntries.cend());
        std::sort(fileGameEntries.begin(), fileGameEntries.end());
        auto last = std::unique(fileGameEntries.begin(), fileGameEntries.end());
        fileGameEntries.erase(last, fileGameEntries.end());

        #if defined(_WIN64)
        configFile.open(Utils::String::
                stringToWideString(getCustomCollectionConfigPath(name)).c_str());
        #else
        configFileOut.open(getCustomCollectionConfigPath(name));
        #endif

        for (auto it = fileGameEntries.cbegin(); it != fileGameEntries.cend(); it++)
            configFileOut << (*it) << std::endl;

        configFileOut.close();
    }
    else {
        LOG(LogError) << "Couldn't find collection to save: " << name;
    }
}

// Functions below to load all collections into memory, and to enable the active ones.

// Load all collection systems.
void CollectionSystemManager::loadCollectionSystems()
{
    initAutoCollectionSystems();
    CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
    mCustomCollectionsBundle = createNewCollectionEntry(decl.name, decl, false);

    // We will also load custom systems here.
    initCustomCollectionSystems();

    if (Settings::getInstance()->getString("CollectionSystemsAuto") != "" ||
            Settings::getInstance()->getString("CollectionSystemsCustom") != "") {
        // Now see which ones are enabled.
        loadEnabledListFromSettings();
        // Add to the main System Vector, and create Views as needed.
        updateSystemsList();
    }
}

// Load settings.
void CollectionSystemManager::loadEnabledListFromSettings()
{
    // We parse the auto collection settings list.
    std::vector<std::string> autoSelected = Utils::String::commaStringToVector(
            Settings::getInstance()->getString("CollectionSystemsAuto"), true);

    // Iterate the map.
    for (std::map<std::string, CollectionSystemData>::iterator
            it = mAutoCollectionSystemsData.begin();
            it != mAutoCollectionSystemsData.end() ; it++ ) {

        it->second.isEnabled = (std::find(autoSelected.cbegin(),
                autoSelected.cend(), it->first) != autoSelected.cend());
    }

    mHasEnabledCustomCollection = false;

    // Parse the custom collection settings list.
    std::vector<std::string> customSelected = Utils::String::commaStringToVector(
            Settings::getInstance()->getString("CollectionSystemsCustom"), true);

    // Iterate the map.
    for (std::map<std::string, CollectionSystemData>::iterator
            it = mCustomCollectionSystemsData.begin();
            it != mCustomCollectionSystemsData.end() ; it++ ) {

        it->second.isEnabled = (std::find(customSelected.cbegin(),
                customSelected.cend(), it->first) != customSelected.cend());
        if (it->second.isEnabled)
            mHasEnabledCustomCollection = true;
    }
}

// Update enabled system list in System View.
void CollectionSystemManager::updateSystemsList()
{
    // Remove all collection systems.
    removeCollectionsFromDisplayedSystems();
    // Add custom enabled collections.
    addEnabledCollectionsToDisplayedSystems(&mCustomCollectionSystemsData);

    // Don't sort bundled collections unless at least one collection is enabled.
    if (!mIsEditingCustom && mHasEnabledCustomCollection) {
        FileData* rootFolder = mCustomCollectionsBundle->getRootFolder();
        // Sort the bundled custom collections.
        if (rootFolder->getChildren().size() > 0) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->
                    getSortTypeString()), Settings::getInstance()->getBool("FavFirstCustom"));
            SystemData::sSystemVector.push_back(mCustomCollectionsBundle);
        }
    }

    // Add auto enabled collections.
    addEnabledCollectionsToDisplayedSystems(&mAutoCollectionSystemsData);

    // Create views for collections, before reload.
    for (auto sysIt = SystemData::sSystemVector.cbegin();
            sysIt != SystemData::sSystemVector.cend(); sysIt++) {
        if ((*sysIt)->isCollection())
            ViewController::get()->getGameListView((*sysIt));
    }

    // If we were editing a custom collection, and it's no longer enabled, exit edit mode.
    if (mIsEditingCustom && !mEditingCollectionSystemData->isEnabled) {
        exitEditMode();
    }
}

// Functions below to manage collection files related to a source FileData.

// Update all collection files related to the source file.
void CollectionSystemManager::refreshCollectionSystems(FileData* file)
{
    if (!file->getSystem()->isGameSystem() || file->getType() != GAME)
        return;

    // If not a collection but rather a real system, then pretend to be a
    // collection in order to be properly processed by updateCollectionSystem().
    // It's seemingly a bit strange, but without rewriting a lot of code for how
    // systems and collections are handled, it's likely the best approach.
    if (!file->getSystem()->isCollection()) {
        CollectionSystemData realSys;
        realSys.system = file->getSystem();
        realSys.isEnabled = true;
        realSys.isPopulated = true;
        realSys.decl.isCustom = false;

        updateCollectionSystem(file, realSys);
    }

    std::map<std::string, CollectionSystemData> allCollections;
    allCollections.insert(mAutoCollectionSystemsData.cbegin(),
            mAutoCollectionSystemsData.cend());
    allCollections.insert(mCustomCollectionSystemsData.cbegin(),
            mCustomCollectionSystemsData.cend());

    for (auto sysDataIt = allCollections.cbegin();
            sysDataIt != allCollections.cend(); sysDataIt++) {
        if (sysDataIt->second.isEnabled)
            updateCollectionSystem(file, sysDataIt->second);
    }
}

void CollectionSystemManager::updateCollectionSystem(FileData* file, CollectionSystemData sysData)
{
    if (sysData.isPopulated) {
        // Collection files use the full path as key, to avoid clashes.
        std::string key = file->getFullPath();

        SystemData* curSys = sysData.system;
        bool mFavoritesSorting = false;

        // Read the applicable favorite sorting setting depending on whether the
        // system is a custom collection or not.
        if (sysData.decl.isCustom)
            mFavoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
        else
            mFavoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

        const std::unordered_map<std::string, FileData*>&children =
                curSys->getRootFolder()->getChildrenByFilename();

        bool found = children.find(key) != children.cend();
        FileData* rootFolder = curSys->getRootFolder();
        FileFilterIndex* fileIndex = curSys->getIndex();
        std::string name = curSys->getName();

        if (found) {
            // If we found it, we need to update it.
            FileData* collectionEntry = children.at(key);
            // Remove it from the index, so we can re-index the metadata after refreshing.
            fileIndex->removeFromIndex(collectionEntry);
            collectionEntry->refreshMetadata();
            // Found it, and we are removing it.
            if (name == "favorites" && file->metadata.get("favorite") == "false") {
                // Need to check if it is still marked as favorite, if not remove it.
                ViewController::get()->
                        getGameListView(curSys).get()->remove(collectionEntry, false);
            }
            else if (curSys->isCollection() && !file->getCountAsGame()) {
                // If the countasgame flag has been set to false, then remove the game.
                ViewController::get()->
                        getGameListView(curSys).get()->remove(collectionEntry, false);
            }

            else {
                // Re-index with new metadata.
                fileIndex->addToIndex(collectionEntry);
                ViewController::get()->onFileChanged(collectionEntry, FILE_METADATA_CHANGED);
            }
        }
        else {
            bool addGame = false;
            // We didn't find it here - we need to check if we should add it.
            if ((name == "recent" && file->metadata.get("playcount") > "0" &&
                    file->getCountAsGame() && includeFileInAutoCollections(file)) ||
                    (name == "favorites" && file->metadata.get("favorite") == "true" &&
                    file->getCountAsGame()))
                addGame = true;
            else if (name == "all" && file->getCountAsGame())
                addGame = true;
            if (addGame) {
                CollectionFileData* newGame = new CollectionFileData(file, curSys);
                rootFolder->addChild(newGame);
                fileIndex->addToIndex(newGame);
                ViewController::get()->onFileChanged(file, FILE_METADATA_CHANGED);
                ViewController::get()->
                        getGameListView(curSys)->onFileChanged(newGame, FILE_METADATA_CHANGED);
            }
        }

        if (name == "recent") {
            rootFolder->sort(rootFolder->getSortTypeFromString("last played, descending"));
        }
        else if (sysData.decl.isCustom &&
                !Settings::getInstance()->getBool("UseCustomCollectionsSystem")) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                    mFavoritesSorting);
        }
        // If the game doesn't exist in the current system and it's a custom
        // collection, then skip the sorting.
        else if (sysData.decl.isCustom &&
                children.find(file->getFullPath()) != children.cend()) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                    mFavoritesSorting);
        }
        else if (!sysData.decl.isCustom) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                    mFavoritesSorting);
        }

        if (name == "recent") {
            trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
            ViewController::get()->onFileChanged(rootFolder, FILE_METADATA_CHANGED);

            // Select the first row of the gamelist (the game just played).
            IGameListView* gameList =
                    ViewController::get()->getGameListView(getSystemToView(sysData.system)).get();
            gameList->setCursor(gameList->getFirstEntry());
        }
        else {
            ViewController::get()->onFileChanged(rootFolder, FILE_SORTED);
            // If it's a custom collection and the setting to group the collections is
            // enabled, we may have to update the parent instead.
            // However it may not necessarily be so if some collections are themed and
            // some are not, so we always need to check whether a parent exists.
            if (sysData.decl.isCustom &&
                    Settings::getInstance()->getBool("UseCustomCollectionsSystem")) {
                // In case of a returned null pointer, we know there is no parent.
                if (rootFolder->getParent() == nullptr) {
                    ViewController::get()->onFileChanged(rootFolder, FILE_METADATA_CHANGED);
                }
                else {
                    rootFolder->getParent()->sort(rootFolder->getSortTypeFromString(
                            rootFolder->getSortTypeString()), mFavoritesSorting);
                    ViewController::get()->onFileChanged(
                            rootFolder->getParent(), FILE_METADATA_CHANGED);
                }
            }
        }
    }
}

void CollectionSystemManager::trimCollectionCount(FileData* rootFolder, int limit)
{
    SystemData* curSys = rootFolder->getSystem();
    while ((int)rootFolder->getChildrenListToDisplay().size() > limit) {
        CollectionFileData* gameToRemove =
                (CollectionFileData*)rootFolder->getChildrenListToDisplay().back();
        ViewController::get()->getGameListView(curSys).get()->remove(gameToRemove, false);
    }
}

// Delete all collection files from collection systems related to the source file.
void CollectionSystemManager::deleteCollectionFiles(FileData* file)
{
    // Collection files use the full path as key, to avoid clashes.
    std::string key = file->getFullPath();

    // Find games in collection systems.
    std::map<std::string, CollectionSystemData> allCollections;
    allCollections.insert(mAutoCollectionSystemsData.cbegin(),
            mAutoCollectionSystemsData.cend());
    allCollections.insert(mCustomCollectionSystemsData.cbegin(),
            mCustomCollectionSystemsData.cend());

    for (auto sysDataIt = allCollections.begin(); sysDataIt != allCollections.end(); sysDataIt++) {
        if (sysDataIt->second.isPopulated) {
            const std::unordered_map<std::string, FileData*>& children =
                    (sysDataIt->second.system)->getRootFolder()->getChildrenByFilename();

            bool found = children.find(key) != children.cend();
            if (found) {
                FileData* collectionEntry = children.at(key);
                SystemData* systemViewToUpdate = getSystemToView(sysDataIt->second.system);
                ViewController::get()->getGameListView(systemViewToUpdate).get()->
                        remove(collectionEntry, false);
                if (sysDataIt->second.decl.isCustom)
                    saveCustomCollection(sysDataIt->second.system);
            }
        }
    }
}

// Return whether the current theme is compatible with Automatic or Custom Collections.
bool CollectionSystemManager::isThemeGenericCollectionCompatible(bool genericCustomCollections)
{
    std::vector<std::string> cfgSys = getCollectionThemeFolders(genericCustomCollections);
    for (auto sysIt = cfgSys.cbegin(); sysIt != cfgSys.cend(); sysIt++) {
        if (!themeFolderExists(*sysIt))
            return false;
    }
    return true;
}

bool CollectionSystemManager::isThemeCustomCollectionCompatible(
        std::vector<std::string> stringVector)
{
    if (isThemeGenericCollectionCompatible(true))
        return true;

    // Get theme path.
    auto themeSets = ThemeData::getThemeSets();
    auto set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
    if (set != themeSets.cend()) {
        std::string defaultThemeFilePath = set->second.path + "/theme.xml";
        if (Utils::FileSystem::exists(defaultThemeFilePath))
            return true;
    }

    for (auto sysIt = stringVector.cbegin(); sysIt != stringVector.cend(); sysIt++) {
        if (!themeFolderExists(*sysIt))
            return false;
    }
    return true;
}

std::string CollectionSystemManager::getValidNewCollectionName(std::string inName, int index)
{
    std::string name = inName;

    if (index == 0) {
        size_t remove = std::string::npos;

        // Get valid name.
        while ((remove = name.find_first_not_of(
                "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-[]() "))
                != std::string::npos)
            name.erase(remove, 1);
    }
    else {
        name += " (" + std::to_string(index) + ")";
    }

    if (name == "") {
        name = "New Collection";
    }

    if (name != inName) {
        LOG(LogInfo) << "Had to change name, from: " << inName << " to: " << name;
    }

    // Get used systems from es_systems.cfg.
    std::vector<std::string> systemsInUse = getSystemsFromConfig();
    // Get folders assigned to custom collections.
    std::vector<std::string> autoSys = getCollectionThemeFolders(false);
    // Get folder assigned to custom collections.
    std::vector<std::string> customSys = getCollectionThemeFolders(true);
    // Get folders assigned to user collections.
    std::vector<std::string> userSys = getUserCollectionThemeFolders();
    // Add them all to the list of systems in use.
    systemsInUse.insert(systemsInUse.cend(), autoSys.cbegin(), autoSys.cend());
    systemsInUse.insert(systemsInUse.cend(), customSys.cbegin(), customSys.cend());
    systemsInUse.insert(systemsInUse.cend(), userSys.cbegin(), userSys.cend());

    for (auto sysIt = systemsInUse.cbegin(); sysIt != systemsInUse.cend(); sysIt++) {
        if (*sysIt == name) {
            if (index > 0)
                name = name.substr(0, name.size()-4);
            return getValidNewCollectionName(name, index+1);
        }
    }
    // If it matches one of the custom collections reserved names then return it.
    if (mCollectionSystemDeclsIndex.find(name) != mCollectionSystemDeclsIndex.cend())
        return getValidNewCollectionName(name, index+1);
    return name;
}

void CollectionSystemManager::setEditMode(std::string collectionName)
{
    if (mCustomCollectionSystemsData.find(collectionName) == mCustomCollectionSystemsData.cend()) {
        LOG(LogError) << "Tried to edit a non-existing collection: " << collectionName;
        return;
    }
    mIsEditingCustom = true;
    mEditingCollection = collectionName;

    CollectionSystemData* sysData = &(mCustomCollectionSystemsData.at(mEditingCollection));
    if (!sysData->isPopulated) {
        populateCustomCollection(sysData);
    }
    // If it's bundled, this needs to be the bundle system.
    mEditingCollectionSystemData = sysData;

    GuiInfoPopup* s = new GuiInfoPopup(mWindow, "Editing the '" +
            Utils::String::toUpper(collectionName) +
            "' Collection. Add/remove games with Y.", 10000);

    mWindow->setInfoPopup(s);
}

void CollectionSystemManager::exitEditMode()
{
    GuiInfoPopup* s = new GuiInfoPopup(mWindow, "Finished editing the '" +
            mEditingCollection + "' Collection.", 4000);

    mWindow->setInfoPopup(s);
    mIsEditingCustom = false;
    mEditingCollection = "Favorites";

    mEditingCollectionSystemData->system->onMetaDataSavePoint();
}

// Add or remove a game from a specific collection.
bool CollectionSystemManager::toggleGameInCollection(FileData* file)
{
    if (file->getType() == GAME) {
        GuiInfoPopup* s;
        bool adding = true;
        std::string name = file->getName();
        std::string sysName = mEditingCollection;
        if (mIsEditingCustom) {
            SystemData* sysData = mEditingCollectionSystemData->system;

            if (!mEditingCollectionSystemData->isPopulated)
                populateCustomCollection(mEditingCollectionSystemData);

            std::string key = file->getFullPath();
            FileData* rootFolder = sysData->getRootFolder();
            const std::unordered_map<std::string, FileData*>&
                    children = rootFolder->getChildrenByFilename();
            bool found = children.find(key) != children.cend();
            FileFilterIndex* fileIndex = sysData->getIndex();
            std::string name = sysData->getName();

            SystemData* systemViewToUpdate = getSystemToView(sysData);

            if (found) {
                adding = false;
                // If we found it, we need to remove it.
                FileData* collectionEntry = children.at(key);
                // Remove from index.
                fileIndex->removeFromIndex(collectionEntry);
                // Remove from bundle index as well, if needed.
                if (systemViewToUpdate != sysData)
                    systemViewToUpdate->getIndex()->removeFromIndex(collectionEntry);

                ViewController::get()->getGameListView(systemViewToUpdate).get()->
                        remove(collectionEntry, false);
                systemViewToUpdate->getRootFolder()->sort(rootFolder->getSortTypeFromString(
                        rootFolder->getSortTypeString()),
                        Settings::getInstance()->getBool("FavFirstCustom"));
            }
            else {
                // We didn't find it here, so we should add it.
                CollectionFileData* newGame = new CollectionFileData(file, sysData);
                rootFolder->addChild(newGame);
                fileIndex->addToIndex(newGame);
                ViewController::get()->getGameListView(systemViewToUpdate)->
                        onFileChanged(newGame, FILE_METADATA_CHANGED);
                if (name == "recent")
                    rootFolder->sort(rootFolder->getSortTypeFromString("last played, descending"));

                ViewController::get()->onFileChanged(systemViewToUpdate->
                        getRootFolder(), FILE_SORTED);

                // Add to bundle index as well, if needed.
                if (systemViewToUpdate != sysData)
                    systemViewToUpdate->getIndex()->addToIndex(newGame);
                refreshCollectionSystems(newGame);
            }
            updateCollectionFolderMetadata(sysData);
            saveCustomCollection(sysData);
        }
        else {
            file->getSourceFileData()->getSystem()->getIndex()->removeFromIndex(file);
            MetaDataList* md = &file->getSourceFileData()->metadata;
            std::string value = md->get("favorite");
            if (value == "false") {
                md->set("favorite", "true");
            }
            else {
                adding = false;
                md->set("favorite", "false");
            }

            file->getSourceFileData()->getSystem()->getIndex()->addToIndex(file);
            file->getSourceFileData()->getSystem()->onMetaDataSavePoint();
            refreshCollectionSystems(file->getSourceFileData());
        }
        if (adding)
            s = new GuiInfoPopup(mWindow, "Added '" + Utils::String::removeParenthesis(name) +
                    "' to '" + Utils::String::toUpper(sysName) + "'", 4000);
        else
            s = new GuiInfoPopup(mWindow, "Removed '" + Utils::String::removeParenthesis(name) +
                    "' from '" + Utils::String::toUpper(sysName) + "'", 4000);
        mWindow->setInfoPopup(s);
        return true;
    }
    return false;
}

SystemData* CollectionSystemManager::getSystemToView(SystemData* sys)
{
    SystemData* systemToView = sys;
    FileData* rootFolder = sys->getRootFolder();

    FileData* bundleRootFolder = mCustomCollectionsBundle->getRootFolder();
    const std::unordered_map<std::string, FileData*>&
            bundleChildren = bundleRootFolder->getChildrenByFilename();

    // Is the rootFolder bundled in the "My Collections" system?
    bool sysFoundInBundle = bundleChildren.find(rootFolder->getKey()) != bundleChildren.cend();

    if (sysFoundInBundle && sys->isCollection())
        systemToView = mCustomCollectionsBundle;
    return systemToView;
}

// Functions below to Handle loading of collection systems, creating empty ones,
// and populating on demand.

// Loads Automatic Collection systems (All, Favorites, Last Played).
void CollectionSystemManager::initAutoCollectionSystems()
{
    for (std::map<std::string, CollectionSystemDecl>::const_iterator
            it = mCollectionSystemDeclsIndex.cbegin();
            it != mCollectionSystemDeclsIndex.cend() ; it++ ) {
        CollectionSystemDecl sysDecl = it->second;

        if (!sysDecl.isCustom)
            createNewCollectionEntry(sysDecl.name, sysDecl);
    }
}

// This may come in handy if at any point in time in the future we want to
// automatically generate metadata for a folder.
void CollectionSystemManager::updateCollectionFolderMetadata(SystemData* sys)
{
    FileData* rootFolder = sys->getRootFolder();

    std::string desc = "This collection is empty.";
    std::string rating = "0";
    std::string players = "1";
    std::string releasedate = "N/A";
    std::string developer = "None";
    std::string genre = "None";
    std::string video = "";
    std::string thumbnail = "";
    std::string image = "";

    std::unordered_map<std::string, FileData*> games = rootFolder->getChildrenByFilename();

    if (games.size() > 0) {
        std::string games_list = "";
        int games_counter = 0;
        for (std::unordered_map<std::string, FileData*>::const_iterator
                iter = games.cbegin(); iter != games.cend(); ++iter) {

            games_counter++;
            FileData* file = iter->second;

            std::string new_rating = file->metadata.get("rating");
            std::string new_releasedate = file->metadata.get("releasedate");
            std::string new_developer = file->metadata.get("developer");
            std::string new_genre = file->metadata.get("genre");
            std::string new_players = file->metadata.get("players");

            rating = (new_rating > rating ? (new_rating != "" ?
                    new_rating : rating) : rating);

            players = (new_players > players ? (new_players != "" ?
                    new_players : players) : players);

            releasedate = (new_releasedate < releasedate ? (new_releasedate != "" ?
                    new_releasedate : releasedate) : releasedate);

            developer = (developer == "None" ? new_developer : (new_developer != developer ?
                    "Various" : new_developer));

            genre = (genre == "None" ? new_genre : (new_genre != genre ?
                    "Various" : new_genre));

            switch (games_counter) {
                case 2:
                case 3:
                    games_list += ", ";
                case 1:
                    games_list += "'" + file->getName() + "'";
                    break;
                case 4:
                    games_list += " among other titles.";
            }
        }

        desc = "This collection contains " + std::to_string(games_counter) +
                " games, including " + games_list;

        FileData* randomGame = sys->getRandomGame();

        if (randomGame) {
            video = randomGame->getVideoPath();
            thumbnail = randomGame->getThumbnailPath();
            image = randomGame->getImagePath();
        }
    }

    rootFolder->metadata.set("desc", desc);
    rootFolder->metadata.set("rating", rating);
    rootFolder->metadata.set("players", players);
    rootFolder->metadata.set("genre", genre);
    rootFolder->metadata.set("releasedate", releasedate);
    rootFolder->metadata.set("developer", developer);
    rootFolder->metadata.set("video", video);
    rootFolder->metadata.set("thumbnail", thumbnail);
    rootFolder->metadata.set("image", image);
}

void CollectionSystemManager::initCustomCollectionSystems()
{
    std::vector<std::string> systems = getCollectionsFromConfigFolder();
    for (auto nameIt = systems.cbegin(); nameIt != systems.cend(); nameIt++) {
        addNewCustomCollection(*nameIt);
    }
}

SystemData* CollectionSystemManager::getAllGamesCollection()
{
    CollectionSystemData* allSysData = &mAutoCollectionSystemsData["all"];
    if (!allSysData->isPopulated)
        populateAutoCollection(allSysData);

    return allSysData->system;
}

SystemData* CollectionSystemManager::addNewCustomCollection(std::string name)
{
    CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
    decl.themeFolder = name;
    decl.name = name;
    decl.longName = name;

    return createNewCollectionEntry(name, decl, true, true);
}

// Create a new empty collection system based on the name and declaration.
SystemData* CollectionSystemManager::createNewCollectionEntry(
        std::string name, CollectionSystemDecl sysDecl, bool index, bool custom)
{
    SystemData* newSys = new SystemData(
            name, sysDecl.longName, mCollectionEnvData, sysDecl.themeFolder, true, custom);

    CollectionSystemData newCollectionData;
    newCollectionData.system = newSys;
    newCollectionData.decl = sysDecl;
    newCollectionData.isEnabled = false;
    newCollectionData.isPopulated = false;

    if (index) {
        if (!sysDecl.isCustom)
            mAutoCollectionSystemsData[name] = newCollectionData;
        else
            mCustomCollectionSystemsData[name] = newCollectionData;
    }

    return newSys;
}

// Populate an automatic collection system.
void CollectionSystemManager::populateAutoCollection(CollectionSystemData* sysData)
{
    SystemData* newSys = sysData->system;
    CollectionSystemDecl sysDecl = sysData->decl;
    FileData* rootFolder = newSys->getRootFolder();
    FileFilterIndex* index = newSys->getIndex();
    for (auto sysIt = SystemData::sSystemVector.cbegin();
            sysIt != SystemData::sSystemVector.cend(); sysIt++) {
        // We won't iterate all collections.
        if ((*sysIt)->isGameSystem() && !(*sysIt)->isCollection()) {
            std::vector<FileData*> files = (*sysIt)->getRootFolder()->getFilesRecursive(GAME);
            for (auto gameIt = files.cbegin(); gameIt != files.cend(); gameIt++) {
                bool include = includeFileInAutoCollections((*gameIt));

                switch (sysDecl.type) {
                    case AUTO_LAST_PLAYED:
                        include = include && (*gameIt)->metadata.get("playcount") > "0";
                        break;
                    case AUTO_FAVORITES:
                        // We may still want to add files we don't
                        // want in auto collections in "favorites"
                        include = (*gameIt)->metadata.get("favorite") == "true";
                        break;
                    default:
                        break;
                }

                if (include) {
                    // Exclude files that are set not to be counted as games.
                    if (!(*gameIt)->getCountAsGame())
                        continue;

                    CollectionFileData* newGame = new CollectionFileData(*gameIt, newSys);
                    rootFolder->addChild(newGame);
                    index->addToIndex(newGame);
                }
            }
        }
    }
    if (rootFolder->getName() == "recent")
        rootFolder->sort(rootFolder->getSortTypeFromString("last played, descending"));
    else
        rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
            Settings::getInstance()->getBool("FavoritesFirst"));

    if (sysDecl.type == AUTO_LAST_PLAYED)
        trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
    sysData->isPopulated = true;
}

// Populate a custom collection system.
void CollectionSystemManager::populateCustomCollection(CollectionSystemData* sysData)
{
    SystemData* newSys = sysData->system;
    sysData->isPopulated = true;
    CollectionSystemDecl sysDecl = sysData->decl;
    std::string path = getCustomCollectionConfigPath(newSys->getName());

    if (!Utils::FileSystem::exists(path)) {
        LOG(LogInfo) << "Couldn't find custom collection config file \"" << path << "\"";
        return;
    }
    LOG(LogInfo) << "Parsing custom collection file \"" << path << "\"...";

    FileData* rootFolder = newSys->getRootFolder();
    FileFilterIndex* index = newSys->getIndex();

    // Get configuration for this custom collection.
    #if _WIN64
    std::ifstream input(Utils::String::stringToWideString(path).c_str());
    #else
    std::ifstream input(path);
    #endif

    // Get all files map.
    std::unordered_map<std::string,FileData*>
            allFilesMap = getAllGamesCollection()->getRootFolder()->getChildrenByFilename();

    // Get the ROM directory, either as configured in es_settings.cfg, or if no value
    // is set there, then use the default hardcoded path.
    const std::string rompath = FileData::getROMDirectory();

    // Iterate list of files in the config file.
    for (std::string gameKey; getline(input, gameKey); ) {
        // If there is a %ROMPATH% variable set for the game, expand it. By doing this
        // it's possible to use either absolute ROM paths in the collection files or using
        // the path variable. The absolute ROM paths are only used for backward compatibility
        // with old custom collections. All custom collections saved by EmulationStation-DE
        // will use the %ROMPATH% variable instead.
        gameKey = Utils::String::replace(gameKey, "%ROMPATH%", rompath);
        gameKey = Utils::String::replace(gameKey, "//", "/");

        std::unordered_map<std::string,FileData*>::const_iterator it = allFilesMap.find(gameKey);
        if (it != allFilesMap.cend()) {
            CollectionFileData* newGame = new CollectionFileData(it->second, newSys);
            rootFolder->addChild(newGame);
            index->addToIndex(newGame);
        }
        else {
            LOG(LogWarning) << "File \"" << gameKey << "\" does not exist, ignoring entry";
        }
    }

    updateCollectionFolderMetadata(newSys);
}

// Functions below to handle System View removal and insertion of collections.

void CollectionSystemManager::removeCollectionsFromDisplayedSystems()
{
    // Remove all collection Systems.
    for (auto sysIt = SystemData::sSystemVector.cbegin();
            sysIt != SystemData::sSystemVector.cend(); ) {
        if ((*sysIt)->isCollection())
            sysIt = SystemData::sSystemVector.erase(sysIt);
        else
            sysIt++;
    }

    // Remove all custom collections in bundle.
    // This should not delete the objects from memory!
    FileData* customRoot = mCustomCollectionsBundle->getRootFolder();
    std::vector<FileData*> mChildren = customRoot->getChildren();
    for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
        customRoot->removeChild(*it);
    }
    // Clear index.
    mCustomCollectionsBundle->getIndex()->resetIndex();
    // Remove view so it's re-created as needed.
    ViewController::get()->removeGameListView(mCustomCollectionsBundle);
}

void CollectionSystemManager::addEnabledCollectionsToDisplayedSystems(
        std::map<std::string, CollectionSystemData>* colSystemData)
{
    // Add auto enabled collections.
    for (std::map<std::string, CollectionSystemData>::iterator
            it = colSystemData->begin() ; it != colSystemData->end() ; it++ ) {
        if (it->second.isEnabled) {
            // Check if populated, otherwise populate.
            if (!it->second.isPopulated) {
                if (it->second.decl.isCustom)
                    populateCustomCollection(&(it->second));
                else
                    populateAutoCollection(&(it->second));
            }
            // Check if it has its own view.
            if (!it->second.decl.isCustom || themeFolderExists(it->first) ||
                    !Settings::getInstance()->getBool("UseCustomCollectionsSystem")) {
                // Theme folder exists, or we chose not to bundle it under the
                // custom-collections system. So we need to create a view.
                SystemData::sSystemVector.push_back(it->second.system);
                // If this is a non-bundled custom collection, then sort it.
                if (it->second.decl.isCustom == true) {
                    FileData* rootFolder = it->second.system->getRootFolder();
                    rootFolder->sort(rootFolder->getSortTypeFromString(
                            rootFolder->getSortTypeString()),
                            Settings::getInstance()->getBool("FavFirstCustom"));
                    // Jump to the first row of the game list, assuming it's not empty.
                    IGameListView* gameList = ViewController::get()->
                            getGameListView((it->second.system)).get();
                    if (!gameList->getCursor()->isPlaceHolder()) {
                        gameList->setCursor(gameList->getFirstEntry());
                    }
                }
            }
            else {
                FileData* newSysRootFolder = it->second.system->getRootFolder();
                mCustomCollectionsBundle->getRootFolder()->addChild(newSysRootFolder);
                mCustomCollectionsBundle->getIndex()->importIndex(it->second.system->getIndex());
            }
        }
    }
}

// Auxiliary functions below to get available custom collection possibilities.

std::vector<std::string> CollectionSystemManager::getSystemsFromConfig()
{
    std::vector<std::string> systems;
    std::string path = SystemData::getConfigPath(false);

    if (!Utils::FileSystem::exists(path))
        return systems;

    pugi::xml_document doc;
    #if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
    #else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
    #endif

    if (!res)
        return systems;

    // Actually read the file.
    pugi::xml_node systemList = doc.child("systemList");

    if (!systemList)
        return systems;

    for (pugi::xml_node system = systemList.child("system");
            system; system = system.next_sibling("system")) {
        // Theme folder.
        std::string themeFolder = system.child("theme").text().get();
        systems.push_back(themeFolder);
    }
    std::sort(systems.begin(), systems.end());
    return systems;
}

// Get all folders from the current theme path.
std::vector<std::string> CollectionSystemManager::getSystemsFromTheme()
{
    std::vector<std::string> systems;

    auto themeSets = ThemeData::getThemeSets();
    if (themeSets.empty())
        return systems; // No theme sets available.

    std::map<std::string, ThemeSet>::const_iterator
            set = themeSets.find(Settings::getInstance()->getString("ThemeSet"));
    if (set == themeSets.cend()) {
        // Currently selected theme set is missing, so just pick the first available set.
        set = themeSets.cbegin();
        Settings::getInstance()->setString("ThemeSet", set->first);
    }

    std::string themePath = set->second.path;

    if (Utils::FileSystem::exists(themePath)) {
        Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(themePath);

        for (Utils::FileSystem::stringList::const_iterator
                it = dirContent.cbegin(); it != dirContent.cend(); ++it) {
            if (Utils::FileSystem::isDirectory(*it)) {
                // ... here you have a directory.
                std::string folder = *it;
                folder = folder.substr(themePath.size()+1);

                if (Utils::FileSystem::exists(set->second.getThemePath(folder)))
                    systems.push_back(folder);
            }
        }
    }
    std::sort(systems.begin(), systems.end());
    return systems;
}

// Return the unused folders from current theme path.
std::vector<std::string> CollectionSystemManager::getUnusedSystemsFromTheme()
{
    // Get used systems in es_systems.cfg.
    std::vector<std::string> systemsInUse = getSystemsFromConfig();
    // Get available folders in theme.
    std::vector<std::string> themeSys = getSystemsFromTheme();
    // Get folders assigned to custom collections.
    std::vector<std::string> autoSys = getCollectionThemeFolders(false);
    // Get folder assigned to custom collections.
    std::vector<std::string> customSys = getCollectionThemeFolders(true);
    // Get folders assigned to user collections.
    std::vector<std::string> userSys = getUserCollectionThemeFolders();
    // Add them all to the list of systems in use.
    systemsInUse.insert(systemsInUse.cend(), autoSys.cbegin(), autoSys.cend());
    systemsInUse.insert(systemsInUse.cend(), customSys.cbegin(), customSys.cend());
    systemsInUse.insert(systemsInUse.cend(), userSys.cbegin(), userSys.cend());

    for (auto sysIt = themeSys.cbegin(); sysIt != themeSys.cend(); ) {
        if (std::find(systemsInUse.cbegin(), systemsInUse.cend(), *sysIt) != systemsInUse.cend())
            sysIt = themeSys.erase(sysIt);
        else
            sysIt++;
    }
    return themeSys;
}

// Return which collection config files exist in the user folder.
std::vector<std::string> CollectionSystemManager::getCollectionsFromConfigFolder()
{
    std::vector<std::string> systems;
    std::string configPath = getCollectionsFolder();

    if (Utils::FileSystem::exists(configPath))
    {
        Utils::FileSystem::stringList dirContent =
                Utils::FileSystem::getDirContent(configPath);
        for (Utils::FileSystem::stringList::const_iterator
                it = dirContent.cbegin(); it != dirContent.cend(); ++it) {
            if (Utils::FileSystem::isRegularFile(*it)) {
                // It's a file.
                std::string filename = Utils::FileSystem::getFileName(*it);
                // Need to confirm filename matches config format.
                if (filename != "custom-.cfg" && Utils::String::startsWith(
                            filename, "custom-") && Utils::String::endsWith(filename, ".cfg")) {
                    filename = filename.substr(7, filename.size()-11);
                    systems.push_back(filename);
                }
                else {
                    LOG(LogInfo) << "Found non-collection config file in collections folder: "
                            << filename;
                }
            }
        }
    }
    return systems;
}

// Return the theme folders for automatic collections (All, Favorites, Last Played)
// or a generic custom collections folder.
std::vector<std::string> CollectionSystemManager::getCollectionThemeFolders(bool custom)
{
    std::vector<std::string> systems;
    for (std::map<std::string, CollectionSystemDecl>::const_iterator
            it = mCollectionSystemDeclsIndex.cbegin();
            it != mCollectionSystemDeclsIndex.cend() ; it++ ) {
        CollectionSystemDecl sysDecl = it->second;
        if (sysDecl.isCustom == custom)
            systems.push_back(sysDecl.themeFolder);
    }
    return systems;
}

// Return the theme folders in use for the user-defined custom collections.
std::vector<std::string> CollectionSystemManager::getUserCollectionThemeFolders()
{
    std::vector<std::string> systems;
    for (std::map<std::string, CollectionSystemData>::const_iterator
            it = mCustomCollectionSystemsData.cbegin();
            it != mCustomCollectionSystemsData.cend() ; it++ )
        systems.push_back(it->second.decl.themeFolder);
    return systems;
}

// Return whether a specific folder exists in the theme.
bool CollectionSystemManager::themeFolderExists(std::string folder)
{
    std::vector<std::string> themeSys = getSystemsFromTheme();
    return std::find(themeSys.cbegin(), themeSys.cend(), folder) != themeSys.cend();
}

bool CollectionSystemManager::includeFileInAutoCollections(FileData* file)
{
    // We exclude non-game files from collections (i.e. "kodi", entries from non-game systems).
    // If/when there are more in the future, maybe this can be a more complex method, with a
    // proper list, but for now a simple string comparison is more performant.
    return file->getName() != "kodi" && file->getSystem()->isGameSystem();
}

std::string getCustomCollectionConfigPath(std::string collectionName)
{
    return getCollectionsFolder() + "/custom-" + collectionName + ".cfg";
}

std::string getCollectionsFolder()
{
    return Utils::FileSystem::getGenericPath(Utils::FileSystem::getHomePath() +
            "/.emulationstation/collections");
}

bool systemSort(SystemData* sys1, SystemData* sys2)
{
    std::string name1 = Utils::String::toUpper(sys1->getName());
    std::string name2 = Utils::String::toUpper(sys2->getName());
    return name1.compare(name2) < 0;
}

// Return whether the system is a custom collection.
bool CollectionSystemManager::getIsCustomCollection(SystemData* system)
{
    // Iterate the map.
    for (std::map<std::string, CollectionSystemData>::const_iterator
            it = mCustomCollectionSystemsData.cbegin();
            it != mCustomCollectionSystemsData.cend() ; it++) {
        if (it->second.system == system)
            return true;
    }

    return false;
}

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  CollectionSystemsManager.cpp
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

#include "CollectionSystemsManager.h"

#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "ThemeData.h"
#include "guis/GuiInfoPopup.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "views/gamelist/IGameListView.h"

#include <fstream>
#include <pugixml.hpp>
#include <random>

std::string myCollectionsName = "collections";

#define LAST_PLAYED_MAX 50

// Handles the getting, initialization, deinitialization,
// saving and deletion of a CollectionSystemsManager instance.
CollectionSystemsManager* CollectionSystemsManager::sInstance = nullptr;

CollectionSystemsManager::CollectionSystemsManager(Window* window)
    : mWindow(window)
{
    // clang-format off
    CollectionSystemDecl systemDecls[] = {
    //  Type                 Name               Long name      Theme folder          isCustom
        {AUTO_ALL_GAMES,     "all",              "all games",    "auto-allgames",       false},
        {AUTO_LAST_PLAYED,   "recent",           "last played",  "auto-lastplayed",     false},
        {AUTO_FAVORITES,     "favorites",        "favorites",    "auto-favorites",      false},
        {CUSTOM_COLLECTION,  myCollectionsName,  "collections",  "custom-collections",  true }
    };
    // clang-format on

    // Create a map of the collections.
    std::vector<CollectionSystemDecl> tempSystemDecl = std::vector<CollectionSystemDecl>(
        systemDecls, systemDecls + sizeof(systemDecls) / sizeof(systemDecls[0]));

    for (std::vector<CollectionSystemDecl>::const_iterator it = tempSystemDecl.cbegin();
         it != tempSystemDecl.cend(); it++)
        mCollectionSystemDeclsIndex[(*it).name] = (*it);

    // Setup the standard environment.
    mCollectionEnvData = new SystemEnvironmentData;
    mCollectionEnvData->mStartPath = "";
    std::vector<std::string> exts;
    mCollectionEnvData->mSearchExtensions = exts;
    std::vector<std::pair<std::string, std::string>> commands;
    mCollectionEnvData->mLaunchCommands = commands;
    std::vector<PlatformIds::PlatformId> allPlatformIds;
    allPlatformIds.push_back(PlatformIds::PLATFORM_IGNORE);
    mCollectionEnvData->mPlatformIds = allPlatformIds;

    std::string path = getCollectionsFolder();
    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    mIsEditingCustom = false;
    mHasEnabledCustomCollection = false;
    mEditingCollection = "Favorites";
    mEditingCollectionSystemData = nullptr;
    mCustomCollectionsBundle = nullptr;
}

CollectionSystemsManager::~CollectionSystemsManager()
{
    assert(sInstance == this);

    // Don't attempt to remove any collections if no systems exist.
    if (SystemData::sSystemVector.size() > 0)
        removeCollectionsFromDisplayedSystems();

    // Delete all custom collections.
    for (std::map<std::string, CollectionSystemData, stringComparator>::const_iterator it =
             mCustomCollectionSystemsData.cbegin();
         it != mCustomCollectionSystemsData.cend(); it++)
        delete it->second.system;

    // Delete the custom collections bundle.
    if (mCustomCollectionsBundle)
        delete mCustomCollectionsBundle;

    // Delete the auto collections systems.
    for (auto it = mAutoCollectionSystemsData.cbegin(); // Line break.
         it != mAutoCollectionSystemsData.cend(); it++)
        delete (*it).second.system;

    delete mCollectionEnvData;
    sInstance = nullptr;
}

CollectionSystemsManager* CollectionSystemsManager::get()
{
    assert(sInstance);
    return sInstance;
}

void CollectionSystemsManager::init(Window* window)
{
    assert(!sInstance);
    sInstance = new CollectionSystemsManager(window);
}

void CollectionSystemsManager::deinit()
{
    if (sInstance)
        delete sInstance;
}

void CollectionSystemsManager::saveCustomCollection(SystemData* sys)
{
    const std::string rompath = FileData::getROMDirectory();
    std::string name = sys->getName();
    std::unordered_map<std::string, FileData*> games =
        sys->getRootFolder()->getChildrenByFilename();
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
        configFileIn.open(
            Utils::String::stringToWideString(getCustomCollectionConfigPath(name)).c_str());
#else
        configFileIn.open(getCustomCollectionConfigPath(name));
#endif
        for (std::string gameEntry; getline(configFileIn, gameEntry);) {
            std::string gamePath = Utils::String::replace(gameEntry, "%ROMPATH%", rompath);
            gamePath = Utils::String::replace(gamePath, "//", "/");
            // Only add the entry if it's not a regular file or a symlink, in other words
            // only add missing files.
            if (!Utils::FileSystem::isRegularFile(gamePath) &&
                !Utils::FileSystem::isSymlink(gamePath))
                fileGameEntries.push_back(gameEntry);
        }
        configFileIn.close();

        for (std::unordered_map<std::string, FileData*>::const_iterator it = games.cbegin();
             it != games.cend(); it++) {
            std::string path = it->first;
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
        configFileOut.open(
            Utils::String::stringToWideString(getCustomCollectionConfigPath(name)).c_str());
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

void CollectionSystemsManager::loadCollectionSystems()
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

void CollectionSystemsManager::loadEnabledListFromSettings()
{
    // We parse the auto collection settings list.
    std::vector<std::string> autoSelected = Utils::String::delimitedStringToVector(
        Settings::getInstance()->getString("CollectionSystemsAuto"), ",", true);

    // Iterate the map.
    for (std::map<std::string, CollectionSystemData, stringComparator>::iterator it =
             mAutoCollectionSystemsData.begin();
         it != mAutoCollectionSystemsData.end(); it++) {
        it->second.isEnabled = (std::find(autoSelected.cbegin(), autoSelected.cend(), it->first) !=
                                autoSelected.cend());
    }

    mHasEnabledCustomCollection = false;

    // Parse the custom collection settings list.
    std::vector<std::string> customSelected = Utils::String::delimitedStringToVector(
        Settings::getInstance()->getString("CollectionSystemsCustom"), ",", true);

    // Iterate the map.
    for (std::map<std::string, CollectionSystemData, stringComparator>::iterator it =
             mCustomCollectionSystemsData.begin();
         it != mCustomCollectionSystemsData.end(); it++) {
        it->second.isEnabled = (std::find(customSelected.cbegin(), customSelected.cend(),
                                          it->first) != customSelected.cend());
        if (it->second.isEnabled)
            mHasEnabledCustomCollection = true;
    }
}

void CollectionSystemsManager::updateSystemsList()
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
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                             Settings::getInstance()->getBool("FavFirstCustom"));
            SystemData::sSystemVector.push_back(mCustomCollectionsBundle);
        }
    }

    // Add auto enabled collections.
    addEnabledCollectionsToDisplayedSystems(&mAutoCollectionSystemsData);

    // Create views for collections, before reload.
    for (auto sysIt = SystemData::sSystemVector.cbegin(); // Line break.
         sysIt != SystemData::sSystemVector.cend(); sysIt++) {
        if ((*sysIt)->isCollection())
            ViewController::get()->getGameListView((*sysIt));
    }

    // If we were editing a custom collection, and it's no longer enabled, exit edit mode.
    if (mIsEditingCustom && !mEditingCollectionSystemData->isEnabled) {
        exitEditMode();
    }
}

void CollectionSystemsManager::refreshCollectionSystems(FileData* file,
                                                        bool refreshDisabledAutoCollections)
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
    allCollections.insert(mAutoCollectionSystemsData.cbegin(), mAutoCollectionSystemsData.cend());
    allCollections.insert(mCustomCollectionSystemsData.cbegin(),
                          mCustomCollectionSystemsData.cend());

    for (auto sysDataIt = allCollections.cbegin(); // Line break.
         sysDataIt != allCollections.cend(); sysDataIt++) {
        if (sysDataIt->second.isEnabled || (refreshDisabledAutoCollections &&
                                            !sysDataIt->second.system->isGroupedCustomCollection()))
            updateCollectionSystem(file, sysDataIt->second);
    }
}

void CollectionSystemsManager::updateCollectionSystem(FileData* file, CollectionSystemData sysData)
{
    if (sysData.isPopulated) {
        // Skip all custom collections where the game does not exist.
        if (sysData.decl.isCustom) {
            if (!inCustomCollection(sysData.system->getFullName(), file))
                return;
        }

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

        const std::unordered_map<std::string, FileData*>& children =
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
                ViewController::get()->getGameListView(curSys).get()->remove(collectionEntry,
                                                                             false);
            }
            else if (name == "recent" && file->metadata.get("lastplayed") == "0") {
                // If lastplayed is set to 0 it means the entry has been cleared, and the
                // game should therefore be removed.
                ViewController::get()->getGameListView(curSys).get()->remove(collectionEntry,
                                                                             false);
                ViewController::get()->onFileChanged(rootFolder, true);
            }
            else if (curSys->isCollection() && !file->getCountAsGame()) {
                // If the countasgame flag has been set to false, then remove the game.
                if (curSys->isGroupedCustomCollection()) {
                    ViewController::get()
                        ->getGameListView(curSys->getRootFolder()->getParent()->getSystem())
                        .get()
                        ->remove(collectionEntry, false);
                    FileData* parentRootFolder =
                        rootFolder->getParent()->getSystem()->getRootFolder();
                    parentRootFolder->sort(parentRootFolder->getSortTypeFromString(
                                               parentRootFolder->getSortTypeString()),
                                           mFavoritesSorting);
                }
                else {
                    ViewController::get()->getGameListView(curSys).get()->remove(collectionEntry,
                                                                                 false);
                }
                rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                                 mFavoritesSorting);
            }
            else {
                // Re-index with new metadata.
                fileIndex->addToIndex(collectionEntry);
                ViewController::get()->onFileChanged(collectionEntry, true);
            }
        }
        else {
            bool addGame = false;
            // We didn't find the entry in the collection, so we need to check if we should add it.
            if ((name == "recent" && file->metadata.get("playcount") > "0" &&
                 file->getCountAsGame() && includeFileInAutoCollections(file)) ||
                (name == "favorites" && file->metadata.get("favorite") == "true" &&
                 file->getCountAsGame())) {
                addGame = true;
            }
            else if (name == "all" && file->getCountAsGame()) {
                addGame = true;
            }
            if (addGame) {
                CollectionFileData* newGame = new CollectionFileData(file, curSys);
                rootFolder->addChild(newGame);
                fileIndex->addToIndex(newGame);
                ViewController::get()->getGameListView(curSys)->onFileChanged(newGame, true);
            }
        }

        if (name == "recent") {
            rootFolder->sort(rootFolder->getSortTypeFromString("last played, ascending"));
        }
        else if (sysData.decl.isCustom &&
                 !Settings::getInstance()->getBool("UseCustomCollectionsSystem")) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                             mFavoritesSorting);
        }
        // If the game doesn't exist in the current system and it's a custom
        // collection, then skip the sorting.
        else if (sysData.decl.isCustom && children.find(file->getFullPath()) != children.cend()) {
            // For custom collections, update either the actual system or its parent depending
            // on whether the collection is grouped or not.
            if (rootFolder->getSystem()->isGroupedCustomCollection()) {
                rootFolder->getParent()->sort(rootFolder->getParent()->getSortTypeFromString(
                                                  rootFolder->getParent()->getSortTypeString()),
                                              mFavoritesSorting);
            }
            else {
                rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                                 mFavoritesSorting);
            }
        }
        else if (!sysData.decl.isCustom) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                             mFavoritesSorting);
        }

        if (name == "recent") {
            trimCollectionCount(rootFolder, LAST_PLAYED_MAX);
            ViewController::get()->onFileChanged(rootFolder, false);
            // This is a bit of a hack to prevent a jump to the first line of the gamelist
            // if an entry is manually adjusted from within the 'recent' gamelist, for example
            // by toggling a game as favorite. If the time since the last played timestamp is
            // less than two seconds, then assume that the game was actually just launched,
            // and therefore jump to the first line. The two seconds is incredibly generous
            // as normally it would rather be some milliseconds, but who knows what special
            // circumstances could cause a slight delay so let's keep a large margin.
            auto nTime = Utils::Time::now();
            if (nTime - Utils::Time::stringToTime(file->metadata.get("lastplayed")) < 2) {
                // Select the first row of the gamelist (the game just played).
                IGameListView* gameList =
                    ViewController::get()->getGameListView(getSystemToView(sysData.system)).get();
                gameList->setCursor(gameList->getFirstEntry());
            }
        }
        else {
            ViewController::get()->onFileChanged(rootFolder, true);
            // For custom collections, update either the actual system or its parent depending
            // on whether the collection is grouped or not.
            if (sysData.decl.isCustom) {
                if (rootFolder->getSystem()->isGroupedCustomCollection())
                    ViewController::get()->onFileChanged(rootFolder->getParent(), true);
                else
                    ViewController::get()->onFileChanged(rootFolder, true);
            }
        }
    }
}

void CollectionSystemsManager::deleteCollectionFiles(FileData* file)
{
    // Collection files use the full path as key, to avoid clashes.
    std::string key = file->getFullPath();

    // Find games in collection systems.
    std::map<std::string, CollectionSystemData> allCollections;
    allCollections.insert(mAutoCollectionSystemsData.cbegin(), mAutoCollectionSystemsData.cend());
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
                ViewController::get()
                    ->getGameListView(systemViewToUpdate)
                    .get()
                    ->remove(collectionEntry, false);
                if (sysDataIt->second.decl.isCustom)
                    saveCustomCollection(sysDataIt->second.system);
            }
        }
    }
}

bool CollectionSystemsManager::isThemeGenericCollectionCompatible(bool genericCustomCollections)
{
    std::vector<std::string> cfgSys = getCollectionThemeFolders(genericCustomCollections);
    for (auto sysIt = cfgSys.cbegin(); sysIt != cfgSys.cend(); sysIt++) {
        if (!themeFolderExists(*sysIt))
            return false;
    }
    return true;
}

bool CollectionSystemsManager::isThemeCustomCollectionCompatible(
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

std::string CollectionSystemsManager::getValidNewCollectionName(std::string inName, int index)
{
    std::string name = inName;

    if (index == 0) {
        size_t remove = std::string::npos;
        // Get valid name.
        while ((remove = name.find_first_not_of(
                    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-[]()' ")) !=
               std::string::npos)
            name.erase(remove, 1);
    }
    else {
        name += " (" + std::to_string(index) + ")";
    }

    if (name == "")
        name = "new collection";

    name = Utils::String::toLower(name);

    if (Utils::String::toLower(name) != Utils::String::toLower(inName)) {
        LOG(LogInfo) << "Had to change name, from: " << inName << " to: " << name;
    }

    // Get used systems from es_systems.xml.
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
                name = name.substr(0, name.size() - 4);
            return getValidNewCollectionName(name, index + 1);
        }
    }
    // If it matches one of the custom collections reserved names then return it.
    if (mCollectionSystemDeclsIndex.find(name) != mCollectionSystemDeclsIndex.cend())
        return getValidNewCollectionName(name, index + 1);
    return name;
}

void CollectionSystemsManager::setEditMode(std::string collectionName, bool showPopup)
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

    if (showPopup) {
        GuiInfoPopup* s = new GuiInfoPopup(mWindow,
                                           "EDITING '" + Utils::String::toUpper(collectionName) +
                                               "' COLLECTION, ADD/REMOVE GAMES WITH 'Y'",
                                           10000);

        mWindow->setInfoPopup(s);
    }
}

void CollectionSystemsManager::exitEditMode(bool showPopup)
{
    if (showPopup) {
        GuiInfoPopup* s = new GuiInfoPopup(
            mWindow,
            "FINISHED EDITING '" + Utils::String::toUpper(mEditingCollection) + "' COLLECTION",
            4000);

        mWindow->setInfoPopup(s);
    }

    mIsEditingCustom = false;
    mEditingCollection = "Favorites";

    // Remove all tick marks from the games that are part of the collection.
    for (auto it = SystemData::sSystemVector.begin(); it != SystemData::sSystemVector.end(); it++) {
        ViewController::get()->getGameListView((*it))->onFileChanged(
            ViewController::get()->getGameListView((*it))->getCursor(), false);
    }

    mEditingCollectionSystemData->system->onMetaDataSavePoint();
}

bool CollectionSystemsManager::inCustomCollection(const std::string& collectionName,
                                                  FileData* gameFile)
{
    auto collectionEntry = mCustomCollectionSystemsData.find(collectionName);

    if (collectionEntry != mCustomCollectionSystemsData.end()) {
        const std::unordered_map<std::string, FileData*>& children =
            collectionEntry->second.system->getRootFolder()->getChildrenByFilename();
        return children.find(gameFile->getFullPath()) != children.cend();
    }
    return false;
}

bool CollectionSystemsManager::toggleGameInCollection(FileData* file)
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
            const std::unordered_map<std::string, FileData*>& children =
                rootFolder->getChildrenByFilename();
            bool found = children.find(key) != children.cend();
            FileFilterIndex* fileIndex = sysData->getIndex();
            std::string name = sysData->getName();

            SystemData* systemViewToUpdate = getSystemToView(sysData);

            if (found) {
                adding = false;
                // If we found it, we need to remove it.
                FileData* collectionEntry = children.at(key);
                ViewController::get()
                    ->getGameListView(systemViewToUpdate)
                    .get()
                    ->remove(collectionEntry, false);
                systemViewToUpdate->getRootFolder()->sort(
                    rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                    Settings::getInstance()->getBool("FavFirstCustom"));
                ViewController::get()->reloadGameListView(systemViewToUpdate);

                updateCollectionFolderMetadata(systemViewToUpdate);
            }
            else {
                // We didn't find it here, so we should add it.
                CollectionFileData* newGame = new CollectionFileData(file, sysData);
                rootFolder->addChild(newGame);

                systemViewToUpdate->getRootFolder()->sort(
                    rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                    Settings::getInstance()->getBool("FavFirstCustom"));
                ViewController::get()->onFileChanged(systemViewToUpdate->getRootFolder(), true);
                fileIndex->addToIndex(newGame);

                // Add to bundle index as well, if needed.
                if (systemViewToUpdate != sysData)
                    systemViewToUpdate->getIndex()->addToIndex(newGame);
            }
            saveCustomCollection(sysData);
        }
        else {
            file->getSourceFileData()->getSystem()->getIndex()->removeFromIndex(
                file->getSourceFileData());
            MetaDataList* md = &file->getSourceFileData()->metadata;
            std::string value = md->get("favorite");
            if (value == "false") {
                md->set("favorite", "true");
            }
            else {
                adding = false;
                md->set("favorite", "false");
            }

            file->getSourceFileData()->getSystem()->getIndex()->addToIndex(
                file->getSourceFileData());
            file->getSourceFileData()->getSystem()->onMetaDataSavePoint();
            refreshCollectionSystems(file->getSourceFileData());
            if (mAutoCollectionSystemsData["favorites"].isEnabled)
                ViewController::get()->reloadGameListView(
                    mAutoCollectionSystemsData["favorites"].system);
        }
        if (adding) {
            s = new GuiInfoPopup(
                mWindow,
                "ADDED '" + Utils::String::toUpper(Utils::String::removeParenthesis(name)) +
                    "' TO '" + Utils::String::toUpper(sysName) + "'",
                4000);
        }
        else {
            s = new GuiInfoPopup(
                mWindow,
                "REMOVED '" + Utils::String::toUpper(Utils::String::removeParenthesis(name)) +
                    "' FROM '" + Utils::String::toUpper(sysName) + "'",
                4000);
        }
        mWindow->setInfoPopup(s);
        return true;
    }
    return false;
}

SystemData* CollectionSystemsManager::getSystemToView(SystemData* sys)
{
    SystemData* systemToView = sys;
    FileData* rootFolder = sys->getRootFolder();

    FileData* bundleRootFolder = mCustomCollectionsBundle->getRootFolder();
    const std::unordered_map<std::string, FileData*>& bundleChildren =
        bundleRootFolder->getChildrenByFilename();

    // Is the rootFolder bundled in the "My Collections" system?
    bool sysFoundInBundle = bundleChildren.find(rootFolder->getKey()) != bundleChildren.cend();

    if (sysFoundInBundle && sys->isCollection())
        systemToView = mCustomCollectionsBundle;
    return systemToView;
}

FileData* CollectionSystemsManager::updateCollectionFolderMetadata(SystemData* sys)
{
    FileData* rootFolder = sys->getRootFolder();
    FileFilterIndex* idx = rootFolder->getSystem()->getIndex();
    std::string desc = "This collection is empty";
    std::vector<FileData*> gamesList;
    std::vector<FileData*> gamesListRandom;

    if (UIModeController::getInstance()->isUIModeKid()) {
        for (FileData* game : rootFolder->getChildrenListToDisplay()) {
            if (game->getKidgame())
                gamesList.push_back(game);
        }
    }
    else {
        gamesList = rootFolder->getChildrenListToDisplay();
    }

    unsigned int gameCount = static_cast<unsigned int>(gamesList.size());

    // If there is more than 1 game in the collection, then randomize the example game names.
    if (gameCount > 1) {
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine{randDev()};
        unsigned int target;

        for (unsigned int i = 0; i < 3; i++) {
            std::uniform_int_distribution<int> uniform_dist(0, gameCount - 1 - i);
            target = uniform_dist(engine);
            gamesListRandom.push_back(gamesList[target]);
            std::vector<FileData*>::iterator it = (gamesList.begin() + target);
            gamesList.erase(it);
            if (gamesList.size() == 0)
                break;
        }
        gamesList.clear();
        gamesList.insert(gamesList.end(), gamesListRandom.begin(), gamesListRandom.end());
        gamesListRandom.clear();
    }

    if (gameCount > 0) {
        if (Settings::getInstance()->getBool("CollectionShowSystemInfo")) {
            switch (gameCount) {
                case 1: {
                    desc = "This collection contains 1 game: '" +
                           gamesList[0]->metadata.get("name") + " [" +
                           gamesList[0]->getSourceFileData()->getSystem()->getName() + "]'";
                    break;
                }
                case 2: {
                    desc = "This collection contains 2 games: '" +
                           gamesList[0]->metadata.get("name") + " [" +
                           gamesList[0]->getSourceFileData()->getSystem()->getName() + "]' and '" +
                           gamesList[1]->metadata.get("name") + " [" +
                           gamesList[1]->getSourceFileData()->getSystem()->getName() + "]'";
                    break;
                }
                default: {
                    desc = "This collection contains " + std::to_string(gameCount) + " games: '" +
                           gamesList[0]->metadata.get("name") + " [" +
                           gamesList[0]->getSourceFileData()->getSystem()->getName() + "]', '" +
                           gamesList[1]->metadata.get("name") + " [" +
                           gamesList[1]->getSourceFileData()->getSystem()->getName() + "]' and '" +
                           gamesList[2]->metadata.get("name") + " [" +
                           gamesList[2]->getSourceFileData()->getSystem()->getName() + "]'";
                    desc += (gameCount == 3 ? "" : ", among others");
                    break;
                }
            }
        }
        else {
            switch (gameCount) {
                case 1: {
                    desc = "This collection contains 1 game: '" +
                           gamesList[0]->metadata.get("name") + " '";
                    break;
                }
                case 2: {
                    desc = "This collection contains 2 games: '" +
                           gamesList[0]->metadata.get("name") + "' and '" +
                           gamesList[1]->metadata.get("name") + "'";
                    break;
                }
                default: {
                    desc = "This collection contains " + std::to_string(gameCount) + " games: '" +
                           gamesList[0]->metadata.get("name") + "', '" +
                           gamesList[1]->metadata.get("name") + "' and '" +
                           gamesList[2]->metadata.get("name") + "'";
                    desc += (gameCount == 3 ? "" : ", among others");
                    break;
                }
            }
        }
    }

    if (idx->isFiltered())
        desc += "\n\n'" + rootFolder->getSystem()->getFullName() +
                "' is filtered so there may be more games available";

    rootFolder->metadata.set("desc", desc);

    // Return a pointer to the first game so that its
    // game media can be displayed in the gamelist.
    if (gamesList.size() > 0)
        return gamesList.front();
    else
        return nullptr;
}

std::vector<std::string> CollectionSystemsManager::getUnusedSystemsFromTheme()
{
    // Get used systems in es_systems.xml.
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

    for (auto sysIt = themeSys.cbegin(); sysIt != themeSys.cend();) {
        if (std::find(systemsInUse.cbegin(), systemsInUse.cend(), *sysIt) != systemsInUse.cend())
            sysIt = themeSys.erase(sysIt);
        else
            sysIt++;
    }
    return themeSys;
}

SystemData* CollectionSystemsManager::addNewCustomCollection(std::string name)
{
    CollectionSystemDecl decl = mCollectionSystemDeclsIndex[myCollectionsName];
    decl.themeFolder = name;
    decl.name = name;
    decl.longName = name;

    return createNewCollectionEntry(name, decl, true, true);
}

void CollectionSystemsManager::deleteCustomCollection(std::string collectionName)
{
    auto collectionEntry = mCustomCollectionSystemsData.find(collectionName);

    // The window deletion needs to be located here instead of in GuiCollectionSystemsOptions
    // (where the custom collection deletions are initiated), as there seems to be some random
    // issue with accessing mWindow via the lambda expression.
    while (mWindow->peekGui() && mWindow->peekGui() != ViewController::get())
        delete mWindow->peekGui();

    if (collectionEntry != mCustomCollectionSystemsData.end()) {
        CollectionSystemsManager::get()->loadEnabledListFromSettings();
        CollectionSystemsManager::get()->updateSystemsList();

        ViewController::get()->removeGameListView(collectionEntry->second.system);
        ViewController::get()->reloadAll();

        delete collectionEntry->second.system;
        mCustomCollectionSystemsData.erase(collectionName);

        // Remove the collection configuration file.
        std::string configFile = getCustomCollectionConfigPath(collectionName);
        Utils::FileSystem::removeFile(configFile);
        LOG(LogDebug) << "CollectionSystemsManager::deleteCustomCollection(): Deleted the "
                         "configuration file '"
                      << configFile << "'.";

        GuiInfoPopup* s = new GuiInfoPopup(
            mWindow, "DELETED COLLECTION '" + Utils::String::toUpper(collectionName) + "'", 5000);
        mWindow->setInfoPopup(s);
    }
    else {
        LOG(LogError) << "Attempted to delete custom collection '" + collectionName + "' " +
                             "which doesn't exist.";
    }
}

void CollectionSystemsManager::reactivateCustomCollectionEntry(FileData* game)
{
    std::string gamePath = Utils::FileSystem::getFileName(game->getFullPath());
    gamePath = "%ROMPATH%/" + game->getSystemName() + "/" + gamePath;

    // Try to read from all custom collection configuration files to see if there are any
    // matching entries for the game passed as the parameter. If so, then enable it in each
    // of those collections. This is done also for disabled collections, as otherwise the
    // game would be missing if the collection was enabled during the program session.
    for (std::map<std::string, CollectionSystemData, stringComparator>::const_iterator it =
             mCustomCollectionSystemsData.cbegin();
         it != mCustomCollectionSystemsData.cend(); it++) {
        std::string path = getCustomCollectionConfigPath(it->first);
        if (Utils::FileSystem::exists(path)) {
#if defined(_WIN64)
            std::ifstream input(Utils::String::stringToWideString(path).c_str());
#else
            std::ifstream input(path);
#endif
            for (std::string gameKey; getline(input, gameKey);) {
                if (gameKey == gamePath) {
                    setEditMode(it->first, false);
                    toggleGameInCollection(game);
                    exitEditMode(false);
                }
            }
            if (input.is_open())
                input.close();
        }
    }
}

void CollectionSystemsManager::repopulateCollection(SystemData* sysData)
{
    for (auto it = mAutoCollectionSystemsData.cbegin(); // Line break.
         it != mAutoCollectionSystemsData.cend(); it++) {
        if ((*it).second.system == sysData) {
            LOG(LogDebug) << "CollectionSystemsManager::repopulateCollection(): "
                             "Repopulating auto collection \""
                          << it->first << "\"";

            CollectionSystemData* autoSystem = &mAutoCollectionSystemsData[it->first];
            std::vector<FileData*> systemEntries =
                autoSystem->system->getRootFolder()->getFilesRecursive(true, true, false);

            // Flag the collection as not populated so it gets repopulated.
            autoSystem->isPopulated = false;
            populateAutoCollection(autoSystem);

            if (systemEntries.empty())
                return;

            // Delete all children from the system.
            for (FileData* entry : systemEntries) {
                autoSystem->system->getRootFolder()->removeChild(entry);
                delete entry;
            }

            // Reset the filters so that they get rebuilt correctly when populating the collection.
            autoSystem->system->getIndex()->resetIndex();

            autoSystem->isPopulated = false;
            populateAutoCollection(autoSystem);

            // The cursor value is now pointing to some random memory address so we need to set
            // it to something valid. For empty collections we need to first create a placeholder
            // and then point to this, and for collections with games in them we select the first
            // entry.
            auto autoView = ViewController::get()->getGameListView(autoSystem->system).get();
            if (autoSystem->system->getRootFolder()->getChildren().size() == 0) {
                autoView->addPlaceholder(autoSystem->system->getRootFolder());
                autoView->setCursor(autoView->getLastEntry());
            }
            else {
                autoView->setCursor(
                    autoSystem->system->getRootFolder()->getChildrenRecursive().front());
                autoView->setCursor(autoView->getFirstEntry());
            }
        }
    }

    for (auto it = mCustomCollectionSystemsData.cbegin(); // Line break.
         it != mCustomCollectionSystemsData.cend(); it++) {
        if ((*it).second.system == sysData) {
            LOG(LogDebug) << "CollectionSystemsManager::repopulateCollection(): "
                             "Repopulating custom collection '"
                          << it->first << "'.";

            CollectionSystemData* customSystem = &mCustomCollectionSystemsData[it->first];
            std::vector<FileData*> systemEntries =
                customSystem->system->getRootFolder()->getFilesRecursive(true, true, false);

            if (systemEntries.empty())
                return;

            for (FileData* entry : systemEntries) {
                customSystem->system->getIndex()->removeFromIndex(entry);
                customSystem->system->getRootFolder()->removeChild(entry);
                delete entry;
            }

            customSystem->isPopulated = false;
            populateCustomCollection(customSystem);

            auto autoView = ViewController::get()->getGameListView(customSystem->system).get();
            autoView->setCursor(
                customSystem->system->getRootFolder()->getChildrenRecursive().front());
            autoView->setCursor(autoView->getFirstEntry());
        }
    }
}

void CollectionSystemsManager::initAutoCollectionSystems()
{
    for (std::map<std::string, CollectionSystemDecl, stringComparator>::const_iterator it =
             mCollectionSystemDeclsIndex.cbegin();
         it != mCollectionSystemDeclsIndex.cend(); it++) {
        CollectionSystemDecl sysDecl = it->second;

        if (!sysDecl.isCustom)
            createNewCollectionEntry(sysDecl.name, sysDecl);
    }
}

void CollectionSystemsManager::initCustomCollectionSystems()
{
    std::vector<std::string> systems = getCollectionsFromConfigFolder();
    for (auto nameIt = systems.cbegin(); nameIt != systems.cend(); nameIt++) {
        addNewCustomCollection(*nameIt);
    }
}

SystemData* CollectionSystemsManager::getAllGamesCollection()
{
    CollectionSystemData* allSysData = &mAutoCollectionSystemsData["all"];
    if (!allSysData->isPopulated)
        populateAutoCollection(allSysData);

    return allSysData->system;
}

SystemData* CollectionSystemsManager::createNewCollectionEntry(std::string name,
                                                               CollectionSystemDecl sysDecl,
                                                               bool index,
                                                               bool custom)
{
    SystemData* newSys = new SystemData(name, sysDecl.longName, mCollectionEnvData,
                                        sysDecl.themeFolder, true, custom);

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

void CollectionSystemsManager::populateAutoCollection(CollectionSystemData* sysData)
{
    SystemData* newSys = sysData->system;
    CollectionSystemDecl sysDecl = sysData->decl;
    FileData* rootFolder = newSys->getRootFolder();
    FileFilterIndex* index = newSys->getIndex();
    for (auto sysIt = SystemData::sSystemVector.cbegin(); // Line break.
         sysIt != SystemData::sSystemVector.cend(); sysIt++) {
        // We won't iterate all collections.
        if ((*sysIt)->isGameSystem() && !(*sysIt)->isCollection()) {
            std::vector<FileData*> files = (*sysIt)->getRootFolder()->getFilesRecursive(GAME);
            for (auto gameIt = files.cbegin(); gameIt != files.cend(); gameIt++) {
                bool include = includeFileInAutoCollections((*gameIt));

                switch (sysDecl.type) {
                    case AUTO_LAST_PLAYED: {
                        include = include && (*gameIt)->metadata.get("playcount") > "0";
                        break;
                    }
                    case AUTO_FAVORITES: {
                        // We may still want to add files we don't want in auto collections
                        // to "favorites".
                        include = (*gameIt)->metadata.get("favorite") == "true";
                        break;
                    }
                    default: {
                        break;
                    }
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
        rootFolder->sort(rootFolder->getSortTypeFromString("last played, ascending"));
    else
        rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                         Settings::getInstance()->getBool("FavoritesFirst"));

    if (sysDecl.type == AUTO_LAST_PLAYED)
        trimCollectionCount(rootFolder, LAST_PLAYED_MAX);

    // For the 'recent' collection we need to populate the gamelist once more as the
    // collection was trimmed down to 50 items. If we don't do this, the game count will
    // not be correct as it would include all the games prior to trimming.
    if (rootFolder->getName() == "recent" && !rootFolder->getChildrenRecursive().empty()) {
        // The following is needed to avoid a crash when repopulating the system as the previous
        // cursor pointer may point to a random memory address.
        auto recentGamelist = ViewController::get()->getGameListView(rootFolder->getSystem()).get();
        recentGamelist->setCursor(
            rootFolder->getSystem()->getRootFolder()->getChildrenRecursive().front());
        recentGamelist->setCursor(recentGamelist->getFirstEntry());
        if (rootFolder->getChildren().size() > 0)
            ViewController::get()
                ->getGameListView(rootFolder->getSystem())
                .get()
                ->onFileChanged(rootFolder->getChildren().front(), false);
    }

    sysData->isPopulated = true;
}

void CollectionSystemsManager::populateCustomCollection(CollectionSystemData* sysData)
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

#if defined(_WIN64)
    std::ifstream input(Utils::String::stringToWideString(path).c_str());
#else
    std::ifstream input(path);
#endif

    // Get all files map.
    std::unordered_map<std::string, FileData*> allFilesMap =
        getAllGamesCollection()->getRootFolder()->getChildrenByFilename();

    // Get the ROM directory, either as configured in es_settings.xml, or if no value
    // is set there, then use the default hardcoded path.
    const std::string rompath = FileData::getROMDirectory();

    // Iterate list of files in the config file.
    for (std::string gameKey; getline(input, gameKey);) {
        // If there is a %ROMPATH% variable set for the game, expand it. By doing this
        // it's possible to use either absolute ROM paths in the collection files or using
        // the path variable. The absolute ROM paths are only used for backward compatibility
        // with old custom collections. All custom collections saved by EmulationStation-DE
        // will use the %ROMPATH% variable instead.
        gameKey = Utils::String::replace(gameKey, "%ROMPATH%", rompath);
        gameKey = Utils::String::replace(gameKey, "//", "/");

        std::unordered_map<std::string, FileData*>::const_iterator it = allFilesMap.find(gameKey);
        if (it != allFilesMap.cend()) {
            CollectionFileData* newGame = new CollectionFileData(it->second, newSys);
            rootFolder->addChild(newGame);
            index->addToIndex(newGame);
        }
        else {
            LOG(LogWarning)
                << "File \"" << gameKey
                << "\" does not exist, is hidden, or is not counted as a game, ignoring entry";
        }
    }

    if (input.is_open())
        input.close();
}

void CollectionSystemsManager::removeCollectionsFromDisplayedSystems()
{
    // Remove all collection Systems.
    for (auto sysIt = SystemData::sSystemVector.cbegin();
         sysIt != SystemData::sSystemVector.cend();) {
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

void CollectionSystemsManager::addEnabledCollectionsToDisplayedSystems(
    std::map<std::string, CollectionSystemData, stringComparator>* colSystemData)
{
    // Add auto enabled collections.
    for (std::map<std::string, CollectionSystemData, stringComparator>::iterator it =
             colSystemData->begin();
         it != colSystemData->end(); it++) {
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
                    rootFolder->sort(
                        rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                        Settings::getInstance()->getBool("FavFirstCustom"));
                    // Jump to the first row of the game list, assuming it's not empty.
                    IGameListView* gameList =
                        ViewController::get()->getGameListView((it->second.system)).get();
                    if (!gameList->getCursor()->isPlaceHolder()) {
                        gameList->setCursor(gameList->getFirstEntry());
                    }
                    it->second.system->setIsGroupedCustomCollection(false);
                }
            }
            else {
                FileData* newSysRootFolder = it->second.system->getRootFolder();
                mCustomCollectionsBundle->getRootFolder()->addChild(newSysRootFolder);
                mCustomCollectionsBundle->getIndex()->importIndex(it->second.system->getIndex());
                it->second.system->setIsGroupedCustomCollection(true);
            }
        }
    }
}

std::vector<std::string> CollectionSystemsManager::getSystemsFromConfig()
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

    for (pugi::xml_node system = systemList.child("system"); system;
         system = system.next_sibling("system")) {
        // Theme folder.
        std::string themeFolder = system.child("theme").text().get();
        systems.push_back(themeFolder);
    }
    std::sort(systems.begin(), systems.end());
    return systems;
}

// Get all folders from the current theme path.
std::vector<std::string> CollectionSystemsManager::getSystemsFromTheme()
{
    std::vector<std::string> systems;

    auto themeSets = ThemeData::getThemeSets();
    if (themeSets.empty())
        return systems; // No theme sets available.

    std::map<std::string, ThemeSet>::const_iterator set =
        themeSets.find(Settings::getInstance()->getString("ThemeSet"));
    if (set == themeSets.cend()) {
        // Currently selected theme set is missing, so just pick the first available set.
        set = themeSets.cbegin();
        Settings::getInstance()->setString("ThemeSet", set->first);
    }

    std::string themePath = set->second.path;

    if (Utils::FileSystem::exists(themePath)) {
        Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(themePath);

        for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin();
             it != dirContent.cend(); it++) {
            if (Utils::FileSystem::isDirectory(*it)) {
                // ... here you have a directory.
                std::string folder = *it;
                folder = folder.substr(themePath.size() + 1);

                if (Utils::FileSystem::exists(set->second.getThemePath(folder)))
                    systems.push_back(folder);
            }
        }
    }
    std::sort(systems.begin(), systems.end());
    return systems;
}

std::vector<std::string> CollectionSystemsManager::getCollectionsFromConfigFolder()
{
    std::vector<std::string> systems;
    std::string configPath = getCollectionsFolder();

    if (Utils::FileSystem::exists(configPath)) {
        Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(configPath);
        for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin();
             it != dirContent.cend(); it++) {
            if (Utils::FileSystem::isRegularFile(*it)) {
                // It's a file.
                std::string filename = Utils::FileSystem::getFileName(*it);
                // Need to confirm filename matches config format.
                if (filename != "custom-.cfg" && Utils::String::startsWith(filename, "custom-") &&
                    Utils::String::endsWith(filename, ".cfg")) {
                    filename = filename.substr(7, filename.size() - 11);
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

std::vector<std::string> CollectionSystemsManager::getCollectionThemeFolders(bool custom)
{
    std::vector<std::string> systems;
    for (std::map<std::string, CollectionSystemDecl, stringComparator>::const_iterator it =
             mCollectionSystemDeclsIndex.cbegin();
         it != mCollectionSystemDeclsIndex.cend(); it++) {
        CollectionSystemDecl sysDecl = it->second;
        if (sysDecl.isCustom == custom)
            systems.push_back(sysDecl.themeFolder);
    }
    return systems;
}

std::vector<std::string> CollectionSystemsManager::getUserCollectionThemeFolders()
{
    std::vector<std::string> systems;
    for (std::map<std::string, CollectionSystemData, stringComparator>::const_iterator it =
             mCustomCollectionSystemsData.cbegin();
         it != mCustomCollectionSystemsData.cend(); it++)
        systems.push_back(it->second.decl.themeFolder);
    return systems;
}

void CollectionSystemsManager::trimCollectionCount(FileData* rootFolder, int limit)
{
    SystemData* curSys = rootFolder->getSystem();
    while (static_cast<int>(rootFolder->getChildrenListToDisplay().size()) > limit) {
        CollectionFileData* gameToRemove =
            (CollectionFileData*)rootFolder->getChildrenListToDisplay().back();
        ViewController::get()->getGameListView(curSys).get()->remove(gameToRemove, false);
    }
}

bool CollectionSystemsManager::themeFolderExists(std::string folder)
{
    std::vector<std::string> themeSys = getSystemsFromTheme();
    return std::find(themeSys.cbegin(), themeSys.cend(), folder) != themeSys.cend();
}

bool CollectionSystemsManager::includeFileInAutoCollections(FileData* file)
{
    // We exclude non-game files from collections (i.e. "kodi", entries from non-game systems).
    // If/when there are more in the future, maybe this can be a more complex method, with a
    // proper list, but for now a simple string comparison is more performant.
    return file->getName() != "kodi" && file->getSystem()->isGameSystem();
}

std::string CollectionSystemsManager::getCustomCollectionConfigPath(std::string collectionName)
{
    return getCollectionsFolder() + "/custom-" + collectionName + ".cfg";
}

std::string CollectionSystemsManager::getCollectionsFolder()
{
    return Utils::FileSystem::getGenericPath(Utils::FileSystem::getHomePath() +
                                             "/.emulationstation/collections");
}

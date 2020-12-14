//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemData.cpp
//
//  Provides data structures for the game systems and populates and indexes them based
//  on the configuration in es_systems.cfg as well as the presence of game ROM files.
//  Also provides functions to read and write to the gamelist files and to handle theme
//  loading.
//

#include "SystemData.h"

#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/gamelist/IGameListView.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Gamelist.h"
#include "Log.h"
#include "Platform.h"
#include "Settings.h"
#include "ThemeData.h"

#include <fstream>
#include <pugixml.hpp>
#include <random>

std::vector<SystemData*> SystemData::sSystemVector;

SystemData::SystemData(
        const std::string& name,
        const std::string& fullName,
        SystemEnvironmentData* envData,
        const std::string& themeFolder,
        bool CollectionSystem,
        bool CustomCollectionSystem)
        : mName(name),
        mFullName(fullName),
        mEnvData(envData),
        mThemeFolder(themeFolder),
        mIsCollectionSystem(CollectionSystem),
        mIsCustomCollectionSystem(CustomCollectionSystem),
        mIsGroupedCustomCollectionSystem(false),
        mIsGameSystem(true),
        mScrapeFlag(false)
{
    mFilterIndex = new FileFilterIndex();

    // If it's an actual system, initialize it, if not, just create the data structure.
    if (!CollectionSystem) {
        mRootFolder = new FileData(FOLDER, mEnvData->mStartPath, mEnvData, this);
        mRootFolder->metadata.set("name", mFullName);

        if (!Settings::getInstance()->getBool("ParseGamelistOnly")) {
            // If there was an error populating the folder or if there were no games found,
            // then don't continue with any additional process steps for this system.
            if (!populateFolder(mRootFolder))
                return;
        }

        if (!Settings::getInstance()->getBool("IgnoreGamelist"))
            parseGamelist(this);

        setupSystemSortType(mRootFolder);

        mRootFolder->sort(mRootFolder->getSortTypeFromString(mRootFolder->getSortTypeString()),
                Settings::getInstance()->getBool("FavoritesFirst"));

        indexAllGameFilters(mRootFolder);
    }
    else {
        // Virtual systems are updated afterwards by CollectionSystemManager.
        // We're just creating the data structure here.
        mRootFolder = new FileData(FOLDER, "" + name, mEnvData, this);
        setupSystemSortType(mRootFolder);
    }
    setIsGameSystemStatus();
    loadTheme();
}

SystemData::~SystemData()
{
    if (Settings::getInstance()->getString("SaveGamelistsMode") == "on exit") {
        if (mRootFolder->getGameCount().first + mRootFolder->getGameCount().second != 0)
            writeMetaData();
    }

    delete mRootFolder;
    delete mFilterIndex;
}

void SystemData::setIsGameSystemStatus()
{
    // We exclude non-game systems from specific operations (i.e. the "RetroPie" system, at least).
    // If/when there are more in the future, maybe this can be a more complex method, with a proper
    // list but for now a simple string comparison is more performant.
    mIsGameSystem = (mName != "retropie");
}

bool SystemData::populateFolder(FileData* folder)
{
    const std::string& folderPath = folder->getPath();
    if (!Utils::FileSystem::exists(folderPath)) {
    LOG(LogDebug) << "SystemData::populateFolder(): Folder with path \"" <<
                folderPath << "\" does not exist";
        return false;
    }
    if (!Utils::FileSystem::isDirectory(folderPath)) {
        LOG(LogWarning) << "Folder with path \"" <<
                folderPath << "\" is not a directory";
        return false;
    }

    // Make sure that this isn't a symlink to an object we already have.
    if (Utils::FileSystem::isSymlink(folderPath)) {
        // If this symlink resolves to somewhere that's at the beginning of our
        // path, it's going to create a recursive loop. Make sure to avoid this.
        if (folderPath.find(Utils::FileSystem::getCanonicalPath(folderPath)) == 0) {
            LOG(LogWarning) << "Skipping infinitely recursive symlink \"" <<
                    folderPath << "\"";
            return false;
        }
    }

    std::string filePath;
    std::string extension;
    bool isGame;
    bool showHiddenFiles = Settings::getInstance()->getBool("ShowHiddenFiles");
    Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(folderPath);

    // If system directory exists but contains no games, return as error.
    if (dirContent.size() == 0)
        return false;

    for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin();
            it != dirContent.cend(); ++it) {
        filePath = *it;

        // Skip hidden files and folders.
        if (!showHiddenFiles && Utils::FileSystem::isHidden(filePath)) {
            LOG(LogDebug) << "SystemData::populateFolder(): Skipping hidden " <<
                    (Utils::FileSystem::isDirectory(filePath) ? "directory \"" : "file \"") <<
                    filePath << "\"";
            continue;
        }

        // This is a little complicated because we allow a list
        // of extensions to be defined (delimited with a space).
        // We first get the extension of the file itself:
        extension = Utils::FileSystem::getExtension(filePath);

        // FYI, folders *can* also match the extension and be added as games.
        // This is mostly just to support higan.
        // See issue #75: https://github.com/Aloshi/EmulationStation/issues/75

        isGame = false;
        if (std::find(mEnvData->mSearchExtensions.cbegin(), mEnvData->mSearchExtensions.cend(),
                extension) != mEnvData->mSearchExtensions.cend()) {
            FileData* newGame = new FileData(GAME, filePath, mEnvData, this);

            // Prevent new arcade assets from being added.
            if (!newGame->isArcadeAsset()) {
                folder->addChild(newGame);
                isGame = true;
            }
        }

        // Add directories that also do not match an extension as folders.
        if (!isGame && Utils::FileSystem::isDirectory(filePath)) {
            FileData* newFolder = new FileData(FOLDER, filePath, mEnvData, this);
            populateFolder(newFolder);

            // Ignore folders that do not contain games.
            if (newFolder->getChildrenByFilename().size() == 0)
                delete newFolder;
            else
                folder->addChild(newFolder);
        }
    }
    return true;
}

void SystemData::indexAllGameFilters(const FileData* folder)
{
    const std::vector<FileData*>& children = folder->getChildren();

    for (std::vector<FileData*>::const_iterator it = children.cbegin();
            it != children.cend(); ++it) {
        switch ((*it)->getType()) {
            case GAME: {
                mFilterIndex->addToIndex(*it);
            }
            break;
            case FOLDER: {
                indexAllGameFilters(*it);
            }
            break;
            default:
                break;
        }
    }
}

std::vector<std::string> readList(const std::string& str, const char* delims = " \t\r\n,")
{
    std::vector<std::string> ret;

    size_t prevOff = str.find_first_not_of(delims, 0);
    size_t off = str.find_first_of(delims, prevOff);
    while (off != std::string::npos || prevOff != std::string::npos) {
        ret.push_back(str.substr(prevOff, off - prevOff));

        prevOff = str.find_first_not_of(delims, off);
        off = str.find_first_of(delims, prevOff);
    }

    return ret;
}

// Creates systems from information located in a config file.
bool SystemData::loadConfig()
{
    deleteSystems();

    std::string path = getConfigPath(false);
    const std::string rompath = FileData::getROMDirectory();

    if (!Utils::FileSystem::exists(path)) {
        LOG(LogWarning) << "Systems configuration file does not exist";
        if (copyConfigTemplate(getConfigPath(true)))
            return false;
        path = getConfigPath(false);
    }

    LOG(LogInfo) << "Parsing systems configuration file \"" << path << "\"...";

    pugi::xml_document doc;
    #if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
    #else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
    #endif

    if (!res) {
        LOG(LogError) << "Could not parse es_systems.cfg";
        LOG(LogError) << res.description();
        return false;
    }

    // Actually read the file.
    pugi::xml_node systemList = doc.child("systemList");

    if (!systemList) {
        LOG(LogError) << "es_systems.cfg is missing the <systemList> tag";
        return false;
    }

    for (pugi::xml_node system = systemList.child("system"); system;
            system = system.next_sibling("system")) {
        std::string name;
        std::string fullname;
        std::string path;
        std::string cmd;
        std::string themeFolder;

        name = system.child("name").text().get();
        fullname = system.child("fullname").text().get();
        path = system.child("path").text().get();

        // If there is a %ROMPATH% variable set for the system, expand it. By doing this
        // it's possible to use either absolute ROM paths in es_systems.cfg or to utilize
        // the ROM path configured as ROMDirectory in es_settings.cfg. If it's set to ""
        // in this configuration file, the default hardcoded path $HOME/ROMs/ will be used.
        path = Utils::String::replace(path, "%ROMPATH%", rompath);
        path = Utils::String::replace(path, "//", "/");

        // Convert extensions list from a string into a vector of strings.
        std::vector<std::string> extensions = readList(system.child("extension").text().get());

        cmd = system.child("command").text().get();

        // Platform ID list
        const char* platformList = system.child("platform").text().get();
        std::vector<std::string> platformStrs = readList(platformList);
        std::vector<PlatformIds::PlatformId> platformIds;
        for (auto it = platformStrs.cbegin(); it != platformStrs.cend(); it++) {
            const char* str = it->c_str();
            PlatformIds::PlatformId platformId = PlatformIds::getPlatformId(str);

            if (platformId == PlatformIds::PLATFORM_IGNORE) {
                // When platform is PLATFORM_IGNORE, do not allow other platforms.
                platformIds.clear();
                platformIds.push_back(platformId);
                break;
            }

            // If there appears to be an actual platform ID supplied
            // but it didn't match the list, generate a warning.
            if (str != nullptr && str[0] != '\0' && platformId == PlatformIds::PLATFORM_UNKNOWN)
                LOG(LogWarning) << "Unknown platform for system \"" << name <<
                        "\" (platform \"" << str << "\" from list \"" << platformList << "\")";
            else if (platformId != PlatformIds::PLATFORM_UNKNOWN)
                platformIds.push_back(platformId);
        }

        // Theme folder.
        themeFolder = system.child("theme").text().as_string(name.c_str());

        // Validate.
        if (name.empty() || path.empty() || extensions.empty() || cmd.empty()) {
            LOG(LogError) << "System \"" << name <<
                    "\" is missing name, path, extension, or command";
            continue;
        }

        // Convert path to generic directory seperators.
        path = Utils::FileSystem::getGenericPath(path);

        #if defined(_WIN64)
        if (!Settings::getInstance()->getBool("ShowHiddenFiles") &&
                Utils::FileSystem::isHidden(path)) {
            LOG(LogWarning) << "Skipping hidden ROM folder " << path;
            continue;
        }
        #endif

        // Create the system runtime environment data.
        SystemEnvironmentData* envData = new SystemEnvironmentData;
        envData->mStartPath = path;
        envData->mSearchExtensions = extensions;
        envData->mLaunchCommand = cmd;
        envData->mPlatformIds = platformIds;

        SystemData* newSys = new SystemData(name, fullname, envData, themeFolder);
        bool onlyHidden = false;

        // If the option to show hidden games has been disabled, then check whether all
        // games for the system are hidden. That will flag the system as empty.
        if (!Settings::getInstance()->getBool("ShowHiddenGames")) {
            std::vector<FileData*> recursiveGames =
                    newSys->getRootFolder()->getChildrenRecursive();
            onlyHidden = true;
            for (auto it = recursiveGames.cbegin(); it != recursiveGames.cend(); it++) {
                if ((*it)->getType() != FOLDER) {
                    onlyHidden = (*it)->getHidden();
                    if (!onlyHidden)
                        break;
                }
            }
        }

        if (newSys->getRootFolder()->getChildrenByFilename().size() == 0 || onlyHidden) {
            LOG(LogDebug) << "SystemData::loadConfig(): System \"" << name <<
                    "\" has no games, ignoring it";
            delete newSys;
            delete envData;
        }
        else {
            sSystemVector.push_back(newSys);
        }
    }

    // Sort systems by their full names.
    std::sort(std::begin(sSystemVector), std::end(sSystemVector),
            [](SystemData* a, SystemData* b) {
                return a->getFullName() < b->getFullName(); });

    // Don't load any collections if there are no systems available.
    if (sSystemVector.size() > 0)
        CollectionSystemManager::get()->loadCollectionSystems();

    return true;
}

bool SystemData::copyConfigTemplate(const std::string& path)
{
    std::string systemsTemplateFile;;

    LOG(LogInfo) <<
            "Attempting to copy template es_systems.cfg file from the resources directory...";

    #if defined(_WIN64)
    systemsTemplateFile = ResourceManager::getInstance()->
            getResourcePath(":/templates/es_systems.cfg_windows", false);
    #elif defined(__APPLE__)
    systemsTemplateFile = ResourceManager::getInstance()->
            getResourcePath(":/templates/es_systems.cfg_macos", false);
    #elif defined(__FreeBSD__)
    systemsTemplateFile = ResourceManager::getInstance()->
            getResourcePath(":/templates/es_systems.cfg_freebsd", false);
    #elif defined(__NetBSD__)
    systemsTemplateFile = ResourceManager::getInstance()->
            getResourcePath(":/templates/es_systems.cfg_netbsd", false);
    #elif defined(__OpenBSD__)
    systemsTemplateFile = ResourceManager::getInstance()->
            getResourcePath(":/templates/es_systems.cfg_openbsd", false);
    #else
    // Assume that anything else is some type of Linux system.
    systemsTemplateFile = ResourceManager::getInstance()->
            getResourcePath(":/templates/es_systems.cfg_linux", false);
    #endif

    if (systemsTemplateFile == "") {
        LOG(LogError) << "Can't find the es_systems.cfg template file";
        return true;
    }
    else if (Utils::FileSystem::copyFile(systemsTemplateFile, path, false)) {
        LOG(LogError) << "Copying of es_systems.cfg template file failed";
        return true;
    }

    LOG(LogInfo) << "Template es_systems.cfg file copied successfully";

    return false;
}

void SystemData::deleteSystems()
{
    for (unsigned int i = 0; i < sSystemVector.size(); i++)
        delete sSystemVector.at(i);

    sSystemVector.clear();
}

std::string SystemData::getConfigPath(bool forWrite)
{
    std::string path = Utils::FileSystem::getHomePath() + "/.emulationstation/es_systems.cfg";
    if (forWrite || Utils::FileSystem::exists(path))
        return path;

    return "";
}

bool SystemData::isVisible()
{
    // This function doesn't make much sense at the moment; if a game system does not have any
    // games available, it will not be processed during startup and will as such not exist.
    // In the future this function may be used for an option to hide specific systems, but
    // for the time being all systems will always be visible.
    return true;
}

SystemData* SystemData::getNext() const
{
    std::vector<SystemData*>::const_iterator it = getIterator();

    // As we are starting in a valid gamelistview, this will
    // always succeed, even if we have to come full circle.
    do {
        it++;
        if (it == sSystemVector.cend())
            it = sSystemVector.cbegin();
    } while (!(*it)->isVisible());

    return *it;
}

SystemData* SystemData::getPrev() const
{
    std::vector<SystemData*>::const_reverse_iterator it = getRevIterator();

    // As we are starting in a valid gamelistview, this will
    // always succeed, even if we have to come full circle.
    do {
        it++;
        if (it == sSystemVector.crend())
            it = sSystemVector.crbegin();
    } while (!(*it)->isVisible());

    return *it;
}

std::string SystemData::getGamelistPath(bool forWrite) const
{
    std::string filePath;

    filePath = mRootFolder->getPath() + "/gamelist.xml";
    if (Utils::FileSystem::exists(filePath))
        return filePath;

    filePath = Utils::FileSystem::getHomePath() + "/.emulationstation/gamelists/" +
            mName + "/gamelist.xml";

    // Make sure the directory exists if we're going to write to it,
    // or crashes will happen.
    if (forWrite)
        Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(filePath));
    if (forWrite || Utils::FileSystem::exists(filePath))
        return filePath;

    return "";
}

std::string SystemData::getThemePath() const
{
    // Locations where we check for themes, in the following order:
    // 1. [SYSTEM_PATH]/theme.xml
    // 2. System theme from currently selected theme set [CURRENT_THEME_PATH]/[SYSTEM]/theme.xml
    // 3. Default system theme from currently selected theme set [CURRENT_THEME_PATH]/theme.xml

    // First, check game folder.
    std::string localThemePath = mRootFolder->getPath() + "/theme.xml";
    if (Utils::FileSystem::exists(localThemePath))
        return localThemePath;

    // Not in game folder, try system theme in theme sets.
    localThemePath = ThemeData::getThemeFromCurrentSet(mThemeFolder);

    if (Utils::FileSystem::exists(localThemePath))
        return localThemePath;

    // Not system theme, try default system theme in theme set.
    localThemePath = Utils::FileSystem::getParent(Utils::FileSystem::getParent(localThemePath)) +
            "/theme.xml";

    return localThemePath;
}

bool SystemData::hasGamelist() const
{
    return (Utils::FileSystem::exists(getGamelistPath(false)));
}

SystemData* SystemData::getRandomSystem(const SystemData* currentSystem)
{
    unsigned int total = 0;
    for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++) {
        if ((*it)->isGameSystem())
            total++;
    }

    if (total < 2)
        return nullptr;

    SystemData* randomSystem = nullptr;

    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine{randDev()};
        std::uniform_int_distribution<int> uniform_dist(0, total - 1);
        int target = uniform_dist(engine);

        for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++) {
            if ((*it)->isGameSystem()) {
                if (target > 0) {
                    target--;
                }
                else {
                    randomSystem = (*it);
                    break;
                }
            }
        }
    }
    while (randomSystem == currentSystem);

    return randomSystem;
}

FileData* SystemData::getRandomGame(const FileData* currentGame)
{
    std::vector<FileData*> gameList;
    bool onlyFolders = false;
    bool hasFolders = false;

    // If we're in the custom collection group list, then get the list of collections,
    // otherwise get a list of all the folder and file entries in the view.
    if (currentGame && currentGame->getType() == FOLDER && currentGame->
            getSystem()->isGroupedCustomCollection()) {
        gameList = ViewController::get()->getGameListView(mRootFolder->getSystem()).get()->
                getCursor()->getParent()->getParent()->getChildrenListToDisplay();
    }
    else {
        gameList = ViewController::get()->getGameListView(mRootFolder->
                getSystem()).get()->getCursor()->getParent()->getChildrenListToDisplay();
    }

    if (gameList.size() > 0 && gameList.front()->getParent()->getOnlyFoldersFlag())
        onlyFolders = true;

    if (gameList.size() > 0 && gameList.front()->getParent()->getHasFoldersFlag())
        hasFolders = true;

    // If this is a mixed view of folders and files, then remove all the folder entries
    // as we want to exclude them from the random selection.
    if (!onlyFolders && hasFolders) {
        unsigned int i = 0;
        do {
            if (gameList[i]->getType() == FOLDER)
                gameList.erase(gameList.begin() + i);
            else
                i++;
        } while (i < gameList.size());
    }

    if (!currentGame && gameList.size() == 1)
        return gameList.front();

    // If there is only one folder and one file in the list, then return the file.
    if (!onlyFolders && hasFolders && gameList.size() == 1)
        return gameList.front();

    if (currentGame && currentGame->getType() == PLACEHOLDER)
        return nullptr;

    unsigned int total = static_cast<int>(gameList.size());
    int target = 0;

    if (total < 2)
        return nullptr;

    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine{randDev()};
        std::uniform_int_distribution<int> uniform_dist(0, total - 1);
        target = uniform_dist(engine);
    }
    while (currentGame && gameList.at(target) == currentGame);

    return gameList.at(target);
}

void SystemData::sortSystem(bool reloadGamelist, bool jumpToFirstRow)
{
    if (getName() == "recent")
        return;

    bool favoritesSorting;

    if (this->isCustomCollection() ||
            (this->isCollection() && this->getFullName() == "collections"))
        favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
    else
        favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

    FileData* rootFolder = getRootFolder();
    // Assign the sort type to all grouped custom collections.
    if (mIsCollectionSystem && mFullName == "collections") {
        for (auto it = rootFolder->getChildren().begin();
                it != rootFolder->getChildren().end(); it++) {
            setupSystemSortType((*it)->getSystem()->getRootFolder());
        }
    }
    setupSystemSortType(rootFolder);

    rootFolder->sort(rootFolder->getSortTypeFromString(
            rootFolder->getSortTypeString()), favoritesSorting);

    if (reloadGamelist)
        ViewController::get()->reloadGameListView(this, false);

    if (jumpToFirstRow) {
        IGameListView* gameList = ViewController::get()->getGameListView(this).get();
        gameList->setCursor(gameList->getFirstEntry());
    }
}

std::pair<unsigned int, unsigned int> SystemData::getDisplayedGameCount() const
{
    // Return all games for the system which are marked as 'countasgame'. As this flag is set
    // by default, normally most games will be included in the number returned from here.
    // The actual game counting takes place in FileData during sorting.
    return mRootFolder->getGameCount();
}

void SystemData::loadTheme()
{
    mTheme = std::make_shared<ThemeData>();

    std::string path = getThemePath();

    if (!Utils::FileSystem::exists(path)) // No theme available for this platform.
        return;

    try {
        // Build map with system variables for theme to use.
        std::map<std::string, std::string> sysData;
        sysData.insert(std::pair<std::string, std::string>("system.name", getName()));
        sysData.insert(std::pair<std::string, std::string>("system.theme", getThemeFolder()));
        sysData.insert(std::pair<std::string, std::string>("system.fullName", getFullName()));

        mTheme->loadFile(sysData, path);
    }
    catch (ThemeException& e) {
        LOG(LogError) << e.what();
        mTheme = std::make_shared<ThemeData>(); // Reset to empty.
    }
}

void SystemData::writeMetaData() {
    if (Settings::getInstance()->getBool("IgnoreGamelist") || mIsCollectionSystem)
        return;

    // Save changed game data back to xml.
    updateGamelist(this);
}

void SystemData::onMetaDataSavePoint() {
    if (Settings::getInstance()->getString("SaveGamelistsMode") != "always")
        return;

    writeMetaData();
}

void SystemData::setupSystemSortType(FileData* mRootFolder)
{
    // If DefaultSortOrder is set to something, check that it is actually a valid value.
    if (Settings::getInstance()->getString("DefaultSortOrder") != "") {
        for (unsigned int i = 0; i < FileSorts::SortTypes.size(); i++) {
            if (FileSorts::SortTypes.at(i).description ==
                    Settings::getInstance()->getString("DefaultSortOrder")) {
                mRootFolder->setSortTypeString(Settings::getInstance()->
                        getString("DefaultSortOrder"));
                break;
            }
        }
    }
    // If no valid sort type was defined in the configuration file, set to default sorting.
    if (mRootFolder->getSortTypeString() == "")
        mRootFolder->setSortTypeString(Settings::getInstance()->
                getDefaultString("DefaultSortOrder"));
}

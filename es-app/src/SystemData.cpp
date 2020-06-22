//
//  SystemData.cpp
//
//  Provides data structures for the game systems and populates and indexes them based
//  on the configuration in es_systems.cfg as well as the presence of game ROM files.
//  Also provides functions to read and write to the gamelist files and to handle theme
//  loading.
//

#include "SystemData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Gamelist.h"
#include "Log.h"
#include "Platform.h"
#include "Settings.h"
#include "ThemeData.h"
#include "views/UIModeController.h"
#include <pugixml/src/pugixml.hpp>
#include <fstream>
#ifdef WIN32
#include <Windows.h>
#endif

std::vector<SystemData*> SystemData::sSystemVector;

SystemData::SystemData(
        const std::string& name,
        const std::string& fullName,
        SystemEnvironmentData* envData,
        const std::string& themeFolder,
        bool CollectionSystem)
        : mName(name),
        mFullName(fullName),
        mEnvData(envData),
        mThemeFolder(themeFolder),
        mIsCollectionSystem(CollectionSystem),
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

        mRootFolder->sort(getSortTypeFromString(mRootFolder->getSortTypeString()),
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
    if (Settings::getInstance()->getString("SaveGamelistsMode") == "on exit")
        writeMetaData();

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
    if (!Utils::FileSystem::isDirectory(folderPath)) {
        LOG(LogWarning) << "Error - folder with path \"" << folderPath << "\" is not a directory!";
        return false;
    }

    // Make sure that this isn't a symlink to an object we already have.
    if (Utils::FileSystem::isSymlink(folderPath)) {
        // If this symlink resolves to somewhere that's at the beginning of our
        // path, it's going to create a recursive loop. Make sure to avoid this.
        if (folderPath.find(Utils::FileSystem::getCanonicalPath(folderPath)) == 0) {
            LOG(LogWarning) << "Skipping infinitely recursive symlink \"" << folderPath << "\"";
            return false;
        }
    }

    std::string filePath;
    std::string extension;
    bool isGame;
    bool showHidden = Settings::getInstance()->getBool("ShowHiddenFiles");
    Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(folderPath);

    // If system directory exists but contains no games, return as error.
    if (dirContent.size() == 0)
        return false;

    for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin();
            it != dirContent.cend(); ++it) {
        filePath = *it;

        // Skip hidden files and folders.
        if (!showHidden && Utils::FileSystem::isHidden(filePath))
            continue;

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
    const std::string rompath  = FileData::getROMDirectory();

    LOG(LogInfo) << "Loading system config file " << path << "...";

    if (!Utils::FileSystem::exists(path)) {
        LOG(LogError) << "es_systems.cfg file does not exist!";
        writeExampleConfig(getConfigPath(true));
        return false;
    }

    pugi::xml_document doc;
    pugi::xml_parse_result res = doc.load_file(path.c_str());

    if (!res) {
        LOG(LogError) << "Could not parse es_systems.cfg file!";
        LOG(LogError) << res.description();
        return false;
    }

    // Actually read the file.
    pugi::xml_node systemList = doc.child("systemList");

    if (!systemList) {
        LOG(LogError) << "es_systems.cfg is missing the <systemList> tag!";
        return false;
    }

    for (pugi::xml_node system = systemList.child("system"); system;
            system = system.next_sibling("system")) {
        std::string name, fullname, path, cmd, themeFolder;

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
            if (str != NULL && str[0] != '\0' && platformId == PlatformIds::PLATFORM_UNKNOWN)
                LOG(LogWarning) << "  Unknown platform for system \"" << name << "\" (platform \""
                        << str << "\" from list \"" << platformList << "\")";
            else if (platformId != PlatformIds::PLATFORM_UNKNOWN)
                platformIds.push_back(platformId);
        }

        // Theme folder.
        themeFolder = system.child("theme").text().as_string(name.c_str());

        // Validate.
        if (name.empty() || path.empty() || extensions.empty() || cmd.empty()) {
            LOG(LogError) << "System \"" << name <<
                    "\" is missing name, path, extension, or command!";
            continue;
        }

        // Convert path to generic directory seperators.
        path = Utils::FileSystem::getGenericPath(path);

        // Expand home symbol if the startpath contains ~
        if (path[0] == '~')
        {
            path.erase(0, 1);
            path.insert(0, Utils::FileSystem::getHomePath());
        }

        // Create the system runtime environment data.
        SystemEnvironmentData* envData = new SystemEnvironmentData;
        envData->mStartPath = path;
        envData->mSearchExtensions = extensions;
        envData->mLaunchCommand = cmd;
        envData->mPlatformIds = platformIds;

        SystemData* newSys = new SystemData(name, fullname, envData, themeFolder);
        if (newSys->getRootFolder()->getChildrenByFilename().size() == 0) {
            LOG(LogWarning) << "System \"" << name << "\" has no games! Ignoring it.";
            delete newSys;
        }
        else {
            sSystemVector.push_back(newSys);
        }
    }
    // Don't load any collections if there are no systems available.
    if (sSystemVector.size() > 0)
        CollectionSystemManager::get()->loadCollectionSystems();

    return true;
}

void SystemData::writeExampleConfig(const std::string& path)
{
    std::ofstream file(path.c_str());

    file << "<!-- This is the EmulationStation Systems configuration file.\n"
            "All systems must be contained within the <systemList> tag.-->\n"
            "\n"
            "<systemList>\n"
            "	<!-- Here's an example system to get you started. -->\n"
            "	<system>\n"
            "\n"
            "		<!-- A short name, used internally. Traditionally lower-case. -->\n"
            "		<name>nes</name>\n"
            "\n"
            "		<!-- A \"pretty\" name, displayed in menus and such. -->\n"
            "		<fullname>Nintendo Entertainment System</fullname>\n"
            "\n"
            "		<!-- The path to start searching for ROMs in. '~' will be expanded to $HOME on Linux or %HOMEPATH% on Windows. -->\n"
            "		<path>~/roms/nes</path>\n"
            "\n"
            "		<!-- A list of extensions to search for, delimited by any of the whitespace characters (\", \\r\\n\\t\").\n"
            "		You MUST include the period at the start of the extension! It's also case sensitive. -->\n"
            "		<extension>.nes .NES</extension>\n"
            "\n"
            "		<!-- The shell command executed when a game is selected. A few special tags are replaced if found in a command:\n"
            "		%ROM% is replaced by a bash-special-character-escaped absolute path to the ROM.\n"
            "		%BASENAME% is replaced by the \"base\" name of the ROM.  For example, \"/foo/bar.rom\" would have a basename of \"bar\". Useful for MAME.\n"
            "		%ROM_RAW% is the raw, unescaped path to the ROM. -->\n"
            "		<command>retroarch -L ~/cores/libretro-fceumm.so %ROM%</command>\n"
            "\n"
            "		<!-- The platform to use when scraping. You can see the full list of accepted platforms in src/PlatformIds.cpp.\n"
            "		It's case sensitive, but everything is lowercase. This tag is optional.\n"
            "		You can use multiple platforms too, delimited with any of the whitespace characters (\", \\r\\n\\t\"), eg: \"genesis, megadrive\" -->\n"
            "		<platform>nes</platform>\n"
            "\n"
            "		<!-- The theme to load from the current theme set.  See THEMES.md for more information.\n"
            "		This tag is optional. If not set, it will default to the value of <name>. -->\n"
            "		<theme>nes</theme>\n"
            "	</system>\n"
            "</systemList>\n";

    file.close();

    LOG(LogError) << "Example config written!  Go read it at \"" << path << "\"!";
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
   return (getDisplayedGameCount() > 0 ||
           (UIModeController::getInstance()->isUIModeFull() && mIsCollectionSystem) ||
           (mIsCollectionSystem && mName == "favorites"));
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

unsigned int SystemData::getGameCount() const
{
    return (unsigned int)mRootFolder->getFilesRecursive(GAME).size();
}

SystemData* SystemData::getRandomSystem()
{
    // This is a bit brute force.
    // It might be more efficient to just do a while (!gameSystem) do random again...
    unsigned int total = 0;
    for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++) {
        if ((*it)->isGameSystem())
            total++;
    }

    // Get a random number in range.
    int target = (int)Math::round((std::rand() / (float)RAND_MAX) * (total - 1));
    for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); it++)
    {
        if ((*it)->isGameSystem()) {
            if (target > 0)
                target--;
            else
                return (*it);
        }
    }

    // If we end up here, there is no valid system.
    return NULL;
}

FileData* SystemData::getRandomGame()
{
    std::vector<FileData*> list = mRootFolder->getFilesRecursive(GAME, true);
    unsigned int total = (int)list.size();
    int target = 0;

    // Get a random number in range.
    if (total == 0)
        return NULL;
    target = (int)Math::round((std::rand() / (float)RAND_MAX) * (total - 1));

    return list.at(target);
}

unsigned int SystemData::getDisplayedGameCount() const
{
    return (unsigned int)mRootFolder->getFilesRecursive(GAME, true).size();
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
    // If no valid sort type was defined in the configuration
    // file, set sorting to "filename, ascending".
    if (mRootFolder->getSortTypeString() == "")
        mRootFolder->setSortTypeString(FileSorts::SortTypes.at(0).description);
}

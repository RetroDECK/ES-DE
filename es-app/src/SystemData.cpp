//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  SystemData.cpp
//
//  Provides data structures for the game systems and populates and indexes them based
//  on the configuration in es_systems.xml as well as the presence of game ROM files.
//  Also provides functions to read and write to the gamelist files and to handle theme
//  loading.
//

#include "SystemData.h"

#include "CollectionSystemsManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Gamelist.h"
#include "Log.h"
#include "Platform.h"
#include "Settings.h"
#include "ThemeData.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "views/gamelist/IGameListView.h"

#include <fstream>
#include <pugixml.hpp>
#include <random>

std::vector<SystemData*> SystemData::sSystemVector;
std::unique_ptr<FindRules> SystemData::sFindRules;

FindRules::FindRules()
{
    LOG(LogInfo) << "Loading emulator find rules...";
    loadFindRules();
}

void FindRules::loadFindRules()
{
    std::string customSystemsDirectory =
        Utils::FileSystem::getHomePath() + "/.emulationstation/custom_systems";

    std::string path = customSystemsDirectory + "/es_find_rules.xml";

    if (Utils::FileSystem::exists(path)) {
        LOG(LogInfo) << "Found custom find rules configuration file";
    }
    else {
#if defined(_WIN64)
        path = ResourceManager::getInstance()->getResourcePath(
            ":/systems/windows/es_find_rules.xml", false);
#elif defined(__APPLE__)
        path = ResourceManager::getInstance()->getResourcePath(":/systems/macos/es_find_rules.xml",
                                                               false);
#else
        path = ResourceManager::getInstance()->getResourcePath(":/systems/unix/es_find_rules.xml",
                                                               false);
#endif
    }

    if (path == "") {
        LOG(LogWarning) << "No find rules configuration file found";
        return;
    }

    LOG(LogInfo) << "Parsing find rules configuration file \"" << path << "\"...";

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
#else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
#endif

    if (!res) {
        LOG(LogError) << "Couldn't parse es_find_rules.xml: " << res.description();
        return;
    }

    // Actually read the file.
    pugi::xml_node ruleList = doc.child("ruleList");

    if (!ruleList) {
        LOG(LogError) << "es_find_rules.xml is missing the <ruleList> tag";
        return;
    }

    EmulatorRules emulatorRules;
    CoreRules coreRules;

    for (pugi::xml_node emulator = ruleList.child("emulator"); emulator;
         emulator = emulator.next_sibling("emulator")) {
        std::string emulatorName = emulator.attribute("name").as_string();
        if (emulatorName.empty()) {
            LOG(LogWarning) << "Found emulator tag without name attribute, skipping entry";
            continue;
        }
        if (mEmulators.find(emulatorName) != mEmulators.end()) {
            LOG(LogWarning) << "Found repeating emulator tag \"" << emulatorName
                            << "\", skipping entry";
            continue;
        }
        for (pugi::xml_node rule = emulator.child("rule"); rule; rule = rule.next_sibling("rule")) {
            std::string ruleType = rule.attribute("type").as_string();
            if (ruleType.empty()) {
                LOG(LogWarning) << "Found rule tag without type attribute for emulator \""
                                << emulatorName << "\", skipping entry";
                continue;
            }
#if defined(_WIN64)
            if (ruleType != "winregistrypath" && ruleType != "systempath" &&
                ruleType != "staticpath") {
#else
            if (ruleType != "systempath" && ruleType != "staticpath") {
#endif
                LOG(LogWarning) << "Found invalid rule type \"" << ruleType << "\" for emulator \""
                                << emulatorName << "\", skipping entry";
                continue;
            }
            for (pugi::xml_node entry = rule.child("entry"); entry;
                 entry = entry.next_sibling("entry")) {
                std::string entryValue = entry.text().get();
                if (ruleType == "systempath")
                    emulatorRules.systemPaths.push_back(entryValue);
                else if (ruleType == "staticpath")
                    emulatorRules.staticPaths.push_back(entryValue);
#if defined(_WIN64)
                else if (ruleType == "winregistrypath")
                    emulatorRules.winRegistryPaths.push_back(entryValue);
#endif
            }
        }
        mEmulators[emulatorName] = emulatorRules;
        emulatorRules.systemPaths.clear();
        emulatorRules.staticPaths.clear();
#if defined(_WIN64)
        emulatorRules.winRegistryPaths.clear();
#endif
    }

    for (pugi::xml_node core = ruleList.child("core"); core; core = core.next_sibling("core")) {
        std::string coreName = core.attribute("name").as_string();
        if (coreName.empty()) {
            LOG(LogWarning) << "Found core tag without name attribute, skipping entry";
            continue;
        }
        if (mCores.find(coreName) != mCores.end()) {
            LOG(LogWarning) << "Found repeating core tag \"" << coreName << "\", skipping entry";
            continue;
        }
        for (pugi::xml_node rule = core.child("rule"); rule; rule = rule.next_sibling("rule")) {
            std::string ruleType = rule.attribute("type").as_string();
            if (ruleType.empty()) {
                LOG(LogWarning) << "Found rule tag without type attribute for core \"" << coreName
                                << "\", skipping entry";
                continue;
            }
            if (ruleType != "corepath") {
                LOG(LogWarning) << "Found invalid rule type \"" << ruleType << "\" for core \""
                                << coreName << "\", skipping entry";
                continue;
            }
            for (pugi::xml_node entry = rule.child("entry"); entry;
                 entry = entry.next_sibling("entry")) {
                std::string entryValue = entry.text().get();
                if (ruleType == "corepath")
                    coreRules.corePaths.push_back(entryValue);
            }
        }
        mCores[coreName] = coreRules;
        coreRules.corePaths.clear();
    }
}

SystemData::SystemData(const std::string& name,
                       const std::string& fullName,
                       SystemEnvironmentData* envData,
                       const std::string& themeFolder,
                       bool CollectionSystem,
                       bool CustomCollectionSystem)
    : mName(name)
    , mFullName(fullName)
    , mEnvData(envData)
    , mThemeFolder(themeFolder)
    , mIsCollectionSystem(CollectionSystem)
    , mIsCustomCollectionSystem(CustomCollectionSystem)
    , mIsGroupedCustomCollectionSystem(false)
    , mIsGameSystem(true)
    , mScrapeFlag(false)
    , mPlaceholder(nullptr)
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
        // Virtual systems are updated afterwards by CollectionSystemsManager.
        // We're just creating the data structure here.
        mRootFolder = new FileData(FOLDER, "" + name, mEnvData, this);
        setupSystemSortType(mRootFolder);
    }

    // This placeholder can be used later in the gamelist view.
    mPlaceholder = new FileData(PLACEHOLDER, "<No Entries Found>", getSystemEnvData(), this);

    setIsGameSystemStatus();
    loadTheme();
}

SystemData::~SystemData()
{
    if (Settings::getInstance()->getString("SaveGamelistsMode") == "on exit") {
        if (mRootFolder->getGameCount().first + mRootFolder->getGameCount().second != 0)
            writeMetaData();
    }

    if (!mEnvData->mStartPath.empty())
        delete mEnvData;
    delete mRootFolder;
    delete mPlaceholder;
    delete mFilterIndex;
}

void SystemData::setIsGameSystemStatus()
{
    // We exclude non-game systems from specific operations (i.e. the "RetroPie" system, at least).
    // If/when there are more in the future, maybe this can be a more complex method, with a proper
    // list but for now a simple string comparison is enough.
    mIsGameSystem = (mName != "retropie");
}

bool SystemData::populateFolder(FileData* folder)
{
    const std::string& folderPath = folder->getPath();

    std::string filePath;
    std::string extension;
    bool isGame;
    bool showHiddenFiles = Settings::getInstance()->getBool("ShowHiddenFiles");
    Utils::FileSystem::stringList dirContent = Utils::FileSystem::getDirContent(folderPath);

    // If system directory exists but contains no games, return as error.
    if (dirContent.size() == 0)
        return false;

    for (Utils::FileSystem::stringList::const_iterator it = dirContent.cbegin();
         it != dirContent.cend(); it++) {
        filePath = *it;

        // Skip any recursive symlinks as those would hang the application at various places.
        if (Utils::FileSystem::isSymlink(filePath)) {
            if (Utils::FileSystem::resolveSymlink(filePath) ==
                Utils::FileSystem::getFileName(filePath)) {
                LOG(LogWarning) << "Skipped \"" << filePath << "\" as it's a recursive symlink";
                continue;
            }
        }

        // Skip hidden files and folders.
        if (!showHiddenFiles && Utils::FileSystem::isHidden(filePath)) {
            LOG(LogDebug) << "SystemData::populateFolder(): Skipping hidden "
                          << (Utils::FileSystem::isDirectory(filePath) ? "directory \"" : "file \"")
                          << filePath << "\"";
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
            else {
                delete newGame;
            }
        }

        // Add directories that also do not match an extension as folders.
        if (!isGame && Utils::FileSystem::isDirectory(filePath)) {
            // Make sure that it's not a recursive symlink pointing to a location higher in the
            // hierarchy as the application would run forever trying to resolve the link.
            if (Utils::FileSystem::isSymlink(filePath)) {
                const std::string canonicalPath = Utils::FileSystem::getCanonicalPath(filePath);
                const std::string canonicalStartPath =
                    Utils::FileSystem::getCanonicalPath(mEnvData->mStartPath);
                const std::string combinedPath =
                    mEnvData->mStartPath +
                    canonicalPath.substr(canonicalStartPath.size(),
                                         canonicalStartPath.size() - canonicalPath.size());
                if (filePath.find(combinedPath) == 0) {
                    LOG(LogWarning) << "Skipped \"" << filePath << "\" as it's a recursive symlink";
                    continue;
                }
            }

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

    for (std::vector<FileData*>::const_iterator it = children.cbegin(); // Line break.
         it != children.cend(); it++) {
        switch ((*it)->getType()) {
            case GAME:
                mFilterIndex->addToIndex(*it);
                break;
            case FOLDER:
                indexAllGameFilters(*it);
                break;
            default:
                break;
        }
    }
}

std::vector<std::string> readList(const std::string& str, const std::string& delims = " \t\r\n,")
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

bool SystemData::loadConfig()
{
    deleteSystems();

    if (sFindRules.get() == nullptr)
        sFindRules = std::make_unique<FindRules>();

    LOG(LogInfo) << "Populating game systems...";

    std::string path = getConfigPath(true);
    const std::string rompath = FileData::getROMDirectory();

    LOG(LogInfo) << "Parsing systems configuration file \"" << path << "\"...";

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
#else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
#endif

    if (!res) {
        LOG(LogError) << "Couldn't parse es_systems.xml: " << res.description();
        return true;
    }

    // Actually read the file.
    pugi::xml_node systemList = doc.child("systemList");

    if (!systemList) {
        LOG(LogError) << "es_systems.xml is missing the <systemList> tag";
        return true;
    }

    for (pugi::xml_node system = systemList.child("system"); system;
         system = system.next_sibling("system")) {
        std::string name;
        std::string fullname;
        std::string path;
        std::string themeFolder;

        name = system.child("name").text().get();
        fullname = system.child("fullname").text().get();
        path = system.child("path").text().get();

        auto nameFindFunc = [&] {
            for (auto system : sSystemVector) {
                if (system->mName == name) {
                    LOG(LogWarning) << "A system with the name \"" << name
                                    << "\" has already been loaded, skipping duplicate entry";
                    return true;
                }
            }
            return false;
        };

        // If the name is matching a system that has already been loaded, then skip the entry.
        if (nameFindFunc())
            continue;

        // If there is a %ROMPATH% variable set for the system, expand it. By doing this
        // it's possible to use either absolute ROM paths in es_systems.xml or to utilize
        // the ROM path configured as ROMDirectory in es_settings.xml. If it's set to ""
        // in this configuration file, the default hardcoded path $HOME/ROMs/ will be used.
        path = Utils::String::replace(path, "%ROMPATH%", rompath);
#if defined(_WIN64)
        path = Utils::String::replace(path, "\\", "/");
#endif
        path = Utils::String::replace(path, "//", "/");

        // Check that the ROM directory for the system is valid or otherwise abort the processing.
        if (!Utils::FileSystem::exists(path)) {
            LOG(LogDebug) << "SystemData::loadConfig(): Skipping system \"" << name
                          << "\" as the defined ROM directory \"" << path << "\" does not exist";
            continue;
        }
        if (!Utils::FileSystem::isDirectory(path)) {
            LOG(LogDebug) << "SystemData::loadConfig(): Skipping system \"" << name
                          << "\" as the defined ROM directory \"" << path
                          << "\" is not actually a directory";
            continue;
        }
        if (Utils::FileSystem::isSymlink(path)) {
            // Make sure that the symlink is not pointing to somewhere higher in the hierarchy
            // as that would lead to an infite loop, meaning the application would never start.
            std::string resolvedRompath = Utils::FileSystem::getCanonicalPath(rompath);
            if (resolvedRompath.find(Utils::FileSystem::getCanonicalPath(path)) == 0) {
                LOG(LogWarning) << "Skipping system \"" << name
                                << "\" as the defined ROM directory \"" << path
                                << "\" is an infinitely recursive symlink";
                continue;
            }
        }

        // Convert extensions list from a string into a vector of strings.
        std::vector<std::string> extensions = readList(system.child("extension").text().get());

        // Load all launch command tags for the system and if there are multiple tags, then
        // the label attribute needs to be set on all entries as it's a requirement for the
        // alternative emulator logic.
        std::vector<std::pair<std::string, std::string>> commands;
        for (pugi::xml_node entry = system.child("command"); entry;
             entry = entry.next_sibling("command")) {
            if (!entry.attribute("label")) {
                if (commands.size() == 1) {
                    // The first command tag had a label but the second one doesn't.
                    LOG(LogError)
                        << "Missing mandatory label attribute for alternative emulator "
                           "entry, only the first command tag will be processed for system \""
                        << name << "\"";
                    break;
                }
                else if (commands.size() > 1) {
                    // At least two command tags had a label but this one doesn't.
                    LOG(LogError)
                        << "Missing mandatory label attribute for alternative emulator "
                           "entry, no additional command tags will be processed for system \""
                        << name << "\"";
                    break;
                }
            }
            else if (!commands.empty() && commands.back().second == "") {
                // There are more than one command tags and the first tag did not have a label.
                LOG(LogError)
                    << "Missing mandatory label attribute for alternative emulator "
                       "entry, only the first command tag will be processed for system \""
                    << name << "\"";
                break;
            }
            commands.push_back(
                std::make_pair(entry.text().get(), entry.attribute("label").as_string()));
        }

        // Platform ID list
        const std::string platformList =
            Utils::String::toLower(system.child("platform").text().get());

        if (platformList == "") {
            LOG(LogWarning) << "No platform defined for system \"" << name
                            << "\", scraper searches will be inaccurate";
        }

        std::vector<std::string> platformStrs = readList(platformList);
        std::vector<PlatformIds::PlatformId> platformIds;
        for (auto it = platformStrs.cbegin(); it != platformStrs.cend(); it++) {
            std::string str = *it;
            PlatformIds::PlatformId platformId = PlatformIds::getPlatformId(str);

            if (platformId == PlatformIds::PLATFORM_IGNORE) {
                // When platform is PLATFORM_IGNORE, do not allow other platforms.
                platformIds.clear();
                platformIds.push_back(platformId);
                break;
            }

            // If there's a platform entry defined but it does not match the list of supported
            // platforms, then generate a warning.
            if (str != "" && platformId == PlatformIds::PLATFORM_UNKNOWN)
                LOG(LogWarning) << "Unknown platform \"" << str << "\" defined for system \""
                                << name << "\", scraper searches will be inaccurate";
            else if (platformId != PlatformIds::PLATFORM_UNKNOWN)
                platformIds.push_back(platformId);
        }

        // Theme folder.
        themeFolder = system.child("theme").text().as_string(name.c_str());

        // Validate.

        if (name.empty()) {
            LOG(LogError)
                << "A system in the es_systems.xml file has no name defined, skipping entry";
            continue;
        }
        else if (fullname.empty() || path.empty() || extensions.empty() || commands.empty()) {
            LOG(LogError) << "System \"" << name
                          << "\" is missing the fullname, path, "
                             "extension, or command tag, skipping entry";
            continue;
        }

        // Convert path to generic directory seperators.
        path = Utils::FileSystem::getGenericPath(path);

#if defined(_WIN64)
        if (!Settings::getInstance()->getBool("ShowHiddenFiles") &&
            Utils::FileSystem::isHidden(path)) {
            LOG(LogWarning) << "Skipping hidden ROM folder \"" << path << "\"";
            continue;
        }
#endif

        // Create the system runtime environment data.
        SystemEnvironmentData* envData = new SystemEnvironmentData;
        envData->mStartPath = path;
        envData->mSearchExtensions = extensions;
        envData->mLaunchCommands = commands;
        envData->mPlatformIds = platformIds;

        SystemData* newSys = new SystemData(name, fullname, envData, themeFolder);
        bool onlyHidden = false;

        // If the option to show hidden games has been disabled, then check whether all
        // games for the system are hidden. That will flag the system as empty.
        if (!Settings::getInstance()->getBool("ShowHiddenGames")) {
            std::vector<FileData*> recursiveGames = newSys->getRootFolder()->getChildrenRecursive();
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
            LOG(LogDebug) << "SystemData::loadConfig(): Skipping system \"" << name
                          << "\" as no files matched any of the defined file extensions";
            delete newSys;
        }
        else {
            sSystemVector.push_back(newSys);
        }
    }

    // Sort systems by their full names.
    std::sort(std::begin(sSystemVector), std::end(sSystemVector),
              [](SystemData* a, SystemData* b) { return a->getFullName() < b->getFullName(); });

    // Don't load any collections if there are no systems available.
    if (sSystemVector.size() > 0)
        CollectionSystemsManager::get()->loadCollectionSystems();

    return false;
}

void SystemData::deleteSystems()
{
    for (unsigned int i = 0; i < sSystemVector.size(); i++)
        delete sSystemVector.at(i);

    sSystemVector.clear();
}

std::string SystemData::getConfigPath(bool legacyWarning)
{
    if (legacyWarning) {
        std::string legacyConfigFile =
            Utils::FileSystem::getHomePath() + "/.emulationstation/es_systems.cfg";

        if (Utils::FileSystem::exists(legacyConfigFile)) {
            LOG(LogInfo) << "Found legacy systems configuration file \"" << legacyConfigFile
                         << "\", to retain your customizations move it to "
                            "\"custom_systems/es_systems.xml\" or otherwise delete the file";
        }
    }

    std::string customSystemsDirectory =
        Utils::FileSystem::getHomePath() + "/.emulationstation/custom_systems";

    if (!Utils::FileSystem::exists(customSystemsDirectory)) {
        LOG(LogInfo) << "Creating custom systems directory \"" << customSystemsDirectory << "\"...";
        Utils::FileSystem::createDirectory(customSystemsDirectory);
        if (!Utils::FileSystem::exists(customSystemsDirectory)) {
            LOG(LogError) << "Couldn't create directory, permission problems?";
        }
    }

    std::string path = customSystemsDirectory + "/es_systems.xml";

    if (Utils::FileSystem::exists(path)) {
        LOG(LogInfo) << "Found custom systems configuration file";
        return path;
    }

#if defined(_WIN64)
    path =
        ResourceManager::getInstance()->getResourcePath(":/systems/windows/es_systems.xml", true);
#elif defined(__APPLE__)
    path = ResourceManager::getInstance()->getResourcePath(":/systems/macos/es_systems.xml", true);
#else
    path = ResourceManager::getInstance()->getResourcePath(":/systems/unix/es_systems.xml", true);
#endif

    return path;
}

bool SystemData::createSystemDirectories()
{
    std::string path = getConfigPath(false);
    const std::string rompath = FileData::getROMDirectory();

    if (!Utils::FileSystem::exists(path)) {
        LOG(LogInfo) << "Systems configuration file does not exist, aborting";
        return true;
    }

    LOG(LogInfo) << "Generating ROM directory structure...";

    if (Utils::FileSystem::exists(rompath) && Utils::FileSystem::isRegularFile(rompath)) {
        LOG(LogError) << "Requested ROM directory \"" << rompath
                      << "\" is actually a file, aborting";
        return true;
    }

    if (!Utils::FileSystem::exists(rompath)) {
        LOG(LogInfo) << "Creating base ROM directory \"" << rompath << "\"...";
        if (!Utils::FileSystem::createDirectory(rompath)) {
            LOG(LogError) << "Couldn't create directory, permission problems or disk full?";
            return true;
        }
    }
    else {
        LOG(LogInfo) << "Base ROM directory \"" << rompath << "\" already exists";
    }

    LOG(LogInfo) << "Parsing systems configuration file \"" << path << "\"...";

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result res = doc.load_file(Utils::String::stringToWideString(path).c_str());
#else
    pugi::xml_parse_result res = doc.load_file(path.c_str());
#endif

    if (!res) {
        LOG(LogError) << "Couldn't parse es_systems.xml";
        LOG(LogError) << res.description();
        return true;
    }

    // Actually read the file.
    pugi::xml_node systemList = doc.child("systemList");

    if (!systemList) {
        LOG(LogError) << "es_systems.xml is missing the <systemList> tag";
        return true;
    }

    std::vector<std::string> systemsVector;

    for (pugi::xml_node system = systemList.child("system"); system;
         system = system.next_sibling("system")) {
        std::string systemDir;
        std::string name;
        std::string fullname;
        std::string path;
        std::string extensions;
        std::vector<std::string> commands;
        std::string platform;
        std::string themeFolder;
        const std::string systemInfoFileName = "/systeminfo.txt";
        bool replaceInfoFile = false;
        std::ofstream systemInfoFile;

        name = system.child("name").text().get();
        fullname = system.child("fullname").text().get();
        path = system.child("path").text().get();
        extensions = system.child("extension").text().get();
        for (pugi::xml_node entry = system.child("command"); entry;
             entry = entry.next_sibling("command")) {
            commands.push_back(entry.text().get());
        }
        platform = Utils::String::toLower(system.child("platform").text().get());
        themeFolder = system.child("theme").text().as_string(name.c_str());

        // Check that the %ROMPATH% variable is actually used for the path element.
        // If not, skip the system.
        if (path.find("%ROMPATH%") != 0) {
            LOG(LogWarning) << "The path element for system \"" << name
                            << "\" does not "
                               "utilize the %ROMPATH% variable, skipping entry";
            continue;
        }
        else {
            systemDir = path.substr(9, path.size() - 9);
        }

        // Trim any leading directory separator characters.
        systemDir.erase(systemDir.begin(),
                        std::find_if(systemDir.begin(), systemDir.end(),
                                     [](char c) { return c != '/' && c != '\\'; }));

        if (!Utils::FileSystem::exists(rompath + systemDir)) {
            if (!Utils::FileSystem::createDirectory(rompath + systemDir)) {
                LOG(LogError) << "Couldn't create system directory \"" << systemDir
                              << "\", permission problems or disk full?";
                return true;
            }
            else {
                LOG(LogInfo) << "Created system directory \"" << systemDir << "\"";
            }
        }
        else {
            LOG(LogInfo) << "System directory \"" << systemDir << "\" already exists";
        }

        if (Utils::FileSystem::exists(rompath + systemDir + systemInfoFileName))
            replaceInfoFile = true;
        else
            replaceInfoFile = false;

        if (replaceInfoFile) {
            if (Utils::FileSystem::removeFile(rompath + systemDir + systemInfoFileName))
                return true;
        }

#if defined(_WIN64)
        systemInfoFile.open(
            Utils::String::stringToWideString(rompath + systemDir + systemInfoFileName).c_str());
#else
        systemInfoFile.open(rompath + systemDir + systemInfoFileName);
#endif

        if (systemInfoFile.fail()) {
            LOG(LogError) << "Couldn't create system information file \""
                          << rompath + systemDir + systemInfoFileName
                          << "\", permission problems or disk full?";
            systemInfoFile.close();
            return true;
        }

        systemInfoFile << "System name:" << std::endl;
        systemInfoFile << name << std::endl << std::endl;
        systemInfoFile << "Full system name:" << std::endl;
        systemInfoFile << fullname << std::endl << std::endl;
        systemInfoFile << "Supported file extensions:" << std::endl;
        systemInfoFile << extensions << std::endl << std::endl;
        systemInfoFile << "Launch command:" << std::endl;
        systemInfoFile << commands.front() << std::endl << std::endl;
        // Alternative emulator configuration entries.
        if (commands.size() > 1) {
            systemInfoFile << (commands.size() == 2 ? "Alternative launch command:" :
                                                      "Alternative launch commands:")
                           << std::endl;
            for (auto it = commands.cbegin() + 1; it != commands.cend(); it++)
                systemInfoFile << (*it) << std::endl;
            systemInfoFile << std::endl;
        }
        systemInfoFile << "Platform (for scraping):" << std::endl;
        systemInfoFile << platform << std::endl << std::endl;
        systemInfoFile << "Theme folder:" << std::endl;
        systemInfoFile << themeFolder << std::endl;
        systemInfoFile.close();

        systemsVector.push_back(systemDir + ": " + fullname);

        if (replaceInfoFile) {
            LOG(LogInfo) << "Replaced existing system information file \""
                         << rompath + systemDir + systemInfoFileName << "\"";
        }
        else {
            LOG(LogInfo) << "Created system information file \""
                         << rompath + systemDir + systemInfoFileName << "\"";
        }
    }

    // Also generate a systems.txt file directly in the ROM directory root that contains the
    // mappings between the system directory names and the full system names. This makes it
    // easier for the users to identify the correct directories for their games.
    if (!systemsVector.empty()) {
        const std::string systemsFileName = "/systems.txt";
        bool systemsFileSuccess = true;

        if (Utils::FileSystem::exists(rompath + systemsFileName)) {
            if (Utils::FileSystem::removeFile(rompath + systemsFileName))
                systemsFileSuccess = false;
        }

        if (systemsFileSuccess) {
            std::ofstream systemsFile;
#if defined(_WIN64)
            systemsFile.open(Utils::String::stringToWideString(rompath + systemsFileName).c_str());
#else
            systemsFile.open(rompath + systemsFileName);
#endif
            if (systemsFile.fail()) {
                systemsFileSuccess = false;
            }
            else {
                for (std::string systemEntry : systemsVector) {
                    systemsFile << systemEntry << std::endl;
                }
                systemsFile.close();
            }
        }

        if (!systemsFileSuccess) {
            LOG(LogWarning) << "System directories successfully created but couldn't create "
                               "the systems.txt file in the ROM directory root";
            return false;
        }
    }

    LOG(LogInfo) << "System directories successfully created";
    return false;
}

bool SystemData::isVisible()
{
    // This function doesn't make much sense at the moment; if a system does not have any
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

    filePath = Utils::FileSystem::getHomePath() + "/.emulationstation/gamelists/" + mName +
               "/gamelist.xml";

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
    localThemePath =
        Utils::FileSystem::getParent(Utils::FileSystem::getParent(localThemePath)) + "/theme.xml";

    return localThemePath;
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
    } while (randomSystem == currentSystem);

    return randomSystem;
}

FileData* SystemData::getRandomGame(const FileData* currentGame)
{
    std::vector<FileData*> gameList;
    bool onlyFolders = false;
    bool hasFolders = false;

    // If we're in the custom collection group list, then get the list of collections,
    // otherwise get a list of all the folder and file entries in the view.
    if (currentGame && currentGame->getType() == FOLDER &&
        currentGame->getSystem()->isGroupedCustomCollection()) {
        gameList = mRootFolder->getParent()->getChildrenListToDisplay();
    }
    else {
        gameList = ViewController::get()
                       ->getGameListView(mRootFolder->getSystem())
                       .get()
                       ->getCursor()
                       ->getParent()
                       ->getChildrenListToDisplay();
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
    } while (currentGame && gameList.at(target) == currentGame);

    return gameList.at(target);
}

void SystemData::sortSystem(bool reloadGamelist, bool jumpToFirstRow)
{
    if (getName() == "recent")
        return;

    bool favoritesSorting;

    if (this->isCustomCollection() ||
        (this->isCollection() && this->getFullName() == "collections")) {
        favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
    }
    else {
        favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");
    }

    FileData* rootFolder = getRootFolder();
    // Assign the sort type to all grouped custom collections.
    if (mIsCollectionSystem && mFullName == "collections") {
        for (auto it = rootFolder->getChildren().begin(); // Line break.
             it != rootFolder->getChildren().end(); it++) {
            setupSystemSortType((*it)->getSystem()->getRootFolder());
        }
    }
    setupSystemSortType(rootFolder);

    rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                     favoritesSorting);

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

void SystemData::writeMetaData()
{
    if (Settings::getInstance()->getBool("IgnoreGamelist") || mIsCollectionSystem)
        return;

    // Save changed game data back to xml.
    updateGamelist(this);
}

void SystemData::onMetaDataSavePoint()
{
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
                mRootFolder->setSortTypeString(
                    Settings::getInstance()->getString("DefaultSortOrder"));
                break;
            }
        }
    }
    // If no valid sort type was defined in the configuration file, set to default sorting.
    if (mRootFolder->getSortTypeString() == "")
        mRootFolder->setSortTypeString(
            Settings::getInstance()->getDefaultString("DefaultSortOrder"));
}

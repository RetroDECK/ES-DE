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
#include "GamelistFileParser.h"
#include "InputManager.h"
#include "Log.h"
#include "Settings.h"
#include "ThemeData.h"
#include "UIModeController.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "views/GamelistView.h"
#include "views/ViewController.h"

#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>

#include <fstream>
#include <pugixml.hpp>
#include <random>

FindRules::FindRules()
{
    LOG(LogInfo) << "Loading emulator find rules...";
    loadFindRules();
}

void FindRules::loadFindRules()
{
    const std::string& customSystemsDirectory {Utils::FileSystem::getHomePath() +
                                               "/.emulationstation/custom_systems"};

    std::string path {customSystemsDirectory + "/es_find_rules.xml"};

    if (Utils::FileSystem::exists(path)) {
        LOG(LogInfo) << "Found custom find rules configuration file";
    }
    else {
#if defined(_WIN64)
        path = ResourceManager::getInstance().getResourcePath(":/systems/windows/es_find_rules.xml",
                                                              false);
#elif defined(__APPLE__)
        path = ResourceManager::getInstance().getResourcePath(":/systems/macos/es_find_rules.xml",
                                                              false);
#else
        path = ResourceManager::getInstance().getResourcePath(":/systems/unix/es_find_rules.xml",
                                                              false);
#endif
    }

    if (path == "") {
        LOG(LogWarning) << "No find rules configuration file found";
        return;
    }

#if defined(_WIN64)
    LOG(LogInfo) << "Parsing find rules configuration file \""
                 << Utils::String::replace(path, "/", "\\") << "\"...";
#else
    LOG(LogInfo) << "Parsing find rules configuration file \"" << path << "\"...";
#endif

    pugi::xml_document doc;
#if defined(_WIN64)
    const pugi::xml_parse_result& res {
        doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
    const pugi::xml_parse_result& res {doc.load_file(path.c_str())};
#endif

    if (!res) {
        LOG(LogError) << "Couldn't parse es_find_rules.xml: " << res.description();
        return;
    }

    // Actually read the file.
    const pugi::xml_node& ruleList {doc.child("ruleList")};

    if (!ruleList) {
        LOG(LogError) << "es_find_rules.xml is missing the <ruleList> tag";
        return;
    }

    EmulatorRules emulatorRules;
    CoreRules coreRules;

    for (pugi::xml_node emulator {ruleList.child("emulator")}; emulator;
         emulator = emulator.next_sibling("emulator")) {
        const std::string& emulatorName {emulator.attribute("name").as_string()};
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
            const std::string& ruleType {rule.attribute("type").as_string()};
            if (ruleType.empty()) {
                LOG(LogWarning) << "Found rule tag without type attribute for emulator \""
                                << emulatorName << "\", skipping entry";
                continue;
            }
#if defined(_WIN64)
            if (ruleType != "winregistrypath" && ruleType != "winregistryvalue" &&
                ruleType != "systempath" && ruleType != "staticpath") {
#else
            if (ruleType != "systempath" && ruleType != "staticpath") {
#endif
                LOG(LogWarning) << "Found invalid rule type \"" << ruleType << "\" for emulator \""
                                << emulatorName << "\", skipping entry";
                continue;
            }
            for (pugi::xml_node entry {rule.child("entry")}; entry;
                 entry = entry.next_sibling("entry")) {
                const std::string& entryValue {entry.text().get()};
                if (ruleType == "systempath")
                    emulatorRules.systemPaths.emplace_back(entryValue);
                else if (ruleType == "staticpath")
                    emulatorRules.staticPaths.emplace_back(entryValue);
#if defined(_WIN64)
                else if (ruleType == "winregistrypath")
                    emulatorRules.winRegistryPaths.emplace_back(entryValue);
                else if (ruleType == "winregistryvalue")
                    emulatorRules.winRegistryValues.emplace_back(entryValue);
#endif
            }
        }
        mEmulators[emulatorName] = emulatorRules;
        emulatorRules.systemPaths.clear();
        emulatorRules.staticPaths.clear();
#if defined(_WIN64)
        emulatorRules.winRegistryPaths.clear();
        emulatorRules.winRegistryValues.clear();
#endif
    }

    for (pugi::xml_node core {ruleList.child("core")}; core; core = core.next_sibling("core")) {
        const std::string& coreName {core.attribute("name").as_string()};
        if (coreName.empty()) {
            LOG(LogWarning) << "Found core tag without name attribute, skipping entry";
            continue;
        }
        if (mCores.find(coreName) != mCores.end()) {
            LOG(LogWarning) << "Found repeating core tag \"" << coreName << "\", skipping entry";
            continue;
        }
        for (pugi::xml_node rule {core.child("rule")}; rule; rule = rule.next_sibling("rule")) {
            const std::string& ruleType {rule.attribute("type").as_string()};
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
            for (pugi::xml_node entry {rule.child("entry")}; entry;
                 entry = entry.next_sibling("entry")) {
                const std::string& entryValue {entry.text().get()};
                if (ruleType == "corepath")
                    coreRules.corePaths.emplace_back(entryValue);
            }
        }
        mCores[coreName] = coreRules;
        coreRules.corePaths.clear();
    }
}

SystemData::SystemData(const std::string& name,
                       const std::string& fullName,
                       const std::string& sortName,
                       SystemEnvironmentData* envData,
                       const std::string& themeFolder,
                       bool CollectionSystem,
                       bool CustomCollectionSystem)
    : mName {name}
    , mFullName {fullName}
    , mSortName {sortName}
    , mEnvData {envData}
    , mThemeFolder {themeFolder}
    , mSymlinkMaxDepthReached {false}
    , mIsCollectionSystem {CollectionSystem}
    , mIsCustomCollectionSystem {CustomCollectionSystem}
    , mIsGroupedCustomCollectionSystem {false}
    , mIsGameSystem {true}
    , mScrapeFlag {false}
    , mFlattenFolders {false}
    , mPlaceholder {nullptr}
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
            GamelistFileParser::parseGamelist(this);

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
    loadTheme(ThemeTriggers::TriggerType::NONE);
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
    // Reserved for future use, could be used to exclude certain systems from some operations,
    // such as dedicated tools systems and similar.
    mIsGameSystem = true;
}

bool SystemData::populateFolder(FileData* folder)
{
    if (mSymlinkMaxDepthReached)
        return false;

    std::string filePath;
    std::string extension;
    const std::string& folderPath {folder->getPath()};
    const bool showHiddenFiles {Settings::getInstance()->getBool("ShowHiddenFiles")};
    const Utils::FileSystem::StringList& dirContent {Utils::FileSystem::getDirContent(folderPath)};
    bool isGame {false};

    // If system directory exists but contains no games, return as error.
    if (dirContent.size() == 0)
        return false;

    if (std::find(dirContent.cbegin(), dirContent.cend(), mEnvData->mStartPath + "/noload.txt") !=
        dirContent.cend()) {
        LOG(LogInfo) << "Not populating system \"" << mName << "\" as a noload.txt file is present";
        return false;
    }

    if (std::find(dirContent.cbegin(), dirContent.cend(), mEnvData->mStartPath + "/flatten.txt") !=
        dirContent.cend()) {
        LOG(LogInfo) << "A flatten.txt file is present for the \"" << mName
                     << "\" system, folder flattening will be applied";
        mFlattenFolders = true;
    }

    for (Utils::FileSystem::StringList::const_iterator it {dirContent.cbegin()};
         it != dirContent.cend(); ++it) {
        filePath = *it;
        const bool isDirectory {Utils::FileSystem::isDirectory(filePath)};

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
                          << (isDirectory ? "directory \"" : "file \"") << filePath << "\"";
            continue;
        }

        // This is a little complicated because we allow a list
        // of extensions to be defined (delimited with a space).
        // We first get the extension of the file itself:
        extension = Utils::FileSystem::getExtension(filePath);

        isGame = false;

        if (std::find(mEnvData->mSearchExtensions.cbegin(), mEnvData->mSearchExtensions.cend(),
                      extension) != mEnvData->mSearchExtensions.cend() &&
            !(isDirectory && extension == ".")) {
            FileData* newGame {new FileData(GAME, filePath, mEnvData, this)};

            // If adding a configured file extension to a directory it will get interpreted as
            // a regular file. This is useful for displaying multi-file/multi-disc games as single
            // entries or for emulators that can get directories passed to them as command line
            // parameters instead of regular files. In these instances we remove the extension
            // from the metadata name so it does not show up in the gamelists and similar.
            if (isDirectory && extension != ".") {
                const std::string folderName {newGame->metadata.get("name")};
                newGame->metadata.set(
                    "name", folderName.substr(0, folderName.length() - extension.length()));
            }

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
        if (!isGame && isDirectory) {
            // Make sure that it's not a recursive symlink as the application would run into a
            // loop trying to resolve the link.
            if (Utils::FileSystem::isSymlink(filePath)) {
                bool recursiveSymlink {false};
                const std::string& canonicalPath {Utils::FileSystem::getCanonicalPath(filePath)};
                const std::string& canonicalStartPath {
                    Utils::FileSystem::getCanonicalPath(mEnvData->mStartPath)};
                // Last resort hack to prevent recursive symlinks in some really unusual situations.
                if (filePath.length() > canonicalStartPath.length() + 100) {
                    int folderDepth {0};
                    const std::string& path {filePath.substr(canonicalStartPath.length())};
                    for (char character : path) {
                        if (character == '/') {
                            ++folderDepth;
                            if (folderDepth == 20) {
                                LOG(LogWarning) << "Skipped \"" << filePath
                                                << "\" as it seems to be a recursive symlink";
                                mSymlinkMaxDepthReached = true;
                                return false;
                            }
                        }
                    }
                }
                if (canonicalStartPath.find(canonicalPath) != std::string::npos)
                    recursiveSymlink = true;
                else if (canonicalPath.size() >= canonicalStartPath.size() &&
                         canonicalPath.find(canonicalStartPath) != std::string::npos) {
                    const std::string& combinedPath {
                        mEnvData->mStartPath +
                        canonicalPath.substr(canonicalStartPath.size(),
                                             canonicalStartPath.size() - canonicalPath.size())};
                    if (Utils::FileSystem::getParent(filePath).find(combinedPath) == 0)
                        recursiveSymlink = true;
                }
                if (recursiveSymlink) {
                    LOG(LogWarning) << "Skipped \"" << filePath << "\" as it's a recursive symlink";
                    continue;
                }
            }

            FileData* newFolder {new FileData(FOLDER, filePath, mEnvData, this)};
            populateFolder(newFolder);

            if (mFlattenFolders) {
                for (auto& entry : newFolder->getChildrenByFilename())
                    folder->addChild(entry.second);
            }
            else {
                // Ignore folders that do not contain games.
                if (newFolder->getChildrenByFilename().size() == 0)
                    delete newFolder;
                else
                    folder->addChild(newFolder);
            }
        }
    }
    return true;
}

void SystemData::indexAllGameFilters(const FileData* folder)
{
    const std::vector<FileData*>& children {folder->getChildren()};

    for (std::vector<FileData*>::const_iterator it {children.cbegin()}; // Line break.
         it != children.cend(); ++it) {
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

    size_t prevOff {str.find_first_not_of(delims, 0)};
    size_t off {str.find_first_of(delims, prevOff)};
    while (off != std::string::npos || prevOff != std::string::npos) {
        ret.emplace_back(str.substr(prevOff, off - prevOff));

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

    if (Settings::getInstance()->getBool("ParseGamelistOnly")) {
        LOG(LogInfo) << "Only parsing the gamelist.xml files, not scanning system directories";
    }

    const std::vector<std::string>& configPaths {getConfigPath(true)};
    const std::string& rompath {FileData::getROMDirectory()};
    bool onlyProcessCustomFile {false};

    const bool splashScreen {Settings::getInstance()->getBool("SplashScreen")};
    float systemCount {0.0f};
    float parsedSystems {0.0f};
    unsigned int gameCount {0};

    // This is only done to get the total system count, for calculating the progress bar position.
    for (auto& configPath : configPaths) {
        pugi::xml_document doc;
#if defined(_WIN64)
        const pugi::xml_parse_result& res {
            doc.load_file(Utils::String::stringToWideString(configPath).c_str())};
#else
        const pugi::xml_parse_result& res {doc.load_file(configPath.c_str())};
#endif
        if (!res)
            break;
        const pugi::xml_node& systemList {doc.child("systemList")};
        if (!systemList)
            continue;
        for (pugi::xml_node system {systemList.child("system")}; system;
             system = system.next_sibling("system")) {
            ++systemCount;
        }
        if (doc.child("loadExclusive"))
            break;
    }

    for (auto& configPath : configPaths) {
        // If the loadExclusive tag is present in the custom es_systems.xml file, then skip
        // processing of the bundled configuration file.
        if (onlyProcessCustomFile)
            break;

#if defined(_WIN64)
        LOG(LogInfo) << "Parsing systems configuration file \""
                     << Utils::String::replace(configPath, "/", "\\") << "\"...";
#else
        LOG(LogInfo) << "Parsing systems configuration file \"" << configPath << "\"...";
#endif

        pugi::xml_document doc;
#if defined(_WIN64)
        const pugi::xml_parse_result& res {
            doc.load_file(Utils::String::stringToWideString(configPath).c_str())};
#else
        const pugi::xml_parse_result& res {doc.load_file(configPath.c_str())};
#endif

        if (!res) {
            LOG(LogError) << "Couldn't parse es_systems.xml: " << res.description();
            return true;
        }

        const pugi::xml_node& loadExclusive {doc.child("loadExclusive")};
        if (loadExclusive) {
            if (configPath == configPaths.front() && configPaths.size() > 1) {
                LOG(LogInfo) << "Only loading custom file as the <loadExclusive> tag is present";
                onlyProcessCustomFile = true;
            }
            else {
                LOG(LogWarning) << "A <loadExclusive> tag is present in the bundled es_systems.xml "
                                   "file, ignoring it as this is only supposed to be used for the "
                                   "custom es_systems.xml file";
            }
        }

        // Actually read the file.
        const pugi::xml_node& systemList {doc.child("systemList")};

        if (!systemList) {
            LOG(LogError) << "es_systems.xml is missing the <systemList> tag";
            return true;
        }

        unsigned int lastTime {0};
        unsigned int accumulator {0};
        SDL_Event event {};

        for (pugi::xml_node system {systemList.child("system")}; system;
             system = system.next_sibling("system")) {
            // Poll events so that the OS doesn't think the application is hanging on startup,
            // this is required as the main application loop hasn't started yet.
            while (SDL_PollEvent(&event)) {
                InputManager::getInstance().parseEvent(event);
                if (event.type == SDL_QUIT) {
                    sStartupExitSignal = true;
                    return true;
                }
            };

            std::string name;
            std::string fullname;
            std::string sortName;
            std::string path;
            std::string themeFolder;

            name = Utils::String::replace(system.child("name").text().get(), "\n", "");
            fullname = Utils::String::replace(system.child("fullname").text().get(), "\n", "");
            sortName = system.child("systemsortname").text().get();
            path = system.child("path").text().get();

            if (splashScreen) {
                const unsigned int curTime {SDL_GetTicks()};
                accumulator += curTime - lastTime;
                lastTime = curTime;
                ++parsedSystems;
                // This prevents Renderer::swapBuffers() from being called excessively which
                // could lead to significantly longer application startup times.
                if (accumulator > 40) {
                    accumulator = 0;
                    const float progress {glm::mix(0.0f, 0.5f, parsedSystems / systemCount)};
                    Window::getInstance()->renderSplashScreen(Window::SplashScreenState::SCANNING,
                                                              progress);
                    lastTime += SDL_GetTicks() - curTime;
                }
            }

            auto nameFindFunc = [&] {
                for (auto system : sSystemVector) {
                    if (system->mName == name) {
                        LOG(LogDebug) << "A system with the name \"" << name
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

            // In case ~ is used, expand it to the home directory path.
            path = Utils::FileSystem::expandHomePath(path);

            // Check that the ROM directory for the system is valid or otherwise abort the
            // processing.
            if (!Utils::FileSystem::exists(path)) {
                LOG(LogDebug) << "SystemData::loadConfig(): Skipping system \"" << name
#if defined(_WIN64)
                              << "\" as the defined ROM directory \""
                              << Utils::String::replace(path, "/", "\\")
#else
                              << "\" as the defined ROM directory \"" << path
#endif
                              << "\" does not exist";
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
                const std::string& resolvedRompath {Utils::FileSystem::getCanonicalPath(rompath)};
                if (resolvedRompath.find(Utils::FileSystem::getCanonicalPath(path)) == 0) {
                    LOG(LogWarning)
                        << "Skipping system \"" << name << "\" as the defined ROM directory \""
                        << path << "\" is an infinitely recursive symlink";
                    continue;
                }
            }

            // Convert extensions list from a string into a vector of strings.
            std::vector<std::string> extensions {readList(system.child("extension").text().get())};

            // Load all launch command tags for the system and if there are multiple tags, then
            // the label attribute needs to be set on all entries as it's a requirement for the
            // alternative emulator logic.
            std::vector<std::pair<std::string, std::string>> commands;
            for (pugi::xml_node entry {system.child("command")}; entry;
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
                commands.emplace_back(
                    std::make_pair(entry.text().get(), entry.attribute("label").as_string()));
            }

            // Platform ID list
            const std::string& platformList {
                Utils::String::toLower(system.child("platform").text().get())};

            if (platformList == "") {
                LOG(LogWarning) << "No platform defined for system \"" << name
                                << "\", scraper searches will be inaccurate";
            }

            const std::vector<std::string>& platformStrs {readList(platformList)};
            std::vector<PlatformIds::PlatformId> platformIds;
            for (auto it = platformStrs.cbegin(); it != platformStrs.cend(); ++it) {
                std::string str {*it};
                const PlatformIds::PlatformId platformId {PlatformIds::getPlatformId(str)};

                if (platformId == PlatformIds::PLATFORM_IGNORE) {
                    // When platform is PLATFORM_IGNORE, do not allow other platforms.
                    platformIds.clear();
                    platformIds.emplace_back(platformId);
                    break;
                }

                // If there's a platform entry defined but it does not match the list of supported
                // platforms, then generate a warning.
                if (str != "" && platformId == PlatformIds::PLATFORM_UNKNOWN)
                    LOG(LogWarning) << "Unknown platform \"" << str << "\" defined for system \""
                                    << name << "\", scraper searches will be inaccurate";
                else if (platformId != PlatformIds::PLATFORM_UNKNOWN)
                    platformIds.emplace_back(platformId);
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

            if (sortName == "") {
                sortName = fullname;
            }
            else {
                LOG(LogDebug) << "SystemData::loadConfig(): System \"" << name
                              << "\" has a <systemsortname> tag set, sorting as \"" << sortName
                              << "\" instead of \"" << fullname << "\"";
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
            SystemEnvironmentData* envData {new SystemEnvironmentData};
            envData->mStartPath = path;
            envData->mSearchExtensions = extensions;
            envData->mLaunchCommands = commands;
            envData->mPlatformIds = platformIds;

            SystemData* newSys {new SystemData(name, fullname, sortName, envData, themeFolder)};
            bool onlyHidden {false};

            // If the option to show hidden games has been disabled, then check whether all
            // games for the system are hidden. That will flag the system as empty.
            if (!Settings::getInstance()->getBool("ShowHiddenGames")) {
                std::vector<FileData*> recursiveGames {
                    newSys->getRootFolder()->getChildrenRecursive()};
                onlyHidden = true;
                for (auto it = recursiveGames.cbegin(); it != recursiveGames.cend(); ++it) {
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
                sSystemVector.emplace_back(newSys);
                gameCount += newSys->getRootFolder()->getGameCount().first;
            }
        }
    }

    if (splashScreen) {
        if (sSystemVector.size() > 0)
            Window::getInstance()->renderSplashScreen(Window::SplashScreenState::SCANNING, 0.5f);
        else
            Window::getInstance()->renderSplashScreen(Window::SplashScreenState::SCANNING, 1.0f);
    }

    LOG(LogInfo) << "Parsed configuration for " << systemCount << " system"
                 << (systemCount == 1 ? ", loaded " : "s, loaded ") << sSystemVector.size()
                 << " system" << (sSystemVector.size() == 1 ? "" : "s")
                 << " (collections not included)";
    LOG(LogInfo) << "Total game count: " << gameCount;

    // Sort systems by sortName, which will normally be the same as the full name.
    std::sort(std::begin(sSystemVector), std::end(sSystemVector), [](SystemData* a, SystemData* b) {
        return Utils::String::toUpper(a->getSortName()) < Utils::String::toUpper(b->getSortName());
    });

    // Don't load any collections if there are no systems available.
    if (sSystemVector.size() > 0)
        CollectionSystemsManager::getInstance()->loadCollectionSystems();

    return false;
}

std::string SystemData::getLaunchCommandFromLabel(const std::string& label)
{
    auto commandIter = std::find_if(
        mEnvData->mLaunchCommands.cbegin(), mEnvData->mLaunchCommands.cend(),
        [label](std::pair<std::string, std::string> command) { return (command.second == label); });

    if (commandIter != mEnvData->mLaunchCommands.cend())
        return (*commandIter).first;

    return "";
}

void SystemData::deleteSystems()
{
    for (unsigned int i {0}; i < sSystemVector.size(); ++i)
        delete sSystemVector.at(i);

    sSystemVector.clear();
}

std::vector<std::string> SystemData::getConfigPath(bool legacyWarning)
{
    std::vector<std::string> paths;

    if (legacyWarning) {
        const std::string& legacyConfigFile {Utils::FileSystem::getHomePath() +
                                             "/.emulationstation/es_systems.cfg"};

        if (Utils::FileSystem::exists(legacyConfigFile)) {
#if defined(_WIN64)
            LOG(LogInfo) << "Found legacy systems configuration file \""
                         << Utils::String::replace(legacyConfigFile, "/", "\\")
                         << "\", to retain your customizations move it to "
                            "\"custom_systems\\es_systems.xml\" or otherwise delete the file";
#else
            LOG(LogInfo) << "Found legacy systems configuration file \"" << legacyConfigFile
                         << "\", to retain your customizations move it to "
                            "\"custom_systems/es_systems.xml\" or otherwise delete the file";
#endif
        }
    }

    const std::string& customSystemsDirectory {Utils::FileSystem::getHomePath() +
                                               "/.emulationstation/custom_systems"};

    if (!Utils::FileSystem::exists(customSystemsDirectory)) {
        LOG(LogInfo) << "Creating custom systems directory \"" << customSystemsDirectory << "\"...";
        Utils::FileSystem::createDirectory(customSystemsDirectory);
        if (!Utils::FileSystem::exists(customSystemsDirectory)) {
            LOG(LogError) << "Couldn't create directory, permission problems?";
        }
    }

    std::string path {customSystemsDirectory + "/es_systems.xml"};

    if (Utils::FileSystem::exists(path)) {
        LOG(LogInfo) << "Found custom systems configuration file";
        paths.emplace_back(path);
    }

#if defined(_WIN64)
    path = ResourceManager::getInstance().getResourcePath(":/systems/windows/es_systems.xml", true);
#elif defined(__APPLE__)
    path = ResourceManager::getInstance().getResourcePath(":/systems/macos/es_systems.xml", true);
#else
    path = ResourceManager::getInstance().getResourcePath(":/systems/unix/es_systems.xml", true);
#endif

    paths.emplace_back(path);
    return paths;
}

bool SystemData::createSystemDirectories()
{
    std::vector<std::string> configPaths {getConfigPath(true)};
    const std::string& rompath {FileData::getROMDirectory()};

    bool onlyProcessCustomFile = false;

    LOG(LogInfo) << "Generating ROM directory structure...";

    if (Utils::FileSystem::exists(rompath) && Utils::FileSystem::isRegularFile(rompath)) {
        LOG(LogError) << "Requested ROM directory \"" << rompath
                      << "\" is actually a file, aborting";
        return true;
    }

    if (!Utils::FileSystem::exists(rompath)) {
#if defined(_WIN64)
        if (rompath.size() == 3 && rompath[1] == ':' && rompath[2] == '\\') {
            if (Utils::FileSystem::driveExists(rompath)) {
                LOG(LogInfo) << "ROM directory set to root of device " << rompath;
            }
            else {
                LOG(LogError) << "Device " << rompath << " does not exist";
                return true;
            }
        }
        else {
            LOG(LogInfo) << "Creating base ROM directory \"" << rompath << "\"...";
            if (!Utils::FileSystem::createDirectory(rompath)) {
                LOG(LogError) << "Couldn't create directory, permission problems or disk full?";
                return true;
            }
        }
#else
        LOG(LogInfo) << "Creating base ROM directory \"" << rompath << "\"...";
        if (!Utils::FileSystem::createDirectory(rompath)) {
            LOG(LogError) << "Couldn't create directory, permission problems or disk full?";
            return true;
        }
#endif
    }
    else {
        LOG(LogInfo) << "Base ROM directory \"" << rompath << "\" already exists";
    }

    if (configPaths.size() > 1) {
        // If the loadExclusive tag is present in the custom es_systems.xml file, then skip
        // processing of the bundled configuration file.
        pugi::xml_document doc;
#if defined(_WIN64)
        const pugi::xml_parse_result& res {
            doc.load_file(Utils::String::stringToWideString(configPaths.front()).c_str())};
#else
        const pugi::xml_parse_result& res {doc.load_file(configPaths.front().c_str())};
#endif
        if (res) {
            const pugi::xml_node& loadExclusive {doc.child("loadExclusive")};
            if (loadExclusive)
                onlyProcessCustomFile = true;
        }
    }

    // Process the custom es_systems.xml file after the bundled file, as any systems with identical
    // <path> tags will be overwritten by the last occurrence.
    std::reverse(configPaths.begin(), configPaths.end());

    std::vector<std::pair<std::string, std::string>> systemsVector;

    for (auto& configPath : configPaths) {
        // If the loadExclusive tag is present.
        if (onlyProcessCustomFile && configPath == configPaths.front())
            continue;

#if defined(_WIN64)
        LOG(LogInfo) << "Parsing systems configuration file \""
                     << Utils::String::replace(configPath, "/", "\\") << "\"...";
#else
        LOG(LogInfo) << "Parsing systems configuration file \"" << configPath << "\"...";
#endif

        pugi::xml_document doc;
#if defined(_WIN64)
        const pugi::xml_parse_result& res {
            doc.load_file(Utils::String::stringToWideString(configPath).c_str())};
#else
        const pugi::xml_parse_result& res {doc.load_file(configPath.c_str())};
#endif

        if (!res) {
            LOG(LogError) << "Couldn't parse es_systems.xml";
            LOG(LogError) << res.description();
            return true;
        }

        // Actually read the file.
        const pugi::xml_node& systemList {doc.child("systemList")};

        if (!systemList) {
            LOG(LogError) << "es_systems.xml is missing the <systemList> tag";
            return true;
        }

        for (pugi::xml_node system {systemList.child("system")}; system;
             system = system.next_sibling("system")) {
            std::string systemDir;
            std::string name;
            std::string fullname;
            std::string path;
            std::string extensions;
            std::vector<std::string> commands;
            std::string platform;
            std::string themeFolder;
            const std::string systemInfoFileName {"/systeminfo.txt"};
            bool replaceInfoFile = false;
            std::ofstream systemInfoFile;

            name = system.child("name").text().get();
            fullname = system.child("fullname").text().get();
            path = system.child("path").text().get();
            extensions = system.child("extension").text().get();
            for (pugi::xml_node entry {system.child("command")}; entry;
                 entry = entry.next_sibling("command")) {
                commands.emplace_back(entry.text().get());
            }
            platform = Utils::String::toLower(system.child("platform").text().get());
            const bool multiplePlatforms {
                std::find_if(platform.cbegin(), platform.cend(), [](char character) {
                    return (std::isspace(character) || character == ',');
                }) != platform.cend()};

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
                Utils::String::stringToWideString(rompath + systemDir + systemInfoFileName)
                    .c_str());
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
            if (configPaths.size() != 1 && configPath == configPaths.back())
                systemInfoFile << name << " (custom system)" << std::endl << std::endl;
            else
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
                for (auto it = commands.cbegin() + 1; it != commands.cend(); ++it)
                    systemInfoFile << (*it) << std::endl;
                systemInfoFile << std::endl;
            }
            systemInfoFile << "Platform" << (multiplePlatforms ? "s" : "")
                           << " (for scraping):" << std::endl;
            systemInfoFile << platform << std::endl << std::endl;
            systemInfoFile << "Theme folder:" << std::endl;
            systemInfoFile << themeFolder << std::endl;
            systemInfoFile.close();

            auto systemIter = std::find_if(systemsVector.cbegin(), systemsVector.cend(),
                                           [systemDir](std::pair<std::string, std::string> system) {
                                               return system.first == systemDir;
                                           });

            if (systemIter != systemsVector.cend())
                systemsVector.erase(systemIter);

            if (configPaths.size() != 1 && configPath == configPaths.back())
                systemsVector.emplace_back(
                    std::make_pair(systemDir + " (custom system)", fullname));
            else
                systemsVector.emplace_back(std::make_pair(systemDir, fullname));

            if (replaceInfoFile) {
                LOG(LogInfo) << "Replaced existing system information file \""
                             << rompath + systemDir + systemInfoFileName << "\"";
            }
            else {
                LOG(LogInfo) << "Created system information file \""
                             << rompath + systemDir + systemInfoFileName << "\"";
            }
        }
    }

    // Also generate a systems.txt file directly in the ROM directory root that contains the
    // mappings between the system directory names and the full system names. This makes it
    // easier for the users to identify the correct directories for their games.
    if (!systemsVector.empty()) {
        const std::string& systemsFileName {"/systems.txt"};
        bool systemsFileSuccess {true};

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
                std::sort(systemsVector.begin(), systemsVector.end());
                for (auto systemEntry : systemsVector) {
                    systemsFile << systemEntry.first.append(": ").append(systemEntry.second)
                                << std::endl;
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

SystemData* SystemData::getSystemByName(const std::string& systemName)
{
    for (auto it : sSystemVector) {
        if ((*it).getName() == systemName)
            return it;
    }

    return nullptr;
}

SystemData* SystemData::getNext() const
{
    auto it = std::find(sSystemVector.cbegin(), sSystemVector.cend(), this);

    ++it;
    if (it == sSystemVector.cend())
        it = sSystemVector.cbegin();

    return *it;
}

SystemData* SystemData::getPrev() const
{
    auto it = std::find(sSystemVector.crbegin(), sSystemVector.crend(), this);

    ++it;
    if (it == sSystemVector.crend())
        it = sSystemVector.crbegin();

    return *it;
}

std::string SystemData::getGamelistPath(bool forWrite) const
{
    std::string filePath {mRootFolder->getPath() + "/gamelist.xml"};
    const std::string gamelistPath {Utils::FileSystem::getHomePath() +
                                    "/.emulationstation/gamelists/" + mName};

    if (Utils::FileSystem::exists(filePath)) {
        if (Settings::getInstance()->getBool("LegacyGamelistFileLocation")) {
            return filePath;
        }
        else {
#if defined(_WIN64)
            LOG(LogWarning) << "Found a gamelist.xml file in \""
                            << Utils::String::replace(mRootFolder->getPath(), "/", "\\")
                            << "\\\" which will not get loaded, move it to \""
                            << Utils::String::replace(gamelistPath, "/", "\\")
                            << "\\\" or otherwise delete it";
#else
            LOG(LogWarning) << "Found a gamelist.xml file in \"" << mRootFolder->getPath()
                            << "/\" which will not get loaded, move it to \"" << gamelistPath
                            << "/\" or otherwise delete it";
#endif
        }
    }

    filePath = gamelistPath + "/gamelist.xml";

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
    // Check for the precence of [CURRENT_THEME_PATH]/[SYSTEM]/theme.xml and if this does not
    // exist, then try the default for the theme set, i.e. [CURRENT_THEME_PATH]/theme.xml
    std::string themePath {ThemeData::getThemeFromCurrentSet(mThemeFolder)};

    if (Utils::FileSystem::exists(themePath))
        return themePath;

    themePath = Utils::FileSystem::getParent(Utils::FileSystem::getParent(themePath));

    if (themePath != "") {
        themePath.append("/theme.xml");
        if (Utils::FileSystem::exists(themePath))
            return themePath;
    }

    return "";
}

SystemData* SystemData::getRandomSystem(const SystemData* currentSystem)
{
    int total {0};
    for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); ++it) {
        if ((*it)->isGameSystem())
            ++total;
    }

    if (total < 2)
        return nullptr;

    SystemData* randomSystem {nullptr};

    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine {randDev()};
        std::uniform_int_distribution<int> uniform_dist {0, total - 1};
        int target {uniform_dist(engine)};

        for (auto it = sSystemVector.cbegin(); it != sSystemVector.cend(); ++it) {
            if ((*it)->isGameSystem()) {
                if (target > 0) {
                    --target;
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

FileData* SystemData::getRandomGame(const FileData* currentGame, bool gameSelectorMode)
{
    std::vector<FileData*> gameList;
    bool onlyFolders {false};
    bool hasFolders {false};

    // If we're in the custom collection group list, then get the list of collections,
    // otherwise get a list of all the folder and file entries in the view.
    if (currentGame && currentGame->getType() == FOLDER &&
        currentGame->getSystem()->isGroupedCustomCollection()) {
        gameList = mRootFolder->getParent()->getChildrenListToDisplay();
    }
    else {
        if (gameSelectorMode) {
            gameList = mRootFolder->getFilesRecursive(GAME, false, false);
            if (Settings::getInstance()->getString("UIMode") == "kid") {
                // Doing some extra work here instead of in FileData is OK as it's only needed
                // for the rare combination of a gameselector being present while in kid mode.
                for (auto it = gameList.begin(); it != gameList.end();) {
                    if (!(*it)->getKidgame())
                        it = gameList.erase(it);
                    else
                        ++it;
                }
            }
        }
        else {
            gameList = ViewController::getInstance()
                           ->getGamelistView(mRootFolder->getSystem())
                           .get()
                           ->getCursor()
                           ->getParent()
                           ->getChildrenListToDisplay();
        }
    }

    if (gameList.size() > 0 && gameList.front()->getParent()->getOnlyFoldersFlag())
        onlyFolders = true;

    if (gameList.size() > 0 && gameList.front()->getParent()->getHasFoldersFlag())
        hasFolders = true;

    // If this is a mixed view of folders and files, then remove all the folder entries
    // as we want to exclude them from the random selection.
    if (!onlyFolders && hasFolders) {
        unsigned int i {0};
        do {
            if (gameList[i]->getType() == FOLDER)
                gameList.erase(gameList.begin() + i);
            else
                ++i;
        } while (i < gameList.size());
    }

    if (!currentGame && gameList.size() == 1)
        return gameList.front();

    // If there is only one folder and one file in the list, then return the file.
    if (!onlyFolders && hasFolders && gameList.size() == 1)
        return gameList.front();

    if (currentGame && currentGame->getType() == PLACEHOLDER)
        return nullptr;

    int total {static_cast<int>(gameList.size())};
    int target {0};

    if (total < 2)
        return nullptr;

    do {
        // Get a random number in range.
        std::random_device randDev;
        //  Mersenne Twister pseudorandom number generator.
        std::mt19937 engine {randDev()};
        std::uniform_int_distribution<int> uniform_dist {0, total - 1};
        target = uniform_dist(engine);
    } while (currentGame && gameList.at(target) == currentGame);

    return gameList.at(target);
}

void SystemData::sortSystem(bool reloadGamelist, bool jumpToFirstRow)
{
    if (getName() == "recent")
        return;

    bool favoritesSorting {false};

    if (this->isCustomCollection() ||
        (this->isCollection() && this->getFullName() == "collections")) {
        favoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
    }
    else {
        favoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");
    }

    FileData* rootFolder {getRootFolder()};
    // Assign the sort type to all grouped custom collections.
    if (mIsCollectionSystem && mFullName == "collections") {
        for (auto it = rootFolder->getChildren().begin(); // Line break.
             it != rootFolder->getChildren().end(); ++it) {
            setupSystemSortType((*it)->getSystem()->getRootFolder());
        }
    }
    setupSystemSortType(rootFolder);

    rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                     favoritesSorting);

    if (reloadGamelist)
        ViewController::getInstance()->reloadGamelistView(this, false);

    if (jumpToFirstRow) {
        GamelistView* gameList {ViewController::getInstance()->getGamelistView(this).get()};
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

void SystemData::loadTheme(ThemeTriggers::TriggerType trigger)
{
    mTheme = std::make_shared<ThemeData>();

    const std::string& path {getThemePath()};

    if (!Utils::FileSystem::exists(path)) {
        // No theme available for this platform.
        if (!mIsCustomCollectionSystem) {
            LOG(LogWarning) << "There is no \"" << mThemeFolder
                            << "\" configuration available for the selected theme set \""
                            << Settings::getInstance()->getString("ThemeSet")
                            << "\", system will be unthemed";
        }
        return;
    }

    try {
        // Build a map with system variables for the theme to use. Assign a backspace character
        // to the variables that are not applicable. This will be used in ThemeData to make sure
        // unpopulated system variables do not lead to theme loading errors.
        std::map<std::string, std::string> sysData;
        sysData.insert(std::pair<std::string, std::string>("system.name", getName()));
        sysData.insert(std::pair<std::string, std::string>("system.theme", getThemeFolder()));
        sysData.insert(std::pair<std::string, std::string>("system.fullName", getFullName()));
        if (isCollection() && isCustomCollection()) {
            sysData.insert(
                std::pair<std::string, std::string>("system.name.customCollections", getName()));
            sysData.insert(std::pair<std::string, std::string>("system.fullName.customCollections",
                                                               getFullName()));
            sysData.insert(std::pair<std::string, std::string>("system.theme.customCollections",
                                                               getThemeFolder()));
            sysData.insert(
                std::pair<std::string, std::string>("system.name.autoCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.fullName.autoCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.theme.autoCollections", "\b"));
            sysData.insert(std::pair<std::string, std::string>("system.name.noCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.fullName.noCollections", "\b"));
            sysData.insert(std::pair<std::string, std::string>("system.theme.noCollections", "\b"));
        }
        else if (isCollection()) {
            sysData.insert(
                std::pair<std::string, std::string>("system.name.autoCollections", getName()));
            sysData.insert(std::pair<std::string, std::string>("system.fullName.autoCollections",
                                                               getFullName()));
            sysData.insert(std::pair<std::string, std::string>("system.theme.autoCollections",
                                                               getThemeFolder()));
            sysData.insert(
                std::pair<std::string, std::string>("system.name.customCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.fullName.customCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.theme.customCollections", "\b"));
            sysData.insert(std::pair<std::string, std::string>("system.name.noCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.fullName.noCollections", "\b"));
            sysData.insert(std::pair<std::string, std::string>("system.theme.noCollections", "\b"));
        }
        else {
            sysData.insert(
                std::pair<std::string, std::string>("system.name.noCollections", getName()));
            sysData.insert(std::pair<std::string, std::string>("system.fullName.noCollections",
                                                               getFullName()));
            sysData.insert(std::pair<std::string, std::string>("system.theme.noCollections",
                                                               getThemeFolder()));
            sysData.insert(
                std::pair<std::string, std::string>("system.name.autoCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.fullName.autoCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.theme.autoCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.name.customCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.fullName.customCollections", "\b"));
            sysData.insert(
                std::pair<std::string, std::string>("system.theme.customCollections", "\b"));
        }

        mTheme->loadFile(sysData, path, trigger, isCustomCollection());
    }
    catch (ThemeException& e) {
        LOG(LogError) << e.what() << " (system \"" << mName << "\", theme \"" << mThemeFolder
                      << "\")";
        mTheme = std::make_shared<ThemeData>(); // Reset to empty.
    }
}

void SystemData::writeMetaData()
{
    if (Settings::getInstance()->getBool("IgnoreGamelist") || mIsCollectionSystem)
        return;

    // Save changed game data back to xml.
    GamelistFileParser::updateGamelist(this);
}

void SystemData::onMetaDataSavePoint()
{
    if (Settings::getInstance()->getString("SaveGamelistsMode") != "always")
        return;

    writeMetaData();
}

void SystemData::setupSystemSortType(FileData* rootFolder)
{
    // If DefaultSortOrder is set to something, check that it is actually a valid value.
    if (Settings::getInstance()->getString("DefaultSortOrder") != "") {
        for (unsigned int i {0}; i < FileSorts::SortTypes.size(); ++i) {
            if (FileSorts::SortTypes.at(i).description ==
                Settings::getInstance()->getString("DefaultSortOrder")) {
                rootFolder->setSortTypeString(
                    Settings::getInstance()->getString("DefaultSortOrder"));
                break;
            }
        }
    }
    // If no valid sort type was defined in the configuration file, set to default sorting.
    if (rootFolder->getSortTypeString() == "")
        rootFolder->setSortTypeString(
            Settings::getInstance()->getDefaultString("DefaultSortOrder"));
}

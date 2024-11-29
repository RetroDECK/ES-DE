//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GamelistFileParser.cpp
//
//  Parses and updates the gamelist.xml files.
//

#include "GamelistFileParser.h"

#include "FileData.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <pugixml.hpp>

namespace GamelistFileParser
{
    FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type)
    {
        // First, verify that path is within the system's root folder.
        FileData* root {system->getRootFolder()};
        bool contains {false};
        std::string relative {Utils::FileSystem::removeCommonPath(path, root->getPath(), contains)};

        if (!contains) {
            LOG(LogError) << "Path \"" << path << "\" is outside system path \""
                          << system->getStartPath() << "\"";
            return nullptr;
        }

        const Utils::FileSystem::StringList& pathList {Utils::FileSystem::getPathList(relative)};
        auto path_it = pathList.begin();
        FileData* treeNode {root};
        bool found {false};

        while (path_it != pathList.end()) {
            // Workaround for an extremely rare issue that can basically only happen if a dot (.)
            // has been defined as a valid extension for the system (meaning extensionless files
            // are loaded), in combination with the "Only show ROMs from gamelist.xml files" option
            // being enabled and a stale entry being present in the gamelist.xml file that perfectly
            // matches a folder which is actually in use. The workaround is not a perfect solution
            // but it at least prevents the application from crashing.
            if (treeNode->getType() != FOLDER) {
                LOG(LogWarning)
                    << "Invalid gamelist entry caused by folder having the same name as a stale "
                    << "extensionless game file (this may cause undefined behavior):";
                return nullptr;
            }

            const std::unordered_map<std::string, FileData*>& children {
                treeNode->getChildrenByFilename()};

            const std::string key {*path_it};
            found = children.find(key) != children.cend();
            if (found)
                treeNode = children.at(key);

            if (treeNode->getNoLoad())
                return treeNode;

            // This is the end.
            if (path_it == --pathList.end()) {
                if (found)
                    return treeNode;

                if (type == FOLDER) {
                    if (!Utils::FileSystem::exists(path + "/noload.txt")) {
                        LOG(LogWarning) << "A folder defined in gamelist.xml does not exist or "
                                           "contains no valid games: \""
                                        << path << "\"";
                    }
                    return nullptr;
                }

                // Handle the special situation where a file exists and has an entry in the
                // gamelist.xml file but the file extension is not configured in es_systems.xml.
                const std::vector<std::string>& extensions {
                    system->getSystemEnvData()->mSearchExtensions};

                if (std::find(extensions.cbegin(), extensions.cend(),
                              Utils::FileSystem::getExtension(path)) == extensions.cend()) {
#if defined(_WIN64)
                    LOG(LogWarning) << "File \"" << Utils::String::replace(path, "/", "\\")
#else
                    LOG(LogWarning) << "File \"" << path
#endif
                                    << "\" is present in gamelist.xml but the extension is not "
                                       "configured in es_systems.xml";
                    // In case there are no entries left in the folder.
                    if (treeNode != system->getRootFolder() && treeNode->getChildren().empty())
                        delete treeNode;
                    return nullptr;
                }

                FileData* file {new FileData(type, path, system->getSystemEnvData(), system)};

                // Skipping arcade assets from gamelist.
                if (!file->isArcadeAsset())
                    treeNode->addChild(file);
                return file;
            }

            if (!found) {
                // Don't create folders unless they're including any games.
                // If the type is FOLDER it's going to be empty, so don't bother.
                if (type == FOLDER) {
                    LOG(LogWarning) << "A folder defined in gamelist.xml does not exist or "
                                       "contains no valid games: \""
                                    << path << "\"";
                    return nullptr;
                }

                if (!system->getFlattenFolders()) {
                    // Create missing folder.
                    FileData* folder {new FileData(FOLDER, treeNode->getPath() + "/" + *path_it,
                                                   system->getSystemEnvData(), system)};
                    treeNode->addChild(folder);
                    treeNode = folder;
                }
            }

            ++path_it;
        }

        return nullptr;
    }

    void parseGamelist(SystemData* system)
    {
        const bool trustGamelist {Settings::getInstance()->getBool("ParseGamelistOnly")};
        const std::string& xmlpath {system->getGamelistPath(false)};

        if (!Utils::FileSystem::exists(xmlpath)) {
            LOG(LogDebug) << "GamelistFileParser::parseGamelist(): System \"" << system->getName()
                          << "\" does not have a gamelist.xml file";
            // Get rid of any orphaned noload.txt folder entries.
            for (auto child : system->getRootFolder()->getChildrenRecursive()) {
                if (child->getNoLoad())
                    delete child;
            }
            return;
        }

        if (Utils::FileSystem::getFileSize(xmlpath) == 0) {
            LOG(LogWarning) << "GamelistFileParser::parseGamelist(): System \"" << system->getName()
                            << "\" has an empty gamelist.xml file";
            for (auto child : system->getRootFolder()->getChildrenRecursive()) {
                if (child->getNoLoad())
                    delete child;
            }
            return;
        }

#if defined(_WIN64)
        LOG(LogInfo) << "Parsing gamelist file \"" << Utils::String::replace(xmlpath, "/", "\\")
                     << "\"...";
#else
        LOG(LogInfo) << "Parsing gamelist file \"" << xmlpath << "\"...";
#endif

        pugi::xml_document doc;
#if defined(_WIN64)
        const pugi::xml_parse_result& result {
            doc.load_file(Utils::String::stringToWideString(xmlpath).c_str())};
#else
        const pugi::xml_parse_result& result {doc.load_file(xmlpath.c_str())};
#endif

        if (!result) {
            LOG(LogError) << "Error parsing gamelist file \"" << xmlpath
                          << "\": " << result.description();
            return;
        }

        const pugi::xml_node& root {doc.child("gameList")};
        if (!root) {
            LOG(LogError) << "Couldn't find <gameList> node in gamelist \"" << xmlpath << "\"";
            return;
        }

        const pugi::xml_node& alternativeEmulator {doc.child("alternativeEmulator")};
        if (alternativeEmulator) {
            const std::string& label {alternativeEmulator.child("label").text().get()};
            if (label != "") {
                bool validLabel {false};
                for (auto& command : system->getSystemEnvData()->mLaunchCommands) {
                    if (command.second == label)
                        validLabel = true;
                }
                if (validLabel) {
                    system->setAlternativeEmulator(label);
                    LOG(LogDebug) << "GamelistFileParser::parseGamelist(): System \""
                                  << system->getName()
                                  << "\" has a valid alternativeEmulator entry: \"" << label
                                  << "\"";
                }
                else {
                    system->setAlternativeEmulator("<INVALID>" + label);
                    LOG(LogWarning) << "System \"" << system->getName()
                                    << "\" has an invalid alternativeEmulator entry that does "
                                       "not match any command tag in es_systems.xml: \""
                                    << label << "\"";
                }
            }
        }

        const std::string& relativeTo {system->getStartPath()};
        const bool showHiddenFiles {Settings::getInstance()->getBool("ShowHiddenFiles")};

        const std::vector<std::string> tagList {"game", "folder"};
        const FileType typeList[2] = {GAME, FOLDER};

        for (int i {0}; i < 2; ++i) {
            std::string tag {tagList[i]};
            FileType type {typeList[i]};
            for (pugi::xml_node fileNode {root.child(tag.c_str())}; fileNode;
                 fileNode = fileNode.next_sibling(tag.c_str())) {
                const std::string& path {Utils::FileSystem::resolveRelativePath(
                    fileNode.child("path").text().get(), relativeTo, false)};

                if (!trustGamelist && !Utils::FileSystem::exists(path)) {
#if defined(_WIN64)
                    LOG(LogWarning) << (type == GAME ? "File \"" : "Folder \"")
                                    << Utils::String::replace(path, "/", "\\")
#else
                    LOG(LogWarning) << (type == GAME ? "File \"" : "Folder \"") << path
#endif
                                    << "\" does not exist, skipping entry";
                    continue;
                }

                // Skip hidden files, check both the file itself and the directory in which
                // it is located.
                if (!showHiddenFiles &&
                    (Utils::FileSystem::isHidden(path) ||
                     Utils::FileSystem::isHidden(Utils::FileSystem::getParent(path)))) {
                    LOG(LogDebug) << "GamelistFileParser::parseGamelist(): Skipping hidden file \""
                                  << path << "\"";
                    continue;
                }

                FileData* file {findOrCreateFile(system, path, type)};

                if (file != nullptr && file->getNoLoad())
                    continue;

                // Don't load entries with the wrong type. This should very rarely (if ever) happen.
                if (file != nullptr && ((tag == "game" && file->getType() == FOLDER) ||
                                        (tag == "folder" && file->getType() == GAME))) {
                    LOG(LogWarning)
                        << "Game/folder mismatch for \"" << path << "\", skipping entry";
                    continue;
                }

                if (!file) {
                    if (!Utils::FileSystem::exists(path + "/noload.txt")) {
#if defined(_WIN64)
                        LOG(LogWarning)
                            << "Couldn't process \"" << Utils::String::replace(path, "/", "\\")
                            << "\", skipping entry";
#else
                        LOG(LogWarning) << "Couldn't process \"" << path << "\", skipping entry";
#endif
                    }
                    continue;
                }
                else if (!file->isArcadeAsset()) {
                    const std::string defaultName {file->metadata.get("name")};
                    if (file->getType() == FOLDER) {
                        file->metadata =
                            MetaDataList::createFromXML(FOLDER_METADATA, fileNode, relativeTo);
                    }
                    else {
                        file->metadata =
                            MetaDataList::createFromXML(GAME_METADATA, fileNode, relativeTo);
                    }

                    // Make sure a name gets set if one doesn't exist.
                    if (file->metadata.get("name").empty())
                        file->metadata.set("name", defaultName);

                    file->metadata.resetChangedFlag();
                }
                else {
                    // Skip arcade asset entries as these will not be used in any way inside
                    // the application.
                    LOG(LogDebug) << "GamelistFileParser::parseGamelist(): Skipping arcade asset \""
                                  << file->getName() << "\"";
                    delete file;
                    continue;
                }
                // If the game is flagged as hidden and the option has not been set to show hidden
                // games, then delete the entry. This leaves no trace of the entry at all in ES
                // but that is fine as the option to show hidden files is defined as requiring an
                // application restart.
                if (!Settings::getInstance()->getBool("ShowHiddenGames")) {
                    if (file->getHidden()) {
                        LOG(LogDebug)
                            << "GamelistFileParser::parseGamelist(): Skipping hidden "
                            << (type == GAME ? "file" : "folder") << " entry \"" << file->getName()
                            << "\"" << " (\"" << file->getPath() << "\")";
                        FileData* parent {file->getParent()};
                        delete file;
                        // In case there are no entries left in the folder.
                        if (parent != system->getRootFolder() && parent->getChildren().empty())
                            delete parent;
                    }
                    // Also delete any folders which are empty, i.e. all their entries are hidden.
                    else if (file->getType() == FOLDER && file->getChildren().size() == 0) {
                        delete file;
                    }
                }
            }
            // Get rid of any orphaned noload.txt folder entries.
            for (auto child : system->getRootFolder()->getChildrenRecursive()) {
                if (child->getNoLoad())
                    delete child;
            }
        }
    }

    void addFileDataNode(pugi::xml_node& parent,
                         const FileData* file,
                         const std::string& tag,
                         SystemData* system)
    {
        // Create game and add to parent node.
        pugi::xml_node newNode {parent.append_child(tag.c_str())};

        // Write metadata.
        file->metadata.appendToXML(newNode, true, system->getStartPath());

        // First element is "name", there's only one element and the name is the default.
        if (newNode.children().begin() == newNode.child("name") &&
            ++newNode.children().begin() == newNode.children().end() &&
            newNode.child("name").text().get() == file->getDisplayName()) {

            // If the only info is the default name, don't bother
            // with this node, delete it and ultimately do nothing.
            parent.remove_child(newNode);
        }
        else {
            // There's something useful in there so we'll keep the node, add the path.

            // Try and make the path relative if we can so things still
            // work if we change the ROM folder location in the future.
            newNode.prepend_child("path").text().set(
                Utils::FileSystem::createRelativePath(file->getPath(), system->getStartPath(),
                                                      false)
                    .c_str());
        }
    }

    void updateGamelist(SystemData* system, bool updateAlternativeEmulator)
    {
        // We do this by reading the XML again, adding changes and then writing them back,
        // because there might be information missing in our systemdata which we would otherwise
        // miss in the new XML file. We have the complete information for every game though, so
        // we can simply remove a game we already have in the system from the XML, and then add
        // it back from its GameData information.
        if (Settings::getInstance()->getBool("IgnoreGamelist"))
            return;

        pugi::xml_document doc;
        pugi::xml_node root;
        const std::string& xmlReadPath {system->getGamelistPath(false)};
        bool hasAlternativeEmulatorTag {false};

        if (Utils::FileSystem::exists(xmlReadPath) &&
            Utils::FileSystem::getFileSize(xmlReadPath) != 0) {
            // Parse an existing file first.

#if defined(_WIN64)
            const pugi::xml_parse_result& result {
                doc.load_file(Utils::String::stringToWideString(xmlReadPath).c_str())};
#else
            const pugi::xml_parse_result& result {doc.load_file(xmlReadPath.c_str())};
#endif

            if (!result) {
                LOG(LogError) << "Error parsing gamelist file \"" << xmlReadPath
                              << "\": " << result.description();
                return;
            }

            root = doc.child("gameList");
            if (!root) {
                LOG(LogError) << "Couldn't find <gameList> node in gamelist \"" << xmlReadPath
                              << "\"";
                return;
            }
            if (updateAlternativeEmulator) {
                pugi::xml_node alternativeEmulator {doc.child("alternativeEmulator")};

                if (alternativeEmulator)
                    hasAlternativeEmulatorTag = true;

                if (system->getAlternativeEmulator() != "") {
                    if (!alternativeEmulator) {
                        doc.prepend_child("alternativeEmulator");
                        alternativeEmulator = doc.child("alternativeEmulator");
                    }

                    const pugi::xml_node& label {alternativeEmulator.child("label")};

                    if (label && system->getAlternativeEmulator() !=
                                     alternativeEmulator.child("label").text().get()) {
                        alternativeEmulator.remove_child(label);
                        alternativeEmulator.prepend_child("label").text().set(
                            system->getAlternativeEmulator().c_str());
                    }
                    else if (!label) {
                        alternativeEmulator.prepend_child("label").text().set(
                            system->getAlternativeEmulator().c_str());
                    }
                }
                else if (alternativeEmulator) {
                    doc.remove_child("alternativeEmulator");
                }
            }
        }
        else {
            if (updateAlternativeEmulator && system->getAlternativeEmulator() != "") {
                pugi::xml_node alternativeEmulator {doc.prepend_child("alternativeEmulator")};
                alternativeEmulator.prepend_child("label").text().set(
                    system->getAlternativeEmulator().c_str());
            }
            // Set up an empty gamelist to append to.
            root = doc.append_child("gameList");
        }

        // Now we have all the information from the XML file, so iterate
        // through all our games and add the information from there.
        FileData* rootFolder {system->getRootFolder()};
        if (rootFolder != nullptr) {
            int numUpdated {0};

            // Get only files, no folders.
            std::vector<FileData*> files {rootFolder->getFilesRecursive(GAME | FOLDER)};
            // Iterate through all files, checking if they're already in the XML file.
            for (std::vector<FileData*>::const_iterator fit {files.cbegin()}; // Line break.
                 fit != files.cend(); ++fit) {
                const std::string& tag {((*fit)->getType() == GAME) ? "game" : "folder"};

                // Do not touch if it wasn't changed and is not flagged for deletion.
                if (!(*fit)->metadata.wasChanged() && !(*fit)->getDeletionFlag())
                    continue;

                // Check if the file already exists in the XML file.
                // If it does, remove the entry before adding it back.
                for (pugi::xml_node fileNode {root.child(tag.c_str())}; fileNode;
                     fileNode = fileNode.next_sibling(tag.c_str())) {
                    const pugi::xml_node& pathNode {fileNode.child("path")};
                    if (!pathNode) {
                        LOG(LogError) << "<" << tag << "> node contains no <path> child";
                        continue;
                    }

                    const std::string& nodePath {
                        Utils::FileSystem::getCanonicalPath(Utils::FileSystem::resolveRelativePath(
                            pathNode.text().get(), system->getStartPath(), true))};
                    const std::string& gamePath {
                        Utils::FileSystem::getCanonicalPath((*fit)->getPath())};

                    if (nodePath == gamePath) {
                        // Found it
                        root.remove_child(fileNode);
                        if ((*fit)->getDeletionFlag())
                            ++numUpdated;
                        break;
                    }
                }

                // Add the game to the file, unless it's flagged for deletion.
                if (!(*fit)->getDeletionFlag()) {
                    addFileDataNode(root, *fit, tag, system);
                    (*fit)->metadata.resetChangedFlag();
                    ++numUpdated;
                }
            }

            // Now write the file.
            if (numUpdated > 0 || updateAlternativeEmulator) {
                // Make sure the folders leading up to this path exist (or the write will fail).
                const std::string& xmlWritePath {system->getGamelistPath(true)};
                Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

                if (updateAlternativeEmulator) {
                    if (hasAlternativeEmulatorTag && system->getAlternativeEmulator() == "") {
                        LOG(LogDebug) << "GamelistFileParser::updateGamelist(): Removed the "
                                         "alternativeEmulator tag for system \""
                                      << system->getName() << "\" as the default emulator \""
                                      << system->getSystemEnvData()->mLaunchCommands.front().second
                                      << "\" was selected";
                    }
                    else if (system->getAlternativeEmulator() != "") {
                        LOG(LogDebug) << "GamelistFileParser::updateGamelist(): "
                                         "Added/updated the alternativeEmulator tag for system \""
                                      << system->getName() << "\" to \""
                                      << system->getAlternativeEmulator() << "\"";
                    }
                }
                if (numUpdated > 0) {
                    LOG(LogDebug) << "GamelistFileParser::updateGamelist(): Added/updated "
                                  << numUpdated
                                  << (numUpdated == 1 ? " entity in \"" : " entities in \"")
#if defined(_WIN64)
                                  << Utils::String::replace(xmlWritePath, "/", "\\") << "\"";
#else
                                  << xmlWritePath << "\"";
#endif
                }
#if defined(_WIN64)
                if (!doc.save_file(Utils::String::stringToWideString(xmlWritePath).c_str())) {
#else
                if (!doc.save_file(xmlWritePath.c_str())) {
#endif
                    LOG(LogError) << "Error saving gamelist.xml to \"" << xmlWritePath
                                  << "\" (for system " << system->getName() << ")";
                }
            }
        }
        else {
            LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"";
        }
    }

} // namespace GamelistFileParser

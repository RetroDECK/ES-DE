//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Gamelist.cpp
//
//  Parses and updates the gamelist.xml files.
//

#include "Gamelist.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"

#include <chrono>
#include <pugixml.hpp>

FileData* findOrCreateFile(SystemData* system, const std::string& path, FileType type)
{
    // First, verify that path is within the system's root folder.
    FileData* root = system->getRootFolder();
    bool contains = false;
    std::string relative = Utils::FileSystem::removeCommonPath(path, root->getPath(), contains);

    if (!contains) {
        LOG(LogError) << "File path \"" << path << "\" is outside system path \"" <<
                system->getStartPath() << "\"";
        return nullptr;
    }

    Utils::FileSystem::stringList pathList = Utils::FileSystem::getPathList(relative);
    auto path_it = pathList.begin();
    FileData* treeNode = root;
    bool found = false;
    while (path_it != pathList.end()) {
        const std::unordered_map<std::string, FileData*>& children =
                treeNode->getChildrenByFilename();

        std::string key = *path_it;
        found = children.find(key) != children.cend();
        if (found) {
            treeNode = children.at(key);
        }

        // This is the end
        if (path_it == --pathList.end()) {
            if (found)
                return treeNode;

            if (type == FOLDER) {
                LOG(LogWarning) << "Gamelist: folder doesn't exist, won't create";
                return nullptr;
            }

            FileData* file = new FileData(type, path, system->getSystemEnvData(), system);

            // Skipping arcade assets from gamelist.
            if (!file->isArcadeAsset())
                treeNode->addChild(file);
            return file;
        }

        if (!found) {
            // Don't create folders unless they're including any games.
            // If the type is FOLDER it's going to be empty, so don't bother.
            if (type == FOLDER) {
                LOG(LogWarning) << "Gamelist: folder doesn't exist, won't create";
                return nullptr;
            }

            // Create missing folder.
            FileData* folder = new FileData(FOLDER, Utils::FileSystem::getStem(treeNode->getPath())
                    + "/" + *path_it, system->getSystemEnvData(), system);
            treeNode->addChild(folder);
            treeNode = folder;
        }

        path_it++;
    }

    return nullptr;
}

void parseGamelist(SystemData* system)
{
    bool trustGamelist = Settings::getInstance()->getBool("ParseGamelistOnly");
    std::string xmlpath = system->getGamelistPath(false);

    if (!Utils::FileSystem::exists(xmlpath))
        return;

    LOG(LogInfo) << "Parsing XML file \"" << xmlpath << "\"...";

    pugi::xml_document doc;
    #if defined(_WIN64)
    pugi::xml_parse_result result =
            doc.load_file(Utils::String::stringToWideString(xmlpath).c_str());
    #else
    pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());
    #endif

    if (!result) {
        LOG(LogError) << "Error parsing XML file \"" << xmlpath <<
                "\"!\n	" <<result.description();
        return;
    }

    pugi::xml_node root = doc.child("gameList");
    if (!root) {
        LOG(LogError) << "Could not find <gameList> node in gamelist \"" << xmlpath << "\"!";
        return;
    }

    std::string relativeTo = system->getStartPath();
    bool showHiddenFiles = Settings::getInstance()->getBool("ShowHiddenFiles");

    const char* tagList[2] = { "game", "folder" };
    FileType typeList[2] = { GAME, FOLDER };
    for (int i = 0; i < 2; i++) {
        const char* tag = tagList[i];
        FileType type = typeList[i];
        for (pugi::xml_node fileNode = root.child(tag); fileNode; fileNode =
                fileNode.next_sibling(tag)) {
            const std::string path =
                    Utils::FileSystem::resolveRelativePath(fileNode.child("path").text().get(),
                    relativeTo, false);

            if (!trustGamelist && !Utils::FileSystem::exists(path)) {
                LOG(LogWarning) << "File \"" << path << "\" does not exist, ignoring.";
                continue;
            }

            // Skip hidden files, check both the file itself and the directory in which
            // it is located.
            if (!showHiddenFiles && (Utils::FileSystem::isHidden(path) ||
                    Utils::FileSystem::isHidden(Utils::FileSystem::getParent(path)))) {
                LOG(LogDebug) << "Gamelist::parseGamelist(): Skipping hidden file " << path;
                continue;
            }

            FileData* file = findOrCreateFile(system, path, type);
            if (!file) {
                LOG(LogError) << "Error finding/creating FileData for \"" <<
                        path << "\", skipping.";
                continue;
            }
            else if (!file->isArcadeAsset()) {
                std::string defaultName = file->metadata.get("name");
                file->metadata = MetaDataList::createFromXML(GAME_METADATA, fileNode, relativeTo);

                // Make sure a name gets set if one doesn't exist.
                if (file->metadata.get("name").empty())
                    file->metadata.set("name", defaultName);

                file->metadata.resetChangedFlag();
            }
        }
    }
}

void addFileDataNode(pugi::xml_node& parent, const FileData* file,
                    const char* tag, SystemData* system)
{
    // Create game and add to parent node.
    pugi::xml_node newNode = parent.append_child(tag);

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
        newNode.prepend_child("path").text().set(Utils::FileSystem::createRelativePath(file->
                getPath(), system->getStartPath(), false).c_str());
    }
}

void updateGamelist(SystemData* system)
{
    // We do this by reading the XML again, adding changes and then writing them back,
    // because there might be information missing in our systemdata which we would otherwise
    // miss in the new XML file. We have the complete information for every game though, so
    // we can simply remove a game we already have in the system from the XML, and then add
    // it back from its GameData information...
    if (Settings::getInstance()->getBool("IgnoreGamelist"))
        return;

    pugi::xml_document doc;
    pugi::xml_node root;
    std::string xmlReadPath = system->getGamelistPath(false);

    if (Utils::FileSystem::exists(xmlReadPath)) {
        // Parse an existing file first.
        #if defined(_WIN64)
        pugi::xml_parse_result result =
                doc.load_file(Utils::String::stringToWideString(xmlReadPath).c_str());
        #else
        pugi::xml_parse_result result = doc.load_file(xmlReadPath.c_str());
        #endif

        if (!result) {
            LOG(LogError) << "Error parsing XML file \"" << xmlReadPath << "\"!\n	" <<
                    result.description();
            return;
        }

        root = doc.child("gameList");
        if (!root) {
            LOG(LogError) << "Could not find <gameList> node in gamelist \"" <<
                    xmlReadPath << "\"!";
            return;
        }
    }
    else {
        // Set up an empty gamelist to append to.
        root = doc.append_child("gameList");
    }

    // Now we have all the information from the XML file, so iterate
    // through all our games and add the information from there.
    FileData* rootFolder = system->getRootFolder();
    if (rootFolder != nullptr) {
        int numUpdated = 0;

        // Get only files, no folders.
        std::vector<FileData*> files = rootFolder->getFilesRecursive(GAME | FOLDER);
        // Iterate through all files, checking if they're already in the XML file.
        for (std::vector<FileData*>::const_iterator fit = files.cbegin();
                fit != files.cend(); ++fit) {
            const char* tag = ((*fit)->getType() == GAME) ? "game" : "folder";

            // Do not touch if it wasn't changed and is not flagged for deletion.
            if (!(*fit)->metadata.wasChanged() && !(*fit)->getDeletionFlag())
                continue;

            // Check if the file already exists in the XML file.
            // If it does, remove the entry before adding it back.
            for (pugi::xml_node fileNode = root.child(tag); fileNode;
                    fileNode = fileNode.next_sibling(tag)) {
                pugi::xml_node pathNode = fileNode.child("path");
                if (!pathNode) {
                    LOG(LogError) << "<" << tag << "> node contains no <path> child!";
                    continue;
                }

                std::string nodePath =Utils::FileSystem::getCanonicalPath(
                        Utils::FileSystem::resolveRelativePath(pathNode.text().get(),
                        system->getStartPath(), true));
                std::string gamePath = Utils::FileSystem::getCanonicalPath((*fit)->getPath());

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
        if (numUpdated > 0) {
            // Make sure the folders leading up to this path exist (or the write will fail).
            std::string xmlWritePath(system->getGamelistPath(true));
            Utils::FileSystem::createDirectory(Utils::FileSystem::getParent(xmlWritePath));

            LOG(LogDebug) << "Gamelist::updateGamelist(): Added/updated " << numUpdated <<
                    (numUpdated == 1 ? " entity in '" : " entities in '") << xmlReadPath << "'";

            #if defined(_WIN64)
            if (!doc.save_file(Utils::String::stringToWideString(xmlWritePath).c_str())) {
            #else
            if (!doc.save_file(xmlWritePath.c_str())) {
            #endif
                LOG(LogError) << "Error saving gamelist.xml to \"" <<
                        xmlWritePath << "\" (for system " << system->getName() << ")!";
            }
        }
    }
    else {
        LOG(LogError) << "Found no root folder for system \"" << system->getName() << "\"!";
    }
}

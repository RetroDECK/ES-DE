//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  MameNames.cpp
//
//  Provides expanded game names based on short MAME name arguments. Also contains
//  functions to check whether a passed argument is a MAME BIOS or a MAME device.
//  The data sources are stored in the .emulationstation/resources directory
//  as the files mamebioses.xml, mamedevices.xml and mamenames.xml.
//

#include "MameNames.h"

#include "Log.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"

#include <pugixml.hpp>
#include <string.h>

MameNames& MameNames::getInstance()
{
    static MameNames instance;
    return instance;
}

MameNames::MameNames()
{
    std::string xmlpath {ResourceManager::getInstance().getResourcePath(":/MAME/mamenames.xml")};

    if (!Utils::FileSystem::exists(xmlpath))
        return;

    pugi::xml_document doc;

#if defined(_WIN64)
    LOG(LogInfo) << "Parsing MAME names file \"" << Utils::String::replace(xmlpath, "/", "\\")
                 << "\"...";
    pugi::xml_parse_result result {
        doc.load_file(Utils::String::stringToWideString(xmlpath).c_str())};
#else
    LOG(LogInfo) << "Parsing MAME names file \"" << xmlpath << "\"...";
    pugi::xml_parse_result result {doc.load_file(xmlpath.c_str())};
#endif

    if (!result) {
        LOG(LogError) << "Error parsing MAME names file \"" << xmlpath
                      << "\": " << result.description();
        return;
    }

    for (pugi::xml_node gameNode = doc.child("game"); gameNode;
         gameNode = gameNode.next_sibling("game")) {
        mNamePairs[gameNode.child("mamename").text().get()] =
            gameNode.child("realname").text().get();
    }

    // Read BIOS file.
    xmlpath = ResourceManager::getInstance().getResourcePath(":/MAME/mamebioses.xml");

    if (!Utils::FileSystem::exists(xmlpath))
        return;

#if defined(_WIN64)
    LOG(LogInfo) << "Parsing MAME BIOSes file \"" << Utils::String::replace(xmlpath, "/", "\\")
                 << "\"...";
    result = doc.load_file(Utils::String::stringToWideString(xmlpath).c_str());
#else
    LOG(LogInfo) << "Parsing MAME BIOSes file \"" << xmlpath << "\"...";
    result = doc.load_file(xmlpath.c_str());
#endif

    if (!result) {
        LOG(LogError) << "Error parsing MAME BIOSes file \"" << xmlpath
                      << "\": " << result.description();
        return;
    }

    for (pugi::xml_node biosNode = doc.child("bios"); biosNode;
         biosNode = biosNode.next_sibling("bios")) {
        std::string bios = biosNode.text().get();
        mMameBioses.emplace_back(bios);
    }

    // Read device file.
    xmlpath = ResourceManager::getInstance().getResourcePath(":/MAME/mamedevices.xml");

    if (!Utils::FileSystem::exists(xmlpath))
        return;

#if defined(_WIN64)
    LOG(LogInfo) << "Parsing MAME devices file \"" << Utils::String::replace(xmlpath, "/", "\\")
                 << "\"...";
    result = doc.load_file(Utils::String::stringToWideString(xmlpath).c_str());
#else
    LOG(LogInfo) << "Parsing MAME devices file \"" << xmlpath << "\"...";
    result = doc.load_file(xmlpath.c_str());
#endif

    if (!result) {
        LOG(LogError) << "Error parsing MAME devices file \"" << xmlpath
                      << "\": " << result.description();
        return;
    }

    for (pugi::xml_node deviceNode = doc.child("device"); deviceNode;
         deviceNode = deviceNode.next_sibling("device")) {
        std::string device = deviceNode.text().get();
        mMameDevices.emplace_back(device);
    }
}

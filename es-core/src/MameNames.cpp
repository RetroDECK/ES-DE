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

#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "Log.h"

#include <pugixml.hpp>
#include <string.h>

MameNames* MameNames::sInstance = nullptr;

void MameNames::init()
{
    if (!sInstance)
        sInstance = new MameNames();
}

void MameNames::deinit()
{
    if (sInstance) {
        delete sInstance;
        sInstance = nullptr;
    }
}

MameNames* MameNames::getInstance()
{
    if (!sInstance)
        sInstance = new MameNames();

    return sInstance;
}

MameNames::MameNames()
{
    std::string xmlpath = ResourceManager::getInstance()->getResourcePath(":/MAME/mamenames.xml");

    if (!Utils::FileSystem::exists(xmlpath))
        return;

    LOG(LogInfo) << "Parsing MAME names file \"" << xmlpath << "\"...";

    pugi::xml_document doc;
    #if defined(_WIN64)
    pugi::xml_parse_result result =
            doc.load_file(Utils::String::stringToWideString(xmlpath).c_str());
    #else
    pugi::xml_parse_result result = doc.load_file(xmlpath.c_str());
    #endif

    if (!result) {
        LOG(LogError) << "Error parsing MAME names file \"" << xmlpath << "\": "
                << result.description();
        return;
    }

    for (pugi::xml_node gameNode = doc.child("game");
            gameNode; gameNode = gameNode.next_sibling("game")) {
        NamePair namePair = {
            gameNode.child("mamename").text().get(),
            gameNode.child("realname").text().get()
        };
        mNamePairs.push_back(namePair);
    }

    // Read BIOS file.
    xmlpath = ResourceManager::getInstance()->getResourcePath(":/MAME/mamebioses.xml");

    if (!Utils::FileSystem::exists(xmlpath))
        return;

    LOG(LogInfo) << "Parsing MAME BIOSes file \"" << xmlpath << "\"...";

    #if defined(_WIN64)
    result = doc.load_file(Utils::String::stringToWideString(xmlpath).c_str());
    #else
    result = doc.load_file(xmlpath.c_str());
    #endif

    if (!result) {
        LOG(LogError) << "Error parsing MAME BIOSes file \"" << xmlpath << "\": "
                << result.description();
        return;
    }

    for (pugi::xml_node biosNode = doc.child("bios");
            biosNode; biosNode = biosNode.next_sibling("bios")) {
        std::string bios = biosNode.text().get();
        mMameBioses.push_back(bios);
    }

    // Read device file.
    xmlpath = ResourceManager::getInstance()->getResourcePath(":/MAME/mamedevices.xml");

    if (!Utils::FileSystem::exists(xmlpath))
        return;

    LOG(LogInfo) << "Parsing MAME devices file \"" << xmlpath << "\"...";

    #if defined(_WIN64)
    result = doc.load_file(Utils::String::stringToWideString(xmlpath).c_str());
    #else
    result = doc.load_file(xmlpath.c_str());
    #endif

    if (!result) {
        LOG(LogError) << "Error parsing MAME devices file \"" << xmlpath << "\": "
                << result.description();
        return;
    }

    for (pugi::xml_node deviceNode = doc.child("device");
            deviceNode; deviceNode = deviceNode.next_sibling("device")) {
        std::string device = deviceNode.text().get();
        mMameDevices.push_back(device);
    }
}

MameNames::~MameNames()
{
}

std::string MameNames::getRealName(const std::string& _mameName)
{
    size_t start = 0;
    size_t end = mNamePairs.size();

    while (start < end) {
        const size_t index = (start + end) / 2;
        const int compare = strcmp(mNamePairs[index].mameName.c_str(), _mameName.c_str());

        if (compare < 0)
            start = index + 1;
        else if (compare > 0)
            end = index;
        else
            return mNamePairs[index].realName;
    }

    return _mameName;
}

std::string MameNames::getCleanName(const std::string& _mameName)
{
    std::string cleanName = Utils::String::removeParenthesis(getRealName(_mameName));
    return cleanName;
}

const bool MameNames::isBios(const std::string& _biosName)
{
    return MameNames::find(mMameBioses, _biosName);
}

const bool MameNames::isDevice(const std::string& _deviceName)
{
    return MameNames::find(mMameDevices, _deviceName);

}

const bool MameNames::find(std::vector<std::string> devices, const std::string& name)
{
    size_t start = 0;
    size_t end = devices.size();

    while (start < end) {
        if (!strcmp(devices[start].c_str(), name.c_str()))
            return true;
        const size_t index = (start + end) / 2;
        const int compare = strcmp(devices[index].c_str(), name.c_str());

        if (compare < 0)
            start = index + 1;
        else if (compare > 0)
            end = index;
        else
            return true;
    }
    return false;
}

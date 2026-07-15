//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMPlatformMapping.cpp
//

#include "RomM/RomMPlatformMapping.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <pugixml.hpp>

namespace
{
    std::string getFilePath()
    {
        return Utils::FileSystem::getAppDataDirectory() + "/settings/romm_mapping.xml";
    }
} // namespace

RomMPlatformMapping& RomMPlatformMapping::getInstance()
{
    static RomMPlatformMapping instance;
    return instance;
}

RomMPlatformMapping::RomMPlatformMapping() { loadFile(); }

const RomMSystemMapping* RomMPlatformMapping::getMapping(const std::string& systemName) const
{
    if (systemName.empty())
        return nullptr;
    for (const auto& mapping : mMappings) {
        if (mapping.systemName == systemName)
            return &mapping;
    }
    return nullptr;
}

const RomMSystemMapping* RomMPlatformMapping::findByPlatformId(int rommPlatformId) const
{
    if (rommPlatformId < 0)
        return nullptr;
    for (const auto& mapping : mMappings) {
        if (mapping.rommPlatformId == rommPlatformId)
            return &mapping;
    }
    return nullptr;
}

void RomMPlatformMapping::setMapping(const RomMSystemMapping& mapping)
{
    if (mapping.rommPlatformId < 0)
        return;

    // Enforce a one-to-one relationship between RomM platforms and ES-DE systems: clear any
    // other platform mapping that currently claims the same system name.
    if (!mapping.systemName.empty()) {
        for (auto& existing : mMappings) {
            if (existing.rommPlatformId != mapping.rommPlatformId &&
                existing.systemName == mapping.systemName) {
                existing.systemName.clear();
            }
        }
    }

    for (auto& existing : mMappings) {
        if (existing.rommPlatformId == mapping.rommPlatformId) {
            existing = mapping;
            saveFile();
            return;
        }
    }
    mMappings.emplace_back(mapping);
    saveFile();
}

void RomMPlatformMapping::setSystemNameForPlatform(int rommPlatformId,
                                                   const std::string& systemName)
{
    for (auto& existing : mMappings) {
        if (existing.rommPlatformId == rommPlatformId) {
            existing.systemName = systemName;
            saveFile();
            return;
        }
    }
}

void RomMPlatformMapping::loadFile()
{
    mMappings.clear();

    const std::string path {getFilePath()};
    if (!Utils::FileSystem::exists(path))
        return;

    pugi::xml_document doc;
#if defined(_WIN64)
    pugi::xml_parse_result result {doc.load_file(Utils::String::stringToWideString(path).c_str())};
#else
    pugi::xml_parse_result result {doc.load_file(path.c_str())};
#endif
    if (!result) {
        LOG(LogError) << "Couldn't parse the romm_mapping.xml file: " << result.description();
        return;
    }

    for (pugi::xml_node node {doc.child("platform")}; node; node = node.next_sibling("platform")) {
        RomMSystemMapping mapping;
        mapping.rommPlatformId = node.attribute("rommPlatformId").as_int(-1);
        mapping.platformSlug = node.attribute("platformSlug").as_string();
        mapping.platformFsSlug = node.attribute("platformFsSlug").as_string();
        mapping.systemName = node.attribute("systemName").as_string();
        mapping.syncEnabled = node.attribute("syncEnabled").as_bool();
        if (mapping.rommPlatformId >= 0)
            mMappings.emplace_back(mapping);
    }
}

void RomMPlatformMapping::saveFile()
{
    const std::string path {getFilePath()};

    pugi::xml_document doc;
    for (const auto& mapping : mMappings) {
        pugi::xml_node node {doc.append_child("platform")};
        node.append_attribute("rommPlatformId").set_value(mapping.rommPlatformId);
        node.append_attribute("platformSlug").set_value(mapping.platformSlug.c_str());
        node.append_attribute("platformFsSlug").set_value(mapping.platformFsSlug.c_str());
        node.append_attribute("systemName").set_value(mapping.systemName.c_str());
        node.append_attribute("syncEnabled").set_value(mapping.syncEnabled);
    }

#if defined(_WIN64)
    doc.save_file(Utils::String::stringToWideString(path).c_str());
#else
    doc.save_file(path.c_str());
#endif
}

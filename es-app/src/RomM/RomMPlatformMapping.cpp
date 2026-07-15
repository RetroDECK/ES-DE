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
    for (const auto& mapping : mMappings) {
        if (mapping.systemName == systemName)
            return &mapping;
    }
    return nullptr;
}

void RomMPlatformMapping::setMapping(const RomMSystemMapping& mapping)
{
    for (auto& existing : mMappings) {
        if (existing.systemName == mapping.systemName) {
            existing = mapping;
            saveFile();
            return;
        }
    }
    mMappings.emplace_back(mapping);
    saveFile();
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

    for (pugi::xml_node node {doc.child("system")}; node; node = node.next_sibling("system")) {
        RomMSystemMapping mapping;
        mapping.systemName = node.attribute("name").as_string();
        mapping.syncEnabled = node.attribute("syncEnabled").as_bool();
        mapping.rommPlatformId = node.attribute("rommPlatformId").as_int(-1);
        if (!mapping.systemName.empty())
            mMappings.emplace_back(mapping);
    }
}

void RomMPlatformMapping::saveFile()
{
    const std::string path {getFilePath()};

    pugi::xml_document doc;
    for (const auto& mapping : mMappings) {
        pugi::xml_node node {doc.append_child("system")};
        node.append_attribute("name").set_value(mapping.systemName.c_str());
        node.append_attribute("syncEnabled").set_value(mapping.syncEnabled);
        node.append_attribute("rommPlatformId").set_value(mapping.rommPlatformId);
    }

#if defined(_WIN64)
    doc.save_file(Utils::String::stringToWideString(path).c_str());
#else
    doc.save_file(path.c_str());
#endif
}

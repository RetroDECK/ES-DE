//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMCache.cpp
//

#include "RomM/RomMCache.h"

#include "Log.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include <pugixml.hpp>

namespace
{
    std::string getFilePath()
    {
        return Utils::FileSystem::getAppDataDirectory() + "/settings/romm_cache.xml";
    }

    std::string joinComma(const std::vector<std::string>& items)
    {
        std::string result;
        for (const auto& item : items)
            result += (result.empty() ? "" : ",") + item;
        return result;
    }

    // Utils::String::delimitedStringToVector("", ",") returns a single empty-string element,
    // not an empty vector - guard explicitly, or an empty regions/languages attribute would
    // leak a bogus empty entry into buildVariantTag()'s "!rom.regions.empty()" check.
    std::vector<std::string> splitComma(const std::string& value)
    {
        if (value.empty())
            return {};
        return Utils::String::delimitedStringToVector(value, ",");
    }
} // namespace

RomMCache& RomMCache::getInstance()
{
    static RomMCache instance;
    return instance;
}

RomMCache::RomMCache() { loadFile(); }

std::string RomMCache::getCursor(int rommPlatformId) const
{
    const auto it {mPlatforms.find(rommPlatformId)};
    return it == mPlatforms.cend() ? std::string() : it->second.cursor;
}

std::vector<RomMCache::CachedRom> RomMCache::getRoms(int rommPlatformId) const
{
    const auto it {mPlatforms.find(rommPlatformId)};
    return it == mPlatforms.cend() ? std::vector<CachedRom>() : it->second.roms;
}

bool RomMCache::findCachedSize(int rommId, int64_t& sizeBytesOut) const
{
    CachedRom cached;
    if (!findCachedRom(rommId, cached))
        return false;
    sizeBytesOut = cached.fsSizeBytes;
    return true;
}

bool RomMCache::findCachedRom(int rommId, CachedRom& cachedOut) const
{
    for (const auto& [rommPlatformId, entry] : mPlatforms) {
        for (const auto& rom : entry.roms) {
            if (rom.id == rommId) {
                cachedOut = rom;
                return true;
            }
        }
    }
    return false;
}

void RomMCache::setPlatform(int rommPlatformId,
                            const std::string& cursor,
                            const std::vector<CachedRom>& roms)
{
    PlatformCache& entry {mPlatforms[rommPlatformId]};
    entry.cursor = cursor;
    entry.roms = roms;
}

void RomMCache::clearAll() { mPlatforms.clear(); }

RomMCache::CachedRom RomMCache::fromApiRom(const RomMApiClient::Rom& rom)
{
    CachedRom cached;
    cached.id = rom.id;
    cached.name = rom.name;
    cached.summary = rom.summary;
    cached.fsName = rom.fsName;
    cached.fsSizeBytes = rom.fsSizeBytes;
    cached.revision = rom.revision;
    cached.regions = rom.regions;
    cached.languages = rom.languages;
    cached.hasMultipleFiles = rom.hasMultipleFiles;
    cached.lastPlayed = rom.lastPlayed;
    cached.userHidden = rom.userHidden;
    cached.userRating = rom.userRating;
    cached.userStatus = rom.userStatus;
    cached.genres = rom.genres;
    cached.companies = rom.companies;
    cached.firstReleaseDate = rom.firstReleaseDate;
    cached.averageRating = rom.averageRating;
    cached.playerCount = rom.playerCount;
    return cached;
}

RomMApiClient::Rom RomMCache::toApiRom(const CachedRom& cached)
{
    RomMApiClient::Rom rom;
    rom.id = cached.id;
    rom.name = cached.name;
    rom.summary = cached.summary;
    rom.fsName = cached.fsName;
    rom.fsSizeBytes = cached.fsSizeBytes;
    rom.revision = cached.revision;
    rom.regions = cached.regions;
    rom.languages = cached.languages;
    rom.hasMultipleFiles = cached.hasMultipleFiles;
    rom.lastPlayed = cached.lastPlayed;
    rom.userHidden = cached.userHidden;
    rom.userRating = cached.userRating;
    rom.userStatus = cached.userStatus;
    rom.genres = cached.genres;
    rom.companies = cached.companies;
    rom.firstReleaseDate = cached.firstReleaseDate;
    rom.averageRating = cached.averageRating;
    rom.playerCount = cached.playerCount;
    // urlCover/files/updatedAt stay at their defaults - applyResults()/buildDisplayNames()
    // never read them.
    return rom;
}

void RomMCache::loadFile()
{
    mPlatforms.clear();

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
        LOG(LogError) << "Couldn't parse the romm_cache.xml file: " << result.description()
                      << " - every platform will be treated as never synced this run";
        return;
    }

    for (pugi::xml_node platformNode {doc.child("platform")}; platformNode;
         platformNode = platformNode.next_sibling("platform")) {
        const int rommPlatformId {platformNode.attribute("rommPlatformId").as_int(-1)};
        if (rommPlatformId < 0)
            continue;

        PlatformCache entry;
        entry.cursor = platformNode.attribute("cursor").as_string();

        for (pugi::xml_node romNode {platformNode.child("rom")}; romNode;
             romNode = romNode.next_sibling("rom")) {
            const int romId {romNode.attribute("id").as_int(-1)};
            if (romId < 0)
                continue;
            CachedRom rom;
            rom.id = romId;
            rom.name = romNode.attribute("name").as_string();
            rom.summary = romNode.attribute("summary").as_string();
            rom.fsName = romNode.attribute("fsName").as_string();
            rom.fsSizeBytes = romNode.attribute("fsSizeBytes").as_llong();
            rom.revision = romNode.attribute("revision").as_string();
            rom.regions = splitComma(romNode.attribute("regions").as_string());
            rom.languages = splitComma(romNode.attribute("languages").as_string());
            rom.hasMultipleFiles = romNode.attribute("hasMultipleFiles").as_bool();
            rom.lastPlayed = romNode.attribute("lastPlayed").as_llong();
            rom.userHidden = romNode.attribute("userHidden").as_bool();
            rom.userRating = romNode.attribute("userRating").as_int();
            rom.userStatus = romNode.attribute("userStatus").as_string();
            rom.genres = splitComma(romNode.attribute("genres").as_string());
            rom.companies = splitComma(romNode.attribute("companies").as_string());
            rom.firstReleaseDate = romNode.attribute("firstReleaseDate").as_llong();
            rom.averageRating = romNode.attribute("averageRating").as_float();
            rom.playerCount = romNode.attribute("playerCount").as_string();
            entry.roms.emplace_back(std::move(rom));
        }

        mPlatforms.emplace(rommPlatformId, std::move(entry));
    }
}

void RomMCache::flush()
{
    const std::string path {getFilePath()};

    pugi::xml_document doc;
    for (const auto& [rommPlatformId, entry] : mPlatforms) {
        pugi::xml_node platformNode {doc.append_child("platform")};
        platformNode.append_attribute("rommPlatformId").set_value(rommPlatformId);
        platformNode.append_attribute("cursor").set_value(entry.cursor.c_str());
        for (const auto& rom : entry.roms) {
            pugi::xml_node romNode {platformNode.append_child("rom")};
            romNode.append_attribute("id").set_value(rom.id);
            romNode.append_attribute("name").set_value(rom.name.c_str());
            romNode.append_attribute("summary").set_value(rom.summary.c_str());
            romNode.append_attribute("fsName").set_value(rom.fsName.c_str());
            romNode.append_attribute("fsSizeBytes")
                .set_value(static_cast<long long>(rom.fsSizeBytes));
            romNode.append_attribute("revision").set_value(rom.revision.c_str());
            romNode.append_attribute("regions").set_value(joinComma(rom.regions).c_str());
            romNode.append_attribute("languages").set_value(joinComma(rom.languages).c_str());
            romNode.append_attribute("hasMultipleFiles").set_value(rom.hasMultipleFiles);
            romNode.append_attribute("lastPlayed").set_value(static_cast<long long>(rom.lastPlayed));
            romNode.append_attribute("userHidden").set_value(rom.userHidden);
            romNode.append_attribute("userRating").set_value(rom.userRating);
            romNode.append_attribute("userStatus").set_value(rom.userStatus.c_str());
            romNode.append_attribute("genres").set_value(joinComma(rom.genres).c_str());
            romNode.append_attribute("companies").set_value(joinComma(rom.companies).c_str());
            romNode.append_attribute("firstReleaseDate")
                .set_value(static_cast<long long>(rom.firstReleaseDate));
            romNode.append_attribute("averageRating").set_value(rom.averageRating);
            romNode.append_attribute("playerCount").set_value(rom.playerCount.c_str());
        }
    }

#if defined(_WIN64)
    doc.save_file(Utils::String::stringToWideString(path).c_str());
#else
    doc.save_file(path.c_str());
#endif
}

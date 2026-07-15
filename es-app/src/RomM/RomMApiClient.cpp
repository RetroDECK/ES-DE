//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMApiClient.cpp
//

#include "RomM/RomMApiClient.h"

#include "HttpReq.h"
#include "Log.h"
#include "utils/StringUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <chrono>
#include <thread>

using namespace rapidjson;

namespace
{
    constexpr int MAX_WAIT_MS {30000};
    constexpr int POLL_TIME_MS {50};

    // Known mismatches between ES-DE's platform names (PlatformIds::platformNames) and RomM's
    // filesystem-friendly platform slugs. Left side is the ES-DE name, right side the RomM
    // fs_slug/slug it should be considered equivalent to.
    const std::vector<std::pair<std::string, std::string>> platformAliases {
        {"genesis", "genesis-slash-megadrive"},
        {"megadrive", "genesis-slash-megadrive"},
        {"snes", "snes"},
        {"n64", "n64"},
        {"psx", "ps"},
        {"ps2", "ps2"},
        {"nes", "nes"},
        {"famicom", "nes"},
        {"gb", "gb"},
        {"gbc", "gbc"},
        {"gba", "gba"},
        {"segacd", "segacd"},
        {"sega32x", "sega32"},
        {"mastersystem", "sms"},
        {"gamegear", "gg"},
        {"atari2600", "atari2600"},
        {"arcade", "arcade"},
        {"dreamcast", "dc"},
        {"saturn", "saturn"},
        {"neogeo", "neogeoaes"},
        {"3do", "3do"},
        {"pcengine", "turbografx-16-slash-pc-engine"},
    };
} // namespace

RomMApiClient::RomMApiClient(const std::string& serverURL, const std::string& token)
    : mServerURL {serverURL}
    , mToken {token}
{
    // Strip any trailing slash so buildUrl() can always just append "/api/...".
    while (!mServerURL.empty() && mServerURL.back() == '/')
        mServerURL.pop_back();
}

std::string RomMApiClient::buildUrl(const std::string& path) const { return mServerURL + path; }

bool RomMApiClient::waitForRequest(HttpReq& req)
{
    const int maxIterations {MAX_WAIT_MS / POLL_TIME_MS};
    for (int i {0}; i < maxIterations; ++i) {
        const HttpReq::Status status {req.status()};
        if (status == HttpReq::REQ_SUCCESS)
            return true;
        if (status != HttpReq::REQ_IN_PROGRESS) {
            mLastError = req.getErrorMsg();
            if (mLastError.empty())
                mLastError = "RomM request failed";
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLL_TIME_MS));
    }
    mLastError = "RomM request timed out";
    return false;
}

bool RomMApiClient::testConnection()
{
    if (mServerURL.empty()) {
        mLastError = "No RomM server URL configured";
        return false;
    }

    HttpReq req {buildUrl("/api/platforms"), false, "", "", "", mToken};
    return waitForRequest(req);
}

std::vector<RomMApiClient::Platform> RomMApiClient::fetchPlatforms()
{
    std::vector<Platform> platforms;

    if (mServerURL.empty()) {
        mLastError = "No RomM server URL configured";
        return platforms;
    }

    HttpReq req {buildUrl("/api/platforms"), false, "", "", "", mToken};
    if (!waitForRequest(req))
        return platforms;

    Document doc;
    doc.Parse(req.getContent().c_str());
    if (doc.HasParseError()) {
        mLastError = std::string("Error parsing RomM platforms response: ") +
                     GetParseError_En(doc.GetParseError());
        LOG(LogError) << mLastError;
        return platforms;
    }
    if (!doc.IsArray()) {
        mLastError = "Unexpected RomM platforms response format";
        LOG(LogError) << mLastError;
        return platforms;
    }

    for (const auto& entry : doc.GetArray()) {
        if (!entry.IsObject() || !entry.HasMember("id") || !entry["id"].IsInt())
            continue;

        Platform platform;
        platform.id = entry["id"].GetInt();
        if (entry.HasMember("slug") && entry["slug"].IsString())
            platform.slug = entry["slug"].GetString();
        if (entry.HasMember("fs_slug") && entry["fs_slug"].IsString())
            platform.fsSlug = entry["fs_slug"].GetString();
        if (entry.HasMember("name") && entry["name"].IsString())
            platform.name = entry["name"].GetString();
        if (entry.HasMember("rom_count") && entry["rom_count"].IsInt())
            platform.romCount = entry["rom_count"].GetInt();
        platforms.emplace_back(platform);
    }

    return platforms;
}

namespace
{
    RomMApiClient::Rom parseRom(const rapidjson::Value& entry)
    {
        RomMApiClient::Rom rom;

        if (entry.HasMember("id") && entry["id"].IsInt())
            rom.id = entry["id"].GetInt();
        if (entry.HasMember("name") && entry["name"].IsString())
            rom.name = entry["name"].GetString();
        if (entry.HasMember("summary") && entry["summary"].IsString())
            rom.summary = entry["summary"].GetString();
        if (entry.HasMember("fs_name") && entry["fs_name"].IsString())
            rom.fsName = entry["fs_name"].GetString();
        if (entry.HasMember("fs_size_bytes") && entry["fs_size_bytes"].IsInt64())
            rom.fsSizeBytes = entry["fs_size_bytes"].GetInt64();
        if (entry.HasMember("url_cover") && entry["url_cover"].IsString())
            rom.urlCover = entry["url_cover"].GetString();

        if (entry.HasMember("metadatum") && entry["metadatum"].IsObject()) {
            const auto& metadatum = entry["metadatum"];
            // first_release_date is a Unix timestamp (seconds since epoch), not a string.
            if (metadatum.HasMember("first_release_date") &&
                metadatum["first_release_date"].IsInt64())
                rom.firstReleaseDate = metadatum["first_release_date"].GetInt64();
            if (metadatum.HasMember("average_rating") && metadatum["average_rating"].IsNumber())
                rom.averageRating = metadatum["average_rating"].GetFloat();
            if (metadatum.HasMember("genres") && metadatum["genres"].IsArray()) {
                for (const auto& genre : metadatum["genres"].GetArray()) {
                    if (genre.IsString())
                        rom.genres.emplace_back(genre.GetString());
                }
            }
            if (metadatum.HasMember("companies") && metadatum["companies"].IsArray()) {
                for (const auto& company : metadatum["companies"].GetArray()) {
                    if (company.IsString())
                        rom.companies.emplace_back(company.GetString());
                }
            }
            // player_count is a free-form string in RomM's API, e.g. "1-4", not an integer.
            if (metadatum.HasMember("player_count") && metadatum["player_count"].IsString())
                rom.playerCount = metadatum["player_count"].GetString();
        }

        return rom;
    }
} // namespace

std::vector<RomMApiClient::Rom> RomMApiClient::fetchRoms(int platformId)
{
    std::vector<Rom> roms;

    if (mServerURL.empty()) {
        mLastError = "No RomM server URL configured";
        return roms;
    }

    constexpr int pageSize {250};
    int offset {0};
    int total {0};

    do {
        const std::string url {buildUrl("/api/roms?platform_ids=" + std::to_string(platformId) +
                                        "&limit=" + std::to_string(pageSize) +
                                        "&offset=" + std::to_string(offset))};
        HttpReq req {url, false, "", "", "", mToken};
        if (!waitForRequest(req))
            return roms;

        Document doc;
        doc.Parse(req.getContent().c_str());
        if (doc.HasParseError()) {
            mLastError = std::string("Error parsing RomM roms response: ") +
                         GetParseError_En(doc.GetParseError());
            LOG(LogError) << mLastError;
            return roms;
        }
        if (!doc.IsObject() || !doc.HasMember("items") || !doc["items"].IsArray()) {
            mLastError = "Unexpected RomM roms response format";
            LOG(LogError) << mLastError;
            return roms;
        }

        for (const auto& entry : doc["items"].GetArray())
            roms.emplace_back(parseRom(entry));

        total = (doc.HasMember("total") && doc["total"].IsInt()) ? doc["total"].GetInt() :
                                                                   static_cast<int>(roms.size());
        offset += pageSize;
    } while (offset < total);

    return roms;
}

bool RomMApiClient::fetchRomByHash(const std::string& md5Hash, Rom& outRom)
{
    if (mServerURL.empty()) {
        mLastError = "No RomM server URL configured";
        return false;
    }

    const std::string url {buildUrl("/api/roms/by-hash?md5_hash=" + md5Hash)};
    HttpReq req {url, false, "", "", "", mToken};
    if (!waitForRequest(req))
        return false;

    Document doc;
    doc.Parse(req.getContent().c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        mLastError = "Unexpected RomM rom-by-hash response format";
        return false;
    }
    if (!doc.HasMember("id") || !doc["id"].IsInt())
        return false;

    outRom = parseRom(doc);
    return true;
}

bool RomMApiClient::platformNameMatches(const std::string& esdePlatformName,
                                        const std::string& rommSlug,
                                        const std::string& rommFsSlug)
{
    const std::string esdeLower {Utils::String::toLower(esdePlatformName)};
    const std::string slugLower {Utils::String::toLower(rommSlug)};
    const std::string fsSlugLower {Utils::String::toLower(rommFsSlug)};

    if (esdeLower == slugLower || esdeLower == fsSlugLower)
        return true;

    for (const auto& alias : platformAliases) {
        if (alias.first == esdeLower && (alias.second == slugLower || alias.second == fsSlugLower))
            return true;
    }

    return false;
}

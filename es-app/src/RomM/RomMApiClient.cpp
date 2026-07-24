//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMApiClient.cpp
//

#include "RomM/RomMApiClient.h"

#include "HttpReq.h"
#include "Log.h"
#include "RomM/RomMUtils.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <chrono>
#include <cstdio>
#include <thread>

using namespace rapidjson;

namespace
{
    constexpr int POLL_TIME_MS {50};
    constexpr int PLATFORM_FETCH_MAX_WAIT_MS {5000};
} // namespace

RomMApiClient::RomMApiClient(const std::string& serverURL, const std::string& token)
    : mServerURL {serverURL}
    , mToken {token}
{
    // Strip any trailing slash so buildUrl() can always just append "/api/...".
    while (!mServerURL.empty() && mServerURL.back() == '/')
        mServerURL.pop_back();
}

std::string RomMApiClient::buildUrl(const std::string& path) const
{
    return RomMUtils::joinUrl(mServerURL, path);
}

bool RomMApiClient::waitForRequest(HttpReq& req, int maxWaitMs)
{
    const int maxIterations {maxWaitMs / POLL_TIME_MS};
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

    HttpReq req {buildUrl("/api/platforms"), false, "", mToken};
    return waitForRequest(req);
}

std::vector<RomMApiClient::Platform> RomMApiClient::fetchPlatforms()
{
    std::vector<Platform> platforms;

    if (mServerURL.empty()) {
        mLastError = "No RomM server URL configured";
        return platforms;
    }

    HttpReq req {buildUrl("/api/platforms"), false, "", mToken};
    if (!waitForRequest(req, PLATFORM_FETCH_MAX_WAIT_MS))
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
        platforms.emplace_back(platform);
    }

    return platforms;
}

namespace
{
    // Parses the fixed "YYYY-MM-DDTHH:MM:SS" prefix of an ISO-8601 timestamp as returned by
    // RomM's "updated_at"/"created_at" fields (e.g. "2026-07-10T22:03:33.794021+00:00", always
    // UTC on a live instance) into Unix seconds. Ignores any fractional-seconds/offset suffix -
    // this field is diagnostic-only (see Rom::updatedAt), never correctness-sensitive. Returns
    // 0 on any parse failure.
    int64_t parseIso8601ToUnixSeconds(const std::string& value)
    {
        tm parsedTime {};
        if (sscanf(value.c_str(), "%d-%d-%dT%d:%d:%d", &parsedTime.tm_year, &parsedTime.tm_mon,
                   &parsedTime.tm_mday, &parsedTime.tm_hour, &parsedTime.tm_min,
                   &parsedTime.tm_sec) != 6)
            return 0;

        parsedTime.tm_year -= 1900;
        parsedTime.tm_mon -= 1;

#if defined(_WIN64)
        return static_cast<int64_t>(_mkgmtime(&parsedTime));
#else
        return static_cast<int64_t>(timegm(&parsedTime));
#endif
    }

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
        // revision/regions/languages are top-level fields on RomM's rom schema (not nested
        // under metadatum) - RomM parses them out of the filename itself server-side.
        if (entry.HasMember("revision") && entry["revision"].IsString())
            rom.revision = entry["revision"].GetString();
        if (entry.HasMember("regions") && entry["regions"].IsArray()) {
            for (const auto& region : entry["regions"].GetArray()) {
                if (region.IsString())
                    rom.regions.emplace_back(region.GetString());
            }
        }
        if (entry.HasMember("languages") && entry["languages"].IsArray()) {
            for (const auto& language : entry["languages"].GetArray()) {
                if (language.IsString())
                    rom.languages.emplace_back(language.GetString());
            }
        }
        if (entry.HasMember("has_multiple_files") && entry["has_multiple_files"].IsBool())
            rom.hasMultipleFiles = entry["has_multiple_files"].GetBool();
        if (entry.HasMember("files") && entry["files"].IsArray()) {
            for (const auto& fileEntry : entry["files"].GetArray()) {
                if (!fileEntry.IsObject())
                    continue;
                RomMApiClient::RomFile file;
                if (fileEntry.HasMember("id") && fileEntry["id"].IsInt())
                    file.id = fileEntry["id"].GetInt();
                if (fileEntry.HasMember("file_name") && fileEntry["file_name"].IsString())
                    file.fileName = fileEntry["file_name"].GetString();
                if (fileEntry.HasMember("file_size_bytes") &&
                    fileEntry["file_size_bytes"].IsInt64())
                    file.sizeBytes = fileEntry["file_size_bytes"].GetInt64();
                if (fileEntry.HasMember("category") && fileEntry["category"].IsString())
                    file.category = fileEntry["category"].GetString();
                rom.files.emplace_back(file);
            }
        }
        if (entry.HasMember("updated_at") && entry["updated_at"].IsString())
            rom.updatedAt = parseIso8601ToUnixSeconds(entry["updated_at"].GetString());

        if (entry.HasMember("rom_user") && entry["rom_user"].IsObject()) {
            const auto& romUser = entry["rom_user"];
            if (romUser.HasMember("last_played") && romUser["last_played"].IsString())
                rom.lastPlayed = parseIso8601ToUnixSeconds(romUser["last_played"].GetString());
            if (romUser.HasMember("hidden") && romUser["hidden"].IsBool())
                rom.userHidden = romUser["hidden"].GetBool();
            if (romUser.HasMember("rating") && romUser["rating"].IsInt())
                rom.userRating = romUser["rating"].GetInt();
            if (romUser.HasMember("status") && romUser["status"].IsString())
                rom.userStatus = romUser["status"].GetString();
        }

        if (entry.HasMember("metadatum") && entry["metadatum"].IsObject()) {
            const auto& metadatum = entry["metadatum"];
            // first_release_date is a Unix timestamp in milliseconds since epoch, not a string -
            // normalized to seconds here to match Rom::firstReleaseDate's documented meaning.
            if (metadatum.HasMember("first_release_date") &&
                metadatum["first_release_date"].IsInt64())
                rom.firstReleaseDate = metadatum["first_release_date"].GetInt64() / 1000;
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

std::vector<RomMApiClient::Rom> RomMApiClient::fetchRoms(
    int platformId,
    const std::string& updatedAfterUtc,
    const std::function<void(int, int)>& onRomsFetched)
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
        std::string url {buildUrl("/api/roms?platform_ids=" + std::to_string(platformId) +
                                  "&limit=" + std::to_string(pageSize) +
                                  "&offset=" + std::to_string(offset))};
        if (!updatedAfterUtc.empty())
            url += "&updated_after=" + HttpReq::urlEncode(updatedAfterUtc);

        HttpReq req {url, false, "", mToken};
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

        total = (doc.HasMember("total") && doc["total"].IsInt()) ?
                    doc["total"].GetInt() :
                    static_cast<int>(roms.size() + doc["items"].GetArray().Size());

        auto entryArray = doc["items"].GetArray();
        if (!entryArray.Empty() && onRomsFetched)
            onRomsFetched(0, total);
        for (const auto& entry : doc["items"].GetArray()) {
            roms.emplace_back(parseRom(entry));
            if (onRomsFetched)
                onRomsFetched(1, total);
        }
        if (onRomsFetched)
            onRomsFetched(0, total);

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
    HttpReq req {url, false, "", mToken};
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

bool RomMApiClient::fetchRomById(int romId, Rom& outRom)
{
    if (mServerURL.empty()) {
        mLastError = "No RomM server URL configured";
        return false;
    }

    const std::string url {buildUrl("/api/roms/" + std::to_string(romId))};
    HttpReq req {url, false, "", mToken};
    if (!waitForRequest(req))
        return false;

    Document doc;
    doc.Parse(req.getContent().c_str());
    if (doc.HasParseError() || !doc.IsObject()) {
        mLastError = "Unexpected RomM rom response format";
        return false;
    }
    if (!doc.HasMember("id") || !doc["id"].IsInt())
        return false;

    outRom = parseRom(doc);
    return true;
}

std::string RomMApiClient::getDownloadUrl(int romId, const std::string& fsName) const
{
    return buildUrl("/api/roms/" + std::to_string(romId) + "/content/" +
                    HttpReq::urlEncode(fsName));
}

std::string RomMApiClient::getFileDownloadUrl(int fileId, const std::string& fileName) const
{
    return buildUrl("/api/roms/" + std::to_string(fileId) + "/files/content/" +
                    HttpReq::urlEncode(fileName));
}

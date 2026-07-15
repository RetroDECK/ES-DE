//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomM.cpp
//
//  Functions for scraping from a self-hosted RomM (https://github.com/rommapp/romm) instance.
//  Called from Scraper.
//

#if defined(_MSC_VER) // MSVC compiler.
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#endif

#include "scrapers/RomM.h"

#include "FileData.h"
#include "Log.h"
#include "MameNames.h"
#include "RomM/RomMPlatformMapping.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <cmath>
#include <iomanip>

using namespace rapidjson;

namespace
{
    // Builds the base API URL from the RomM server URL setting, without a trailing slash.
    std::string getBaseURL()
    {
        std::string baseURL {Settings::getInstance()->getString("RomMServerURL")};
        while (!baseURL.empty() && baseURL.back() == '/')
            baseURL.pop_back();
        return baseURL;
    }

    void processGame(const Value& game, std::vector<ScraperSearchResult>& results)
    {
        if (!game.HasMember("id") || !game["id"].IsInt() || !game.HasMember("name") ||
            !game["name"].IsString())
            return;

        ScraperSearchResult result;
        result.gameID = std::to_string(game["id"].GetInt());
        result.mdl.set("name", game["name"].GetString());

        if (game.HasMember("summary") && game["summary"].IsString())
            result.mdl.set("desc", game["summary"].GetString());

        if (game.HasMember("url_cover") && game["url_cover"].IsString()) {
            result.coverUrl = game["url_cover"].GetString();
            result.coverFormat = Utils::FileSystem::getExtension(result.coverUrl);
        }

        if (game.HasMember("metadatum") && game["metadatum"].IsObject()) {
            const Value& metadatum {game["metadatum"]};

            // first_release_date is a Unix timestamp in MILLISECONDS since epoch, UTC (not
            // seconds, and not a string). Converted to a UTC calendar date via gmtime_r() rather
            // than through Utils::Time::DateTime/stringToTime, since those work in local time
            // and would shift the date by a day in most timezones for what is meant to be a
            // plain calendar date, not a precise instant.
            if (metadatum.HasMember("first_release_date") &&
                metadatum["first_release_date"].IsInt64()) {
                const int64_t rawTimestampMs {metadatum["first_release_date"].GetInt64()};
                const time_t releaseTimestamp {static_cast<time_t>(rawTimestampMs / 1000)};

                tm utcTime {};
#if defined(_WIN64)
                gmtime_s(&utcTime, &releaseTimestamp);
#else
                gmtime_r(&releaseTimestamp, &utcTime);
#endif
                const int year {utcTime.tm_year + 1900};
                const int currentYear {
                    Utils::Time::DateTime(Utils::Time::now()).getTimeStruct().tm_year + 1900};

                // RomM/IGDB metadata occasionally has an implausible release date for a
                // specific entry - skip setting the field rather than displaying garbage, and
                // log the raw value so a genuine parsing/unit bug can be told apart from bad
                // upstream data.
                if (year < 1950 || year > currentYear + 2) {
                    LOG(LogWarning)
                        << "RomM scraper: Ignoring implausible release date for \""
                        << game["name"].GetString() << "\" (raw timestamp " << rawTimestampMs
                        << " ms -> year " << year << ")";
                }
                else {
                    // Formatted directly to MD_DATE's raw storage format, see the
                    // "releasedate" default value ("19700101T000000") in MetaData.cpp.
                    std::stringstream dateStream;
                    dateStream << std::setfill('0') << std::setw(4) << year << std::setw(2)
                              << (utcTime.tm_mon + 1) << std::setw(2) << utcTime.tm_mday
                              << "T000000";
                    result.mdl.set("releasedate", dateStream.str());
                }
            }

            if (metadatum.HasMember("average_rating") && metadatum["average_rating"].IsNumber()) {
                // RomM ratings are on a 0-100 scale, ES-DE expects 0.0-1.0. Round to the
                // closest .1 value (i.e. to the closest half-star), mirroring how the
                // ScreenScraper backend rounds its own rating scale.
                float ratingVal {metadatum["average_rating"].GetFloat() / 100.0f};
                ratingVal = std::min(1.0f, std::max(0.0f, ratingVal));
                ratingVal = ceilf(ratingVal / 0.1f) / 10.0f;
                if (ratingVal > 0.0f) {
                    std::stringstream ss;
                    ss << ratingVal;
                    result.mdl.set("rating", ss.str());
                }
            }

            if (metadatum.HasMember("genres") && metadatum["genres"].IsArray()) {
                std::string genre;
                for (const auto& entry : metadatum["genres"].GetArray()) {
                    if (!entry.IsString())
                        continue;
                    if (!genre.empty())
                        genre += ", ";
                    genre += entry.GetString();
                }
                if (!genre.empty())
                    result.mdl.set("genre", genre);
            }

            // RomM doesn't distinguish developer from publisher, so the combined company list
            // is used to populate both fields - better than leaving publisher at its "unknown"
            // default when RomM actually has company data for the game.
            if (metadatum.HasMember("companies") && metadatum["companies"].IsArray()) {
                std::string companies;
                for (const auto& entry : metadatum["companies"].GetArray()) {
                    if (!entry.IsString())
                        continue;
                    if (!companies.empty())
                        companies += ", ";
                    companies += entry.GetString();
                }
                if (!companies.empty()) {
                    result.mdl.set("developer", companies);
                    result.mdl.set("publisher", companies);
                }
            }

            // player_count is a free-form string in RomM's API, e.g. "1-4", not an integer.
            if (metadatum.HasMember("player_count") && metadatum["player_count"].IsString())
                result.mdl.set("players", metadatum["player_count"].GetString());
        }

        result.mediaURLFetch = COMPLETED;
        results.push_back(result);
    }
} // namespace

void romm_generate_scraper_requests(const ScraperSearchParams& params,
                                    std::queue<std::unique_ptr<ScraperRequest>>& requests,
                                    std::vector<ScraperSearchResult>& results)
{
    const std::string baseURL {getBaseURL()};
    if (baseURL.empty()) {
        LOG(LogError) << "RomM scraper: No server URL configured";
        return;
    }

    const std::string token {Settings::getInstance()->getString("RomMToken")};

    std::string cleanName {params.nameOverride};

    // Prefer an exact hash match when available, as it's precise and needs no platform
    // mapping. Only applies to automatic (non-interactive) searches with no name override.
    if (cleanName.empty() && !params.md5Hash.empty() &&
        Settings::getInstance()->getBool("ScraperSearchFileHash")) {
        const std::string path {baseURL +
                                "/api/roms/by-hash?md5_hash=" + HttpReq::urlEncode(params.md5Hash)};
        requests.push(
            std::unique_ptr<ScraperRequest>(new RomMRequest(results, path, token)));
        return;
    }

    if (cleanName.empty()) {
        if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
            cleanName = Utils::String::removeParenthesis(params.game->metadata.get("name"));
        }
        else if (params.game->isArcadeGame()) {
            cleanName = MameNames::getInstance().getCleanName(params.game->getCleanName());
        }
        else if (params.game->getType() == GAME &&
                 Utils::FileSystem::isDirectory(params.game->getFullPath())) {
            // For the special case where a directory has a supported file extension and is
            // therefore interpreted as a file, exclude the extension from the search.
            cleanName = Utils::FileSystem::getStem(params.game->getCleanName());
        }
        else {
            cleanName = params.game->getCleanName();
        }
    }

    cleanName = Utils::String::trim(cleanName);
    if (Settings::getInstance()->getBool("ScraperConvertUnderscores"))
        cleanName = Utils::String::replace(cleanName, "_", " ");

    std::string path {baseURL + "/api/roms?search_term=" + HttpReq::urlEncode(cleanName) +
                      "&limit=" + std::to_string(MAX_SCRAPER_RESULTS)};

    const RomMSystemMapping* mapping {
        RomMPlatformMapping::getInstance().getMapping(params.system->getName())};
    if (mapping != nullptr && mapping->rommPlatformId >= 0) {
        path += "&platform_ids=" + std::to_string(mapping->rommPlatformId);
    }
    else {
        LOG(LogWarning) << "RomM scraper: No platform mapping configured for system \""
                        << params.system->getName() << "\", search will be inaccurate";
    }

    requests.push(
        std::unique_ptr<ScraperRequest>(new RomMRequest(results, path, token)));
}

void RomMRequest::process(const std::unique_ptr<HttpReq>& req,
                          std::vector<ScraperSearchResult>& results)
{
    assert(req->status() == HttpReq::REQ_SUCCESS);

    Document doc;
    doc.Parse(req->getContent().c_str());

    if (doc.HasParseError()) {
        const std::string err {std::string("RomMRequest - Error parsing JSON \n\t") +
                               GetParseError_En(doc.GetParseError())};
        setError(err, true);
        LOG(LogError) << err;
        return;
    }

    if (!doc.IsObject()) {
        LOG(LogWarning) << "RomMRequest - Unexpected response format";
        return;
    }

    // The by-hash endpoint returns a single rom object directly.
    if (doc.HasMember("id")) {
        processGame(doc, results);
        return;
    }

    // The roms-listing endpoint returns a paginated envelope with an "items" array.
    if (doc.HasMember("items") && doc["items"].IsArray()) {
        const Value& items {doc["items"]};
        for (SizeType i {0}; i < items.Size() && i < MAX_SCRAPER_RESULTS; ++i)
            processGame(items[i], results);
        if (results.empty()) {
            LOG(LogDebug) << "RomMRequest::process(): No games found";
        }
        return;
    }

    LOG(LogWarning) << "RomMRequest - Unexpected response format";
}

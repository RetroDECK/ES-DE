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
#include "RomM/RomMApiClient.h"
#include "RomM/RomMUtils.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <unordered_set>

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

    void processGame(const Value& game,
                     const std::string& baseURL,
                     std::vector<ScraperSearchResult>& results)
    {
        if (!game.HasMember("id") || !game["id"].IsInt() || !game.HasMember("name") ||
            !game["name"].IsString())
            return;

        ScraperSearchResult result;
        result.gameID = std::to_string(game["id"].GetInt());
        result.mdl.set("name", game["name"].GetString());

        if (game.HasMember("summary") && game["summary"].IsString())
            result.mdl.set("desc", game["summary"].GetString());

        // Prefer the cover hosted by RomM itself over "url_cover", which links directly to the
        // upstream provider using RomM's own credentials and gets rejected for direct client
        // requests. Both fields are "" rather than absent when there's no cover, hence the
        // length check.
        const char* coverPathField {nullptr};
        if (game.HasMember("path_cover_large") && game["path_cover_large"].IsString() &&
            game["path_cover_large"].GetStringLength() > 0)
            coverPathField = "path_cover_large";
        else if (game.HasMember("path_cover_small") && game["path_cover_small"].IsString() &&
                 game["path_cover_small"].GetStringLength() > 0)
            coverPathField = "path_cover_small";

        if (coverPathField != nullptr) {
            std::string coverPath {game[coverPathField].GetString()};
            // The cache-busting "?ts=..." query RomM appends contains a raw, unencoded space.
            coverPath = Utils::String::replace(coverPath, " ", "%20");
            result.coverUrl = baseURL + coverPath;

            const size_t queryPos {coverPath.find_first_of("?#")};
            result.coverFormat = Utils::String::toLower(
                Utils::FileSystem::getExtension(coverPath.substr(0, queryPos)));
        }
        else if (game.HasMember("url_cover") && game["url_cover"].IsString() &&
                 game["url_cover"].GetStringLength() > 0) {
            // Fallback for when RomM has no cached cover. The upstream URL's real image format
            // is only conveyed via a query parameter, so its path often ends in just the
            // provider's API script name (e.g. ".php") - only trust a plausible image
            // extension, otherwise skip the download rather than save the error response.
            std::string coverPath {game["url_cover"].GetString()};
            const size_t queryPos {coverPath.find_first_of("?#")};
            if (queryPos != std::string::npos)
                coverPath.resize(queryPos);

            static const std::unordered_set<std::string> validImageExtensions {
                ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp"};

            std::string detectedFormat {
                Utils::String::toLower(Utils::FileSystem::getExtension(coverPath))};
            if (validImageExtensions.find(detectedFormat) != validImageExtensions.cend()) {
                result.coverUrl = game["url_cover"].GetString();
                result.coverFormat = detectedFormat;
            }
            else {
                LOG(LogDebug) << "RomM scraper: Ignoring non-image url_cover fallback for \""
                              << game["name"].GetString()
                              << "\": " << game["url_cover"].GetString();
            }
        }

        if (game.HasMember("metadatum") && game["metadatum"].IsObject()) {
            const Value& metadatum {game["metadatum"]};

            // first_release_date is a Unix timestamp in MILLISECONDS since epoch, UTC (not
            // seconds, and not a string).
            if (metadatum.HasMember("first_release_date") &&
                metadatum["first_release_date"].IsInt64()) {
                const std::string releaseDate {RomMUtils::formatReleaseDate(
                    metadatum["first_release_date"].GetInt64() / 1000, game["name"].GetString())};
                if (!releaseDate.empty())
                    result.mdl.set("releasedate", releaseDate);
            }

            if (metadatum.HasMember("average_rating") && metadatum["average_rating"].IsNumber()) {
                const std::string rating {
                    RomMUtils::formatCommunityRating(metadatum["average_rating"].GetFloat())};
                if (!rating.empty())
                    result.mdl.set("rating", rating);
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
        requests.push(std::unique_ptr<ScraperRequest>(new RomMRequest(results, path, token)));
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

    RomMApiClient platformClient {baseURL, token};
    const std::vector<RomMApiClient::Platform> platforms {platformClient.fetchPlatforms()};

    int rommPlatformId {-1};
    for (const auto& platform : platforms) {
        for (const auto& platformId : params.system->getPlatformIds()) {
            if (RomMUtils::platformNameMatches(PlatformIds::getPlatformName(platformId),
                                               platform.slug, platform.fsSlug)) {
                rommPlatformId = platform.id;
                break;
            }
        }
        if (rommPlatformId >= 0)
            break;
    }

    if (rommPlatformId >= 0) {
        path += "&platform_ids=" + std::to_string(rommPlatformId);
    }
    else {
        LOG(LogWarning) << "RomM scraper: No matching RomM platform found for system \""
                        << params.system->getName() << "\", search will be inaccurate";
    }

    requests.push(std::unique_ptr<ScraperRequest>(new RomMRequest(results, path, token)));
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

    const std::string baseURL {getBaseURL()};

    // The by-hash endpoint returns a single rom object directly.
    if (doc.HasMember("id")) {
        processGame(doc, baseURL, results);
        return;
    }

    // The roms-listing endpoint returns a paginated envelope with an "items" array.
    if (doc.HasMember("items") && doc["items"].IsArray()) {
        const Value& items {doc["items"]};
        for (SizeType i {0}; i < items.Size() && i < MAX_SCRAPER_RESULTS; ++i)
            processGame(items[i], baseURL, results);
        if (results.empty()) {
            LOG(LogDebug) << "RomMRequest::process(): No games found";
        }
        return;
    }

    LOG(LogWarning) << "RomMRequest - Unexpected response format";
}

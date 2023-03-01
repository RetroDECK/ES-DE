//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamesDBJSONScraper.cpp
//
//  Functions specifically for scraping from thegamesdb.net
//  Called from Scraper.
//

#if defined(_MSC_VER) // MSVC compiler.
#define _SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING
#endif

#include "scrapers/GamesDBJSONScraper.h"
#include "scrapers/GamesDBJSONScraperResources.h"

#include "FileData.h"
#include "Log.h"
#include "MameNames.h"
#include "PlatformId.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"

#include <exception>
#include <map>
#include <pugixml.hpp>

using namespace PlatformIds;
using namespace rapidjson;

namespace
{
    TheGamesDBJSONRequestResources resources;

    const std::map<PlatformId, std::string> gamesdb_new_platformid_map {
        {THREEDO, "25"},
        {COMMODORE_AMIGA, "4911"},
        {COMMODORE_AMIGA_CD32, "4947"},
        {AMSTRAD_CPC, "4914"},
        {APPLE_II, "4942"},
        {ARCADE, "23"},
        {ATOMISWAVE, "23"},
        {SEGA_NAOMI, "23"},
        {ARCADIA_2001, "4963"},
        {BALLY_ASTROCADE, "4968"},
        {ATARI_800, "4943"},
        {ATARI_2600, "22"},
        {ATARI_5200, "26"},
        {ATARI_7800, "27"},
        {ATARI_JAGUAR, "28"},
        {ATARI_JAGUAR_CD, "29"},
        {ATARI_LYNX, "4924"},
        {ATARI_ST, "4937"},
        {ATARI_XE, "30"},
        {BBC_MICRO, "5013"},
        {CASIO_PV1000, "4964"},
        {CAVESTORY, "1"},
        {COLECOVISION, "31"},
        {COMMODORE_64, "40"},
        {COMMODORE_VIC20, "4945"},
        {CREATRONIC_MEGA_DUCK, "4948"},
        {DAPHNE, "23"},
        {FUJITSU_FM_TOWNS, "4932"},
        {INTELLIVISION, "32"},
        {APPLE_MACINTOSH, "37"},
        {GOOGLE_ANDROID, "4916"},
        {LCD_GAMES, "4951"},
        {MICROSOFT_XBOX, "14"},
        {MICROSOFT_XBOX_360, "15"},
        {MOONLIGHT, "1"},
        {MSX, "4929"},
        {MSX2, "4929"},
        {MSX_TURBO_R, "4929"},
        {SNK_NEO_GEO, "24"},
        {SNK_NEO_GEO_CD, "4956"},
        {SNK_NEO_GEO_POCKET, "4922"},
        {SNK_NEO_GEO_POCKET_COLOR, "4923"},
        {NINTENDO_3DS, "4912"},
        {NINTENDO_64, "3"},
        {NINTENDO_DS, "8"},
        {NINTENDO_FAMICOM, "7"},
        {NINTENDO_FAMICOM_DISK_SYSTEM, "4936"},
        {NINTENDO_ENTERTAINMENT_SYSTEM, "7"},
        {NINTENDO_GAME_BOY, "4"},
        {NINTENDO_GAME_BOY_ADVANCE, "5"},
        {NINTENDO_GAME_BOY_COLOR, "41"},
        {NINTENDO_GAMECUBE, "2"},
        {NINTENDO_WII, "9"},
        {NINTENDO_WII_U, "38"},
        {NINTENDO_VIRTUAL_BOY, "4918"},
        {NINTENDO_GAME_AND_WATCH, "4950"},
        {NINTENDO_POKEMON_MINI, "4957"},
        {NINTENDO_SATELLAVIEW, "6"},
        {NINTENDO_SWITCH, "4971"},
        {BANDAI_SUFAMI_TURBO, "6"},
        {DRAGON32, "4952"},
        {DOS, "1"},
        {TANGERINE_ORIC, "4986"},
        {GAMEENGINE_SCUMMVM, "1"},
        {PC, "1"},
        {PC_WINDOWS, "1"},
        {VALVE_STEAM, "1"},
        {NEC_PCFX, "4930"},
        {PHILIPS_CDI, "4917"},
        {SAMCOUPE, "4979"},
        {SEGA_32X, "33"},
        {SEGA_CD, "21"},
        {SEGA_DREAMCAST, "16"},
        {SEGA_GAME_GEAR, "20"},
        {SEGA_GENESIS, "18"},
        {SEGA_MASTER_SYSTEM, "35"},
        {SEGA_MEGA_DRIVE, "36"},
        {SEGA_SATURN, "17"},
        {SEGA_SG1000, "4949"},
        {SONY_PLAYSTATION, "10"},
        {SONY_PLAYSTATION_2, "11"},
        {SONY_PLAYSTATION_3, "12"},
        {SONY_PLAYSTATION_4, "4919"},
        {SONY_PLAYSTATION_VITA, "39"},
        {SONY_PLAYSTATION_PORTABLE, "13"},
        {SUPER_NINTENDO, "6"},
        {SHARP_X1, "4977"},
        {SHARP_X68000, "4931"},
        {NEC_SUPERGRAFX, "34"},
        {NEC_PC_8800, "4933"},
        {NEC_PC_9800, "4934"},
        {NEC_PC_ENGINE, "34"},
        {NEC_PC_ENGINE_CD, "4955"},
        {BANDAI_WONDERSWAN, "4925"},
        {BANDAI_WONDERSWAN_COLOR, "4926"},
        {SINCLAIR_ZX_SPECTRUM, "4913"},
        {SINCLAIR_ZX81_SINCLAR, "5010"},
        {TIGER_GAME_COM, "4940"},
        {VIDEOPAC_ODYSSEY2, "4927"},
        {VECTREX, "4939"},
        {VTECH_CREATIVISION, "5005"},
        {VTECH_VSMILE, "4988"},
        {WATARA_SUPERVISION, "4959"},
        {TANDY_COLOR_COMPUTER, "4941"},
        {TANDY_TRS80, "4941"},
        {TEXAS_INSTRUMENTS_TI99, "4953"}};

} // namespace

void thegamesdb_generate_json_scraper_requests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results)
{
    resources.prepare();
    std::string path {"https://api.thegamesdb.net/v1"};
    bool usingGameID {false};
    const std::string apiKey {std::string("apikey=") + resources.getApiKey()};
    std::string cleanName {params.nameOverride};
    if (!cleanName.empty() && cleanName.substr(0, 3) == "id:") {
        std::string gameID {cleanName.substr(3)};
        path += "/Games/ByGameID?" + apiKey +
                "&fields=players,publishers,genres,overview,last_updated,rating,"
                "platform,coop,youtube,os,processor,ram,hdd,video,sound,alternates&id=" +
                HttpReq::urlEncode(gameID);
        usingGameID = true;
    }
    else {
        if (cleanName.empty()) {
            // If the setting to search based on the metadata name has been set, then search
            // using this regardless of whether the entry is an arcade game.
            if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
                cleanName = Utils::String::removeParenthesis(params.game->metadata.get("name"));
            }
            else {
                // If not searching based on the metadata name, then check whether it's an
                // arcade game and if so expand to the full game name. This is required as
                // TheGamesDB has issues with searching using the short MAME names.
                if (params.game->isArcadeGame()) {
                    cleanName = MameNames::getInstance().getCleanName(params.game->getCleanName());
                }
                else {
                    if (params.game->getType() == GAME &&
                        Utils::FileSystem::isDirectory(params.game->getFullPath())) {
                        // For the special case where a directory has a supported file extension
                        // and is therefore interpreted as a file, exclude the extension from the
                        // search.
                        cleanName = Utils::FileSystem::getStem(params.game->getCleanName());
                    }
                    else {
                        cleanName = params.game->getCleanName();
                    }
                }
            }
        }

        // Trim leading and trailing whitespaces.
        cleanName = Utils::String::trim(cleanName);

        if (Settings::getInstance()->getBool("ScraperConvertUnderscores"))
            cleanName = Utils::String::replace(cleanName, "_", " ");

        path += "/Games/ByGameName?" + apiKey +
                "&fields=players,publishers,genres,overview,last_updated,rating,"
                "platform,coop,youtube,os,processor,ram,hdd,video,sound,alternates&name=" +
                HttpReq::urlEncode(cleanName);
    }

    if (usingGameID) {
        // If we have the ID already, we don't need the GetGamelist request.
        requests.push(std::unique_ptr<ScraperRequest>(new TheGamesDBJSONRequest(results, path)));
    }
    else {
        std::string platformQueryParam;
        auto& platforms = params.system->getPlatformIds();
        if (!platforms.empty()) {
            bool first {true};
            platformQueryParam += "&filter%5Bplatform%5D=";
            for (auto platformIt = platforms.cbegin(); // Line break.
                 platformIt != platforms.cend(); ++platformIt) {
                auto mapIt = gamesdb_new_platformid_map.find(*platformIt);
                if (mapIt != gamesdb_new_platformid_map.cend()) {
                    if (!first)
                        platformQueryParam += ",";
                    platformQueryParam += HttpReq::urlEncode(mapIt->second);
                    first = false;
                }
                else {
                    LOG(LogWarning)
                        << "TheGamesDB scraper: No support for platform \""
                        << getPlatformName(*platformIt) << "\", search will be inaccurate";
                }
            }
            path += platformQueryParam;
        }
        else {
            LOG(LogWarning) << "TheGamesDB scraper: No platform defined, search will be inaccurate";
        }

        requests.push(
            std::unique_ptr<ScraperRequest>(new TheGamesDBJSONRequest(requests, results, path)));
    }
}

void thegamesdb_generate_json_scraper_requests(
    const std::string& gameIDs,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results)
{
    resources.prepare();
    std::string path = "https://api.thegamesdb.net/v1";
    const std::string apiKey {std::string("apikey=") + resources.getApiKey()};

    path += "/Games/Images/GamesImages?" + apiKey + "&games_id=" + gameIDs;

    requests.push(
        std::unique_ptr<ScraperRequest>(new TheGamesDBJSONRequest(requests, results, path)));
}

namespace
{
    std::string getStringOrThrow(const Value& v, const std::string& key)
    {
        if (!v.HasMember(key.c_str()) || !v[key.c_str()].IsString()) {
            throw std::runtime_error(
                "rapidjson internal assertion failure: missing or non string key:" + key);
        }
        return v[key.c_str()].GetString();
    }

    int getIntOrThrow(const Value& v, const std::string& key)
    {
        if (!v.HasMember(key.c_str()) || !v[key.c_str()].IsInt()) {
            throw std::runtime_error(
                "rapidjson internal assertion failure: missing or non int key:" + key);
        }
        return v[key.c_str()].GetInt();
    }

    int getIntOrThrow(const Value& v)
    {
        if (!v.IsInt()) {
            throw std::runtime_error("rapidjson internal assertion failure: not an int");
        }
        return v.GetInt();
    }

    std::string getDeveloperString(const Value& v)
    {
        if (!v.IsArray())
            return "";

        std::string out = "";
        bool first {true};
        for (int i = 0; i < static_cast<int>(v.Size()); ++i) {
            auto mapIt = resources.gamesdb_new_developers_map.find(getIntOrThrow(v[i]));

            if (mapIt == resources.gamesdb_new_developers_map.cend())
                continue;

            if (!first)
                out += ", ";

            out += mapIt->second;
            first = false;
        }
        return out;
    }

    std::string getPublisherString(const Value& v)
    {
        if (!v.IsArray())
            return "";

        std::string out;
        bool first {true};
        for (int i = 0; i < static_cast<int>(v.Size()); ++i) {
            auto mapIt = resources.gamesdb_new_publishers_map.find(getIntOrThrow(v[i]));

            if (mapIt == resources.gamesdb_new_publishers_map.cend())
                continue;

            if (!first)
                out += ", ";

            out += mapIt->second;
            first = false;
        }
        return out;
    }

    std::string getGenreString(const Value& v)
    {
        if (!v.IsArray())
            return "";

        std::string out;
        bool first {true};
        for (int i = 0; i < static_cast<int>(v.Size()); ++i) {
            auto mapIt = resources.gamesdb_new_genres_map.find(getIntOrThrow(v[i]));

            if (mapIt == resources.gamesdb_new_genres_map.cend())
                continue;

            if (!first)
                out += ", ";

            out += mapIt->second;
            first = false;
        }
        return out;
    }

    void processGame(const Value& game, std::vector<ScraperSearchResult>& results)
    {
        ScraperSearchResult result;

        // Platform IDs.
        if (game.HasMember("platform") && game["platform"].IsInt()) {
            for (auto& platform : gamesdb_new_platformid_map) {
                if (platform.second == std::to_string(game["platform"].GetInt()))
                    result.platformIDs.push_back(platform.first);
            }
        }

        if (result.platformIDs.empty())
            result.platformIDs.push_back(PlatformId::PLATFORM_UNKNOWN);

        if (game.HasMember("id") && game["id"].IsInt())
            result.gameID = std::to_string(getIntOrThrow(game, "id"));

        result.mdl.set("name", getStringOrThrow(game, "game_title"));
        LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Name: " << result.mdl.get("name");

        if (game.HasMember("overview") && game["overview"].IsString())
            result.mdl.set("desc", Utils::String::replace(game["overview"].GetString(), "\r", ""));

        if (game.HasMember("release_date") && game["release_date"].IsString()) {
            result.mdl.set("releasedate", Utils::Time::DateTime(Utils::Time::stringToTime(
                                              game["release_date"].GetString(), "%Y-%m-%d")));
            LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Release Date (unparsed): "
                          << game["release_date"].GetString();
            LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Release Date (parsed): "
                          << result.mdl.get("releasedate");
        }

        if (game.HasMember("developers") && game["developers"].IsArray()) {
            result.mdl.set("developer", getDeveloperString(game["developers"]));
            LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Developer: "
                          << result.mdl.get("developer");
        }

        if (game.HasMember("publishers") && game["publishers"].IsArray()) {
            result.mdl.set("publisher", getPublisherString(game["publishers"]));
            LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Publisher: "
                          << result.mdl.get("publisher");
        }

        if (game.HasMember("genres") && game["genres"].IsArray()) {
            result.mdl.set("genre", getGenreString(game["genres"]));
            LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Genre: "
                          << result.mdl.get("genre");
        }

        if (game.HasMember("players") && game["players"].IsInt()) {
            result.mdl.set("players", std::to_string(game["players"].GetInt()));
            LOG(LogDebug) << "GamesDBJSONScraper::processGame(): Players: "
                          << result.mdl.get("players");
        }

        result.mediaURLFetch = NOT_STARTED;
        results.push_back(result);
    }
} // namespace

void processMediaURLs(const Value& images,
                      const std::string& base_url,
                      std::vector<ScraperSearchResult>& results)
{
    ScraperSearchResult result;

    // Step through each game ID in the JSON server response.
    for (auto it = images.MemberBegin(); it != images.MemberEnd(); ++it) {
        result.gameID = it->name.GetString();
        const Value& gameMedia {images[it->name]};
        result.coverUrl = "";
        result.fanartUrl = "";
        result.marqueeUrl = "";
        result.screenshotUrl = "";
        result.titlescreenUrl = "";

        // Quite excessive testing for valid values, but you never know what the server has
        // returned and we don't want to crash the program due to malformed data.
        if (gameMedia.IsArray()) {
            for (SizeType i = 0; i < gameMedia.Size(); ++i) {
                std::string mediatype;
                std::string mediaside;
                if (gameMedia[i]["type"].IsString())
                    mediatype = gameMedia[i]["type"].GetString();
                if (gameMedia[i]["side"].IsString())
                    mediaside = gameMedia[i]["side"].GetString();

                if (mediatype == "boxart" && mediaside == "front")
                    if (gameMedia[i]["filename"].IsString())
                        result.coverUrl = base_url + gameMedia[i]["filename"].GetString();
                if (mediatype == "boxart" && mediaside == "back")
                    if (gameMedia[i]["filename"].IsString())
                        result.backcoverUrl = base_url + gameMedia[i]["filename"].GetString();
                // Only process the first fanart result.
                if (mediatype == "fanart" && result.fanartUrl == "")
                    if (gameMedia[i]["filename"].IsString())
                        result.fanartUrl = base_url + gameMedia[i]["filename"].GetString();
                if (mediatype == "clearlogo")
                    if (gameMedia[i]["filename"].IsString())
                        result.marqueeUrl = base_url + gameMedia[i]["filename"].GetString();
                if (mediatype == "screenshot")
                    if (gameMedia[i]["filename"].IsString())
                        result.screenshotUrl = base_url + gameMedia[i]["filename"].GetString();
                if (mediatype == "titlescreen")
                    if (gameMedia[i]["filename"].IsString())
                        result.titlescreenUrl = base_url + gameMedia[i]["filename"].GetString();
            }
        }
        result.mediaURLFetch = COMPLETED;
        results.push_back(result);
    }
}

void TheGamesDBJSONRequest::process(const std::unique_ptr<HttpReq>& req,
                                    std::vector<ScraperSearchResult>& results)
{
    assert(req->status() == HttpReq::REQ_SUCCESS);

    Document doc;
    doc.Parse(req->getContent().c_str());

    if (doc.HasParseError()) {
        std::string err {std::string("TheGamesDBJSONRequest - Error parsing JSON \n\t") +
                         GetParseError_En(doc.GetParseError())};
        setError(err);
        LOG(LogError) << err;
        return;
    }

    // If the response contains the 'images' object, then it's a game media URL request.
    if (doc.HasMember("data") && doc["data"].HasMember("images") &&
        doc["data"]["images"].IsObject()) {

        const Value& images {doc["data"]["images"]};
        const Value& base_url {doc["data"]["base_url"]};
        std::string baseImageUrlLarge;

        if (base_url.HasMember("large") && base_url["large"].IsString()) {
            baseImageUrlLarge = base_url["large"].GetString();
        }
        else {
            LOG(LogWarning) << "TheGamesDBJSONRequest - No URL path for large images\n";
            return;
        }

        try {
            processMediaURLs(images, baseImageUrlLarge, results);
        }
        catch (std::runtime_error& e) {
            LOG(LogError) << "Error while processing media URLs: " << e.what();
        }

        // Find how many more requests we can make before the scraper
        // request allowance counter is reset.
        if (doc.HasMember("remaining_monthly_allowance") && doc.HasMember("extra_allowance")) {
            for (size_t i = 0; i < results.size(); ++i) {
                results[i].scraperRequestAllowance =
                    doc["remaining_monthly_allowance"].GetInt() + doc["extra_allowance"].GetInt();
            }
            LOG(LogDebug) << "TheGamesDBJSONRequest::process(): "
                             "Remaining monthly scraping allowance: "
                          << results.back().scraperRequestAllowance;
        }
        return;
    }

    // These process steps are for the initial scraping response.
    if (!doc.HasMember("data") || !doc["data"].HasMember("games") ||
        !doc["data"]["games"].IsArray()) {
        LOG(LogWarning) << "TheGamesDBJSONRequest - Response had no game data\n";
        return;
    }

    const Value& games {doc["data"]["games"]};
    resources.ensureResources();

    for (int i = 0; i < static_cast<int>(games.Size()); ++i) {
        auto& v = games[i];
        try {
            processGame(v, results);
        }
        catch (std::runtime_error& e) {
            LOG(LogError) << "Error while processing game: " << e.what();
        }
    }

    if (results.size() == 0) {
        LOG(LogDebug) << "TheGamesDBJSONRequest::process(): No games found";
    }
}

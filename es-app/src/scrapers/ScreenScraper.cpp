//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScreenScraper.cpp
//
//  Functions specifically for scraping from screenscraper.fr
//  Called from Scraper.
//

#include "scrapers/ScreenScraper.h"

#include "FileData.h"
#include "Log.h"
#include "PlatformId.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"

#include <cmath>
#include <cstring>
#include <pugixml.hpp>

using namespace PlatformIds;

namespace
{
    // List of systems and their IDs from:
    // https://www.screenscraper.fr/api/systemesListe.php?devid=xxx&devpassword=yyy&softname=zzz&output=XML
    const std::map<PlatformId, unsigned short> screenscraper_platformid_map {
        {THREEDO, 29},
        {COMMODORE_AMIGA, 64},
        {COMMODORE_AMIGA_CD32, 130},
        {AMSTRAD_CPC, 65},
        {AMSTRAD_GX4000, 87},
        {APPLE_II, 86},
        {APPLE_IIGS, 217},
        {ARCADE, 75},
        {ARCADIA_2001, 94},
        {BALLY_ASTROCADE, 44},
        {ATARI_800, 43},
        {ATARI_2600, 26},
        {ATARI_5200, 40},
        {ATARI_7800, 41},
        {ATARI_JAGUAR, 27},
        {ATARI_JAGUAR_CD, 171},
        {ATARI_LYNX, 28},
        {ATARI_ST, 42},
        {ATARI_XE, 43},
        {ATOMISWAVE, 53},
        {BBC_MICRO, 37},
        {CASIO_PV1000, 74},
        {CAVESTORY, 135},
        {COLECOVISION, 48},
        {VTECH_CREATIVISION, 241},
        {VTECH_VSMILE, 120},
        {COMMODORE_64, 66},
        {COMMODORE_CDTV, 129},
        {COMMODORE_VIC20, 73},
        {CREATRONIC_MEGA_DUCK, 90},
        {DAPHNE, 49},
        {FUJITSU_FM_TOWNS, 253},
        {INTELLIVISION, 115},
        {GAMEENGINE_LUTRO, 206},
        {GAMEENGINE_WASM4, 262},
        {APPLE_MACINTOSH, 146},
        {GOOGLE_ANDROID, 63},
        {LCD_GAMES, 75},
        {MICROSOFT_XBOX, 32},
        {MICROSOFT_XBOX_360, 33},
        {MOONLIGHT, 138},
        {MSX, 113},
        {MSX2, 116},
        {MSX_TURBO_R, 118},
        {SNK_NEO_GEO, 142},
        {SNK_NEO_GEO_CD, 70},
        {SNK_NEO_GEO_POCKET, 25},
        {SNK_NEO_GEO_POCKET_COLOR, 82},
        {NINTENDO_3DS, 17},
        {NINTENDO_64, 14},
        {NINTENDO_DS, 15},
        {NINTENDO_FAMICOM, 3},
        {NINTENDO_FAMICOM_DISK_SYSTEM, 106},
        {NINTENDO_ENTERTAINMENT_SYSTEM, 3},
        {FAIRCHILD_CHANNELF, 80},
        {NINTENDO_GAME_BOY, 9},
        {NINTENDO_GAME_BOY_ADVANCE, 12},
        {NINTENDO_GAME_BOY_COLOR, 10},
        {NINTENDO_GAMECUBE, 13},
        {NINTENDO_WII, 16},
        {NINTENDO_WII_U, 18},
        {NINTENDO_VIRTUAL_BOY, 11},
        {NINTENDO_GAME_AND_WATCH, 52},
        {NINTENDO_POKEMON_MINI, 211},
        {NINTENDO_SATELLAVIEW, 107},
        {NINTENDO_SWITCH, 225},
        {BANDAI_SUFAMI_TURBO, 108},
        {DRAGON32, 91},
        {DOS, 135},
        {PC, 135},
        {PC_WINDOWS, 138},
        {VALVE_STEAM, 138},
        {NEC_PCFX, 72},
        {GAMEENGINE_PICO8, 234},
        {PHILIPS_CDI, 133},
        {GAMEENGINE_OPENBOR, 214},
        {GAMEENGINE_EASYRPG, 231},
        {TANGERINE_ORIC, 131},
        {GAMEENGINE_SCUMMVM, 123},
        {SEGA_32X, 19},
        {SEGA_CD, 20},
        {SEGA_DREAMCAST, 23},
        {SEGA_GAME_GEAR, 21},
        {SEGA_GENESIS, 1},
        {SEGA_MASTER_SYSTEM, 2},
        {SEGA_MEGA_DRIVE, 1},
        {SEGA_SATURN, 22},
        {SEGA_SG1000, 109},
        {SHARP_X1, 220},
        {SHARP_X68000, 79},
        {GAMEENGINE_SOLARUS, 223},
        {GAMEENGINE_Z_MACHINE, 215},
        {SONY_PLAYSTATION, 57},
        {SONY_PLAYSTATION_2, 58},
        {SONY_PLAYSTATION_3, 59},
        {SONY_PLAYSTATION_VITA, 62},
        {SONY_PLAYSTATION_PORTABLE, 61},
        {SAMCOUPE, 213},
        {SUPER_NINTENDO, 4},
        {NEC_SUPERGRAFX, 105},
        {GAMEENGINE_TIC80, 222},
        {NEC_PC_8800, 221},
        {NEC_PC_9800, 208},
        {NEC_PC_ENGINE, 31},
        {NEC_PC_ENGINE_CD, 114},
        {BANDAI_WONDERSWAN, 45},
        {BANDAI_WONDERSWAN_COLOR, 46},
        {SINCLAIR_ZX_SPECTRUM, 76},
        {SINCLAIR_ZX81_SINCLAR, 77},
        {VIDEOPAC_ODYSSEY2, 104},
        {VECTREX, 102},
        {TANDY_TRS80, 144},
        {TANDY_COLOR_COMPUTER, 144},
        {TEXAS_INSTRUMENTS_TI99, 205},
        {TIGER_GAME_COM, 121},
        {SEGA_NAOMI, 56},
        {THOMSON_MOTO, 141},
        {UZEBOX, 216},
        {WATARA_SUPERVISION, 207},
        {SPECTRAVIDEO, 218},
        {PALM_OS, 219}};

    // Help XML parsing method, finding an direct child XML node starting from the parent and
    // filtering by an attribute value list.
    pugi::xml_node find_child_by_attribute_list(const pugi::xml_node& node_parent,
                                                const std::string& node_name,
                                                const std::string& attribute_name,
                                                const std::vector<std::string> attribute_values)
    {
        for (auto _val : attribute_values) {
            for (pugi::xml_node node : node_parent.children(node_name.c_str())) {
                if (node.attribute(attribute_name.c_str()).value() == _val)
                    return node;
            }
        }

        return pugi::xml_node(nullptr);
    }

} // namespace

void screenscraper_generate_scraper_requests(const ScraperSearchParams& params,
                                             std::queue<std::unique_ptr<ScraperRequest>>& requests,
                                             std::vector<ScraperSearchResult>& results)
{
    std::string path;
    ScreenScraperRequest::ScreenScraperConfig ssConfig;

    ssConfig.automaticMode = params.automaticMode;

    if (params.game->isArcadeGame())
        ssConfig.isArcadeSystem = true;
    else
        ssConfig.isArcadeSystem = false;

    if (params.nameOverride == "") {
        if (Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
            path = ssConfig.getGameSearchUrl(
                Utils::String::removeParenthesis(params.game->metadata.get("name")));
        }
        else {
            std::string cleanName;
            if (params.game->getType() == GAME &&
                Utils::FileSystem::isDirectory(params.game->getFullPath())) {
                // For the special case where a directory has a supported file extension and is
                // therefore interpreted as a file, exclude the extension from the search.
                cleanName = Utils::FileSystem::getStem(params.game->getCleanName());
            }
            else {
                cleanName = params.game->getCleanName();
            }

            path = ssConfig.getGameSearchUrl(cleanName);
        }
    }
    else {
        path = ssConfig.getGameSearchUrl(params.nameOverride);
    }

    auto& platforms = params.system->getPlatformIds();
    std::vector<unsigned short> p_ids;

    // Get the IDs of each platform from the ScreenScraper list.
    for (auto platformIt = platforms.cbegin(); platformIt != platforms.cend(); ++platformIt) {
        auto mapIt = screenscraper_platformid_map.find(*platformIt);

        if (mapIt != screenscraper_platformid_map.cend()) {
            p_ids.emplace_back(mapIt->second);
        }
        else {
            LOG(LogWarning) << "ScreenScraper: No support for platform \""
                            << getPlatformName(*platformIt) << "\", search will be inaccurate";
            // Add the scrape request without a platform/system ID.
            requests.push(
                std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(requests, results, path)));
        }
    }

    if (p_ids.size() == 0) {
        LOG(LogWarning) << "ScreenScraper: No platform defined, search will be inaccurate";
        // Add the scrape request without a platform/system ID.
        requests.push(
            std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(requests, results, path)));
    }

    // Sort the platform IDs and remove duplicates.
    std::sort(p_ids.begin(), p_ids.end());
    auto last = std::unique(p_ids.begin(), p_ids.end());
    p_ids.erase(last, p_ids.end());

    for (auto platform = p_ids.cbegin(); platform != p_ids.cend(); ++platform) {
        path.append("&systemeid=").append(HttpReq::urlEncode(std::to_string(*platform)));
        requests.push(
            std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(requests, results, path)));
    }
}

void ScreenScraperRequest::process(const std::unique_ptr<HttpReq>& req,
                                   std::vector<ScraperSearchResult>& results)
{
    assert(req->status() == HttpReq::REQ_SUCCESS);

    pugi::xml_document doc;

    // It seems as if screenscraper.fr has changed their API slightly and now just returns
    // a simple text messsage upon not finding any matching game. If we don't return here,
    // we will get a pugixml error trying to process this string as an XML message.
    if (req->getContent().find("Erreur : Rom") == 0)
        return;

    pugi::xml_parse_result parseResult {doc.load_string(req->getContent().c_str())};

    if (!parseResult) {
        std::stringstream ss;
        ss << "ScreenScraperRequest - Error parsing XML: " << parseResult.description();

        std::string err = ss.str();
        LOG(LogError) << err;
        setError("ScreenScraper error: \n" + req->getContent());

        return;
    }

    processGame(doc, results);

    // For some files, screenscraper.fr consistently responds with the game name 'ZZZ(notgame)',
    // or sometimes in the longer format 'ZZZ(notgame):Fichier Annexes - Non Jeux'. For instance
    // this can happen for configuration files for DOS games such as 'setup.exe' and similar.
    // We definitely don't want to save these to our gamelists, so we simply skip these
    // responses. There also seems to be some cases where this type of response is randomly
    // returned instead of a valid game name, and retrying a second time returns the proper
    // name. But it's basically impossible to know which is the case, and we really can't
    // compensate for errors in the scraper service.
    for (auto it = results.cbegin(); it != results.cend();) {
        const std::string gameName {Utils::String::toUpper((*it).mdl.get("name"))};
        if (gameName.substr(0, 12) == "ZZZ(NOTGAME)") {
            LOG(LogWarning) << "ScreenScraperRequest - Received \"ZZZ(notgame)\" as game name, "
                               "ignoring response";
            it = results.erase(it);
        }
        else {
            ++it;
        }
    }
}

void ScreenScraperRequest::processGame(const pugi::xml_document& xmldoc,
                                       std::vector<ScraperSearchResult>& out_results)
{
    pugi::xml_node data {xmldoc.child("Data")};

    // Check if our username was included in the response (assuming an account is used).
    // It seems as if this information is randomly missing from the server response, which
    // also seems to correlate with missing scraper allowance data. This is however a scraper
    // service issue so we're not attempting to compensate for it here.
    if (Settings::getInstance()->getBool("ScraperUseAccountScreenScraper") &&
        Settings::getInstance()->getString("ScraperUsernameScreenScraper") != "" &&
        Settings::getInstance()->getString("ScraperPasswordScreenScraper") != "") {
        std::string userID {data.child("ssuser").child("id").text().get()};
        if (userID != "") {
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Scraping using account \""
                          << userID << "\"";
        }
        else {
            LOG(LogDebug)
                << "ScreenScraperRequest::processGame(): The configured account '"
                << Settings::getInstance()->getString("ScraperUsernameScreenScraper")
                << "' was not included in the scraper response, wrong username or password?";
        }
    }

    // Find how many more requests we can make before the scraper request
    // allowance counter is reset. For some strange reason the ssuser information
    // is not provided for all games even though the request looks identical apart
    // from the game name.
    unsigned requestsToday {data.child("ssuser").child("requeststoday").text().as_uint()};
    unsigned maxRequestsPerDay {data.child("ssuser").child("maxrequestsperday").text().as_uint()};
    unsigned int scraperRequestAllowance {maxRequestsPerDay - requestsToday};

    // Scraping allowance.
    if (maxRequestsPerDay > 0) {
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Daily scraping allowance: "
                      << requestsToday << "/" << maxRequestsPerDay << " ("
                      << scraperRequestAllowance << " remaining)";
    }
    else {
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Daily scraping allowance: "
                         "No statistics were provided with the response";
    }

    if (data.child("jeux"))
        data = data.child("jeux");

    for (pugi::xml_node game {data.child("jeu")}; game; game = game.next_sibling("jeu")) {
        ScraperSearchResult result;
        ScreenScraperRequest::ScreenScraperConfig ssConfig;

        result.scraperRequestAllowance = scraperRequestAllowance;
        result.gameID = game.attribute("id").as_string();

        std::string region {
            Utils::String::toLower(Settings::getInstance()->getString("ScraperRegion"))};
        std::string language {
            Utils::String::toLower(Settings::getInstance()->getString("ScraperLanguage"))};

        // Name fallback: US, WOR(LD). (Xpath: Data/jeu[0]/noms/nom[*]).
        std::string gameName {find_child_by_attribute_list(game.child("noms"), "nom", "region",
                                                           {region, "wor", "us", "ss", "eu", "jp"})
                                  .text()
                                  .get()};
        // Game names sometimes contain ampersands encoded as hex codes.
        gameName = Utils::String::replace(gameName, "&#x26;", "&");
        // In some very rare cases game names contain newline characters that we need to remove.
        result.mdl.set("name", Utils::String::replace(gameName, "\n", ""));
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Name: " << result.mdl.get("name");

        // Validate rating.
        // Process the rating even if the setting to scrape ratings has been disabled.
        // This is required so that the rating can still be shown in the scraper GUI.
        // GuiScraperSearch::saveMetadata() will take care of skipping the rating saving
        // if this option has been set as such.
        if (game.child("note")) {
            float ratingVal {game.child("note").text().as_float() / 20.0f};
            // Round up to the closest .1 value, i.e. to the closest half-star.
            ratingVal = ceilf(ratingVal / 0.1f) / 10;
            std::stringstream ss;
            ss << ratingVal;
            if (ratingVal > 0) {
                result.mdl.set("rating", ss.str());
                LOG(LogDebug) << "ScreenScraperRequest::processGame(): Rating: "
                              << result.mdl.get("rating");
            }
        }

        // Description fallback language: EN, WOR(LD).
        std::string description {find_child_by_attribute_list(game.child("synopsis"), "synopsis",
                                                              "langue", {language, "en", "wor"})
                                     .text()
                                     .get()};

        // Translate some HTML character codes to UTF-8 characters.
        if (!description.empty()) {
            result.mdl.set("desc", Utils::String::replace(description, "&nbsp;", " "));
            result.mdl.set("desc", Utils::String::replace(description, "&copy;", "©"));
        }

        // Get the date proper. The API returns multiple 'date' children nodes to the 'dates'
        // main child of 'jeu'. Date fallback: WOR(LD), US, SS, JP, EU.
        std::string date {find_child_by_attribute_list(game.child("dates"), "date", "region",
                                                       {region, "wor", "us", "ss", "jp", "eu"})
                              .text()
                              .get()};

        // Date can be YYYY-MM-DD or just YYYY.
        if (date.length() > 4) {
            result.mdl.set("releasedate",
                           Utils::Time::DateTime(Utils::Time::stringToTime(date, "%Y-%m-%d")));
        }
        else if (date.length() > 0) {
            result.mdl.set("releasedate",
                           Utils::Time::DateTime(Utils::Time::stringToTime(date, "%Y")));
        }

        if (date.length() > 0) {
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Release Date (unparsed): "
                          << date;
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Release Date (parsed): "
                          << result.mdl.get("releasedate");
        }

        // Developer for the game (Xpath: Data/jeu[0]/developpeur).
        std::string developer {game.child("developpeur").text().get()};
        if (!developer.empty()) {
            result.mdl.set("developer", Utils::String::replace(developer, "&nbsp;", " "));
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Developer: "
                          << result.mdl.get("developer");
        }

        // Publisher for the game (Xpath: Data/jeu[0]/editeur).
        std::string publisher {game.child("editeur").text().get()};
        if (!publisher.empty()) {
            result.mdl.set("publisher", Utils::String::replace(publisher, "&nbsp;", " "));
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Publisher: "
                          << result.mdl.get("publisher");
        }

        // Genre fallback language: EN. (Xpath: Data/jeu[0]/genres/genre[*]).
        std::string genre {
            find_child_by_attribute_list(game.child("genres"), "genre", "langue", {language, "en"})
                .text()
                .get()};
        if (!genre.empty()) {
            result.mdl.set("genre", genre);
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Genre: "
                          << result.mdl.get("genre");
        }

        // Players.
        std::string players {game.child("joueurs").text().get()};
        if (!players.empty()) {
            result.mdl.set("players", players);
            LOG(LogDebug) << "ScreenScraperRequest::processGame(): Players: "
                          << result.mdl.get("players");
        }

        pugi::xml_node system {game.child("systeme")};
        int platformID {system.attribute("id").as_int()};
        int parentPlatformID {system.attribute("parentid").as_int()};

        // Platform IDs.
        for (auto& platform : screenscraper_platformid_map) {
            if (platform.second == platformID || platform.second == parentPlatformID)
                result.platformIDs.emplace_back(platform.first);
        }

        if (result.platformIDs.empty())
            result.platformIDs.emplace_back(PlatformId::PLATFORM_UNKNOWN);

        // ScreenScraper controller scraping is currently broken, it's unclear if they will fix it.
        // // Controller (only for the Arcade and SNK Neo Geo systems).
        // if (parentPlatformID == 75 || parentPlatformID == 142) {
        //    std::string controller {Utils::String::toLower(game.child("controles").text().get())};
        //
        //     LOG(LogError) << controller;
        //
        //     if (!controller.empty()) {
        //         std::string controllerDescription {"Other"};
        //         // Place the steering wheel entry first as some games support both joysticks and
        //         // and steering wheels and it's likely more interesting to capture the steering
        //         // wheel option in this case.
        //         if (controller.find("steering wheel") != std::string::npos ||
        //             controller.find("paddle") != std::string::npos ||
        //             controller.find("pedal") != std::string::npos) {
        //             result.mdl.set("controller", "steering_wheel_generic");
        //             controllerDescription = "Steering wheel";
        //         }
        //         else if (controller.find("control type=\"joy") != std::string::npos ||
        //                  controller.find("joystick") != std::string::npos) {
        //             std::string buttonEntry;
        //             std::string buttonCount;
        //             if (controller.find("p1numbuttons=") != std::string::npos)
        //                buttonEntry = controller.substr(controller.find("p1numbuttons=") + 13, 4);
        //             else if (controller.find("buttons=") != std::string::npos)
        //                 buttonEntry = controller.substr(controller.find("buttons=") + 8, 5);
        //
        //             bool foundDigit {false};
        //             for (unsigned char character : buttonEntry) {
        //                 if (std::isdigit(character)) {
        //                     buttonCount.emplace_back(character);
        //                     foundDigit = true;
        //                 }
        //                 else if (foundDigit == true) {
        //                     break;
        //                 }
        //             }
        //
        //             if (buttonCount == "0") {
        //                 result.mdl.set("controller", "joystick_arcade_no_buttons");
        //                 controllerDescription = "Joystick (no buttons)";
        //             }
        //             else if (buttonCount == "1") {
        //                 result.mdl.set("controller", "joystick_arcade_1_button");
        //                 controllerDescription = "Joystick (1 button)";
        //             }
        //             else if (buttonCount == "2") {
        //                 result.mdl.set("controller", "joystick_arcade_2_buttons");
        //                 controllerDescription = "Joystick (2 buttons)";
        //             }
        //             else if (buttonCount == "3") {
        //                 result.mdl.set("controller", "joystick_arcade_3_buttons");
        //                 controllerDescription = "Joystick (3 buttons)";
        //             }
        //             else if (buttonCount == "4") {
        //                 result.mdl.set("controller", "joystick_arcade_4_buttons");
        //                 controllerDescription = "Joystick (4 buttons)";
        //             }
        //             else if (buttonCount == "5") {
        //                 result.mdl.set("controller", "joystick_arcade_5_buttons");
        //                 controllerDescription = "Joystick (5 buttons)";
        //             }
        //             else if (buttonCount == "6") {
        //                 result.mdl.set("controller", "joystick_arcade_6_buttons");
        //                 controllerDescription = "Joystick (6 buttons)";
        //             }
        //             else {
        //                 controllerDescription = "Joystick (other)";
        //             }
        //         }
        //         else if (controller.find("spinner") != std::string::npos) {
        //             result.mdl.set("controller", "spinner_generic");
        //             controllerDescription = "Spinner";
        //         }
        //         else if (controller.find("trackball") != std::string::npos) {
        //             result.mdl.set("controller", "trackball_generic");
        //             controllerDescription = "Trackball";
        //         }
        //         else if (controller.find("gun") != std::string::npos) {
        //             result.mdl.set("controller", "lightgun_generic");
        //             controllerDescription = "Lightgun";
        //         }
        //         else if (controller.find("stick") != std::string::npos) {
        //             result.mdl.set("controller", "flight_stick_generic");
        //             controllerDescription = "Flight stick";
        //         }
        //
        //         LOG(LogDebug) << "ScreenScraperRequest::processGame(): Controller: "
        //                       << controllerDescription;
        //     }
        // }

        // Media super-node.
        pugi::xml_node media_list {game.child("medias")};
        bool regionFallback {false};

        if (media_list) {
            // 3D box.
            processMedia(result, media_list, ssConfig.media_3dbox, result.box3DUrl,
                         result.box3DFormat, region);
            // Box back cover.
            processMedia(result, media_list, ssConfig.media_backcover, result.backcoverUrl,
                         result.backcoverFormat, region);
            // Box cover.
            processMedia(result, media_list, ssConfig.media_cover, result.coverUrl,
                         result.coverFormat, region);
            // Fan art.
            processMedia(result, media_list, ssConfig.media_fanart, result.fanartUrl,
                         result.fanartFormat, region);
            // Marquee (wheel).
            regionFallback = processMedia(result, media_list, ssConfig.media_marquee,
                                          result.marqueeUrl, result.marqueeFormat, region);
            // Marquee HD (wheel-hd) fallback if no regular wheel image was found or if the
            // image found was a fallback to another region than the one requested. If it was
            // a fallback to another region then it will only get replaced with the wheel-hd
            // image if that is matching the requested region.
            if (regionFallback || result.marqueeUrl == "") {
                std::string marqueeUrlTemp {result.marqueeUrl};
                if (processMedia(result, media_list, ssConfig.media_marquee_hd, result.marqueeUrl,
                                 result.marqueeFormat, region) &&
                    marqueeUrlTemp != "") {
                    result.marqueeUrl = marqueeUrlTemp;
                }
            }
            // Physical media.
            processMedia(result, media_list, ssConfig.media_physicalmedia, result.physicalmediaUrl,
                         result.physicalmediaFormat, region);
            // Screenshot.
            processMedia(result, media_list, ssConfig.media_screenshot, result.screenshotUrl,
                         result.screenshotFormat, region);
            // Title screen.
            processMedia(result, media_list, ssConfig.media_titlescreen, result.titlescreenUrl,
                         result.titlescreenFormat, region);
            // Video.
            processMedia(result, media_list, ssConfig.media_video, result.videoUrl,
                         result.videoFormat, region);
            // Fallback to normalized video if no regular video was found.
            if (result.videoUrl == "")
                processMedia(result, media_list, ssConfig.media_video_normalized, result.videoUrl,
                             result.videoFormat, region);
        }
        result.mediaURLFetch = COMPLETED;
        out_results.emplace_back(result);
    } // Game.

    if (out_results.size() == 0) {
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): No games found";
    }
}

bool ScreenScraperRequest::processMedia(ScraperSearchResult& result,
                                        const pugi::xml_node& media_list,
                                        std::string& mediaType,
                                        std::string& fileURL,
                                        std::string& fileFormat,
                                        const std::string& region)
{
    pugi::xml_node art {pugi::xml_node(nullptr)};
    bool regionFallback {false};

    // Do an XPath query for media[type='$media_type'], then filter by region.
    // We need to do this because any child of 'medias' has the form
    // <media type="..." region="..." format="...">
    // and we need to find the right media for the region.
    pugi::xpath_node_set results {media_list.select_nodes(
        (static_cast<std::string>("media[@type='") + mediaType + "']").c_str())};

    if (results.size()) {
        // Videos and fan art don't have any region attributes, so just take the first entry
        // (which should be the only entry as well).
        if (mediaType == "video" || mediaType == "video-normalized" || mediaType == "fanart") {
            art = results.first().node();
        }
        else {
            std::string otherRegion;
            if (Settings::getInstance()->getBool("ScraperRegionFallback")) {
                // In case none of the regular fallback regions are found, try whatever is the
                // first region in the returned results. This should capture games only released
                // for specific countries and such as well as invalid database entries where the
                // wrong region was defined. This fallback also includes the ss/ScreenScraper
                // region which adds media for unofficial games (e.g. for OpenBOR and PICO-8).
                otherRegion = results.first().node().attribute("region").as_string();
            }
            // Region fallback: world, USA, Japan, EU and custom.
            for (auto regionEntry :
                 std::vector<std::string> {region, "wor", "us", "jp", "eu", "cus", otherRegion}) {
                if (art)
                    break;

                for (auto node : results) {
                    if (node.node().attribute("region").value() == regionEntry) {
                        art = node.node();
                        if (region != regionEntry)
                            regionFallback = true;
                        break;
                    }
                }
            }
        }
    }

    if (art) {
        // Sending a 'softname' containing space will make the media URLs returned
        // by the API also contain the space. Escape any spaces in the URL here.
        fileURL = Utils::String::replace(art.text().get(), " ", "%20");

        // Get the media type returned by ScreenScraper.
        std::string media_type {art.attribute("format").value()};
        if (!media_type.empty())
            fileFormat = "." + media_type;
    }
    else {
        LOG(LogDebug) << "ScreenScraperRequest::processMedia(): "
                         "Failed to find media XML node with name '"
                      << mediaType << "'";
    }

    return regionFallback;
}

// Currently not used in this module.
void ScreenScraperRequest::processList(const pugi::xml_document& xmldoc,
                                       std::vector<ScraperSearchResult>& results)
{
    assert(mRequestQueue != nullptr);

    LOG(LogDebug) << "ScreenScraperRequest::processList(): Processing a list of results";

    pugi::xml_node data {xmldoc.child("Data")};
    pugi::xml_node game {data.child("jeu")};

    if (!game) {
        LOG(LogDebug) << "ScreenScraperRequest::processList(): Found nothing";
    }

    ScreenScraperRequest::ScreenScraperConfig ssConfig;

    // Limit the number of results per platform, not in total.
    // Otherwise if the first platform returns >= 7 games
    // but the second platform contains the relevant game,
    // the relevant result would not be shown.
    for (int i {0}; game && i < MAX_SCRAPER_RESULTS; ++i) {
        std::string id {game.child("id").text().get()};
        std::string name {game.child("nom").text().get()};
        std::string platformId {game.child("systemeid").text().get()};
        std::string path {ssConfig.getGameSearchUrl(name) + "&systemeid=" + platformId +
                          "&gameid=" + id};

        mRequestQueue->push(
            std::unique_ptr<ScraperRequest>(new ScreenScraperRequest(results, path)));

        game = game.next_sibling("jeu");
    }
}

std::string ScreenScraperRequest::ScreenScraperConfig::getGameSearchUrl(
    const std::string gameName) const
{
    std::string searchName {gameName};
    bool singleSearch {false};

    // Trim leading and trailing whitespaces.
    searchName = Utils::String::trim(searchName);

    if (Settings::getInstance()->getBool("ScraperConvertUnderscores"))
        searchName = Utils::String::replace(searchName, "_", " ");

    // If only whitespaces were entered as the search string, then search using a random string
    // that will not return any results. This is a quick and dirty way to avoid french error
    // messages about malformed URLs that would surely confuse the user.
    if (searchName == "")
        searchName = "zzzzzz";

    // If the game is an arcade game and we're not searching using the metadata name, then
    // search using the individual ROM name rather than running a wider text matching search.
    // Also run this search mode if the game name is shorter than four characters, as
    // screenscraper.fr will otherwise throw an error that the necessary search parameters
    // were not provided with the search. Possibly this is because a search using less than
    // four characters would return too many results. But there are some games with really
    // short names, so it's annoying that they can't be searched using this method.
    if (isArcadeSystem && !Settings::getInstance()->getBool("ScraperSearchMetadataName")) {
        singleSearch = true;
    }
    else if (searchName.size() < 4) {
        singleSearch = true;
    }
    else if (searchName.back() == '+') {
        // Special case where ScreenScraper will apparently strip trailing plus characters
        // from the search strings, and if we don't handle this we could end up with less
        // than four characters which would break the wide search.
        std::string trimTrailingPluses {searchName};
        trimTrailingPluses.erase(std::find_if(trimTrailingPluses.rbegin(),
                                              trimTrailingPluses.rend(),
                                              [](char c) { return c != '+'; })
                                     .base(),
                                 trimTrailingPluses.end());

        if (trimTrailingPluses.size() < 4)
            singleSearch = true;
    }
    // Another issue is that ScreenScraper removes the word "the" from the search string, which
    // could also lead to an error for short game names.
    if (!singleSearch) {
        std::string removeThe {
            Utils::String::replace(Utils::String::toUpper(searchName), "THE ", "")};
        // Any additional spaces must also be removed.
        removeThe.erase(removeThe.begin(),
                        std::find_if(removeThe.begin(), removeThe.end(), [](char c) {
                            return !std::isspace(static_cast<unsigned char>(c));
                        }));
        // If "the" is placed at the end of the search string, ScreenScraper also removes it.
        if (removeThe.size() > 4) {
            if (removeThe.substr(removeThe.size() - 4, 4) == " THE")
                removeThe = removeThe.substr(0, removeThe.size() - 4);
        }
        if (removeThe.size() < 4)
            singleSearch = true;
    }

    std::string screenScraperURL;

    if (automaticMode || singleSearch) {
        if (Settings::getInstance()->getBool("ScraperAutomaticRemoveDots"))
            searchName = Utils::String::replace(searchName, ".", "");
        screenScraperURL.append(API_URL_BASE)
            .append("/jeuInfos.php?devid=")
            .append(Utils::String::scramble(API_DEV_U, API_DEV_KEY))
            .append("&devpassword=")
            .append(Utils::String::scramble(API_DEV_P, API_DEV_KEY))
            .append("&softname=")
            .append(HttpReq::urlEncode(API_SOFT_NAME))
            .append("&output=xml")
            .append("&romnom=")
            .append(HttpReq::urlEncode(searchName));
    }
    else {
        screenScraperURL.append(API_URL_BASE)
            .append("/jeuRecherche.php?devid=")
            .append(Utils::String::scramble(API_DEV_U, API_DEV_KEY))
            .append("&devpassword=")
            .append(Utils::String::scramble(API_DEV_P, API_DEV_KEY))
            .append("&softname=")
            .append(HttpReq::urlEncode(API_SOFT_NAME))
            .append("&output=xml")
            .append("&recherche=")
            .append(HttpReq::urlEncode(searchName));
    }

    // Username / password, if this has been setup and activated.
    if (Settings::getInstance()->getBool("ScraperUseAccountScreenScraper")) {
        const std::string username {
            Settings::getInstance()->getString("ScraperUsernameScreenScraper")};
        const std::string password {
            Settings::getInstance()->getString("ScraperPasswordScreenScraper")};
        if (!username.empty() && !password.empty()) {
            screenScraperURL.append("&ssid=")
                .append(HttpReq::urlEncode(username))
                .append("&sspassword=")
                .append(HttpReq::urlEncode(password));
        }
    }

    return screenScraperURL;
}

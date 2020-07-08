//
//  ScreenScraper.cpp
//
//  Functions specifically for scraping from screenscraper.fr
//  Called from Scraper.
//

#include "scrapers/ScreenScraper.h"

#include "utils/TimeUtil.h"
#include "utils/StringUtil.h"
#include "FileData.h"
#include "Log.h"
#include "PlatformId.h"
#include "Settings.h"
#include "SystemData.h"
#include "math/Misc.h"
#include <pugixml.hpp>
#include <cstring>

using namespace PlatformIds;

// List of systems and their IDs from:
// https://www.screenscraper.fr/api/systemesListe.php?devid=xxx&devpassword=yyy&softname=zzz&output=XML
const std::map<PlatformId, unsigned short> screenscraper_platformid_map {
    { THREEDO, 29 },
    { AMIGA, 64 },
    { AMSTRAD_CPC, 65 },
    { APPLE_II, 86 },
    { ARCADE, 75 },
    { ATARI_800, 26 }, // Use ATARI_2600 as an alias for atari 800.
    { ATARI_2600, 26 },
    { ATARI_5200, 40 },
    { ATARI_7800, 41 },
    { ATARI_JAGUAR, 27 },
    { ATARI_JAGUAR_CD, 171 },
    { ATARI_LYNX, 28 },
    { ATARI_ST, 42},
    // Missing Atari XE ?
    { COLECOVISION, 48 },
    { COMMODORE_64, 66 },
    { INTELLIVISION, 115 },
    { MAC_OS, 146 },
    { XBOX, 32 },
    { XBOX_360, 33 },
    { MSX, 113 },
    { NEOGEO, 142 },
    { NEOGEO_POCKET, 25},
    { NEOGEO_POCKET_COLOR, 82 },
    { NINTENDO_3DS, 17 },
    { NINTENDO_64, 14 },
    { NINTENDO_DS, 15 },
    { FAMICOM_DISK_SYSTEM, 106 },
    { NINTENDO_ENTERTAINMENT_SYSTEM, 3 },
    { FAIRCHILD_CHANNELF, 80 },
    { GAME_BOY, 9 },
    { GAME_BOY_ADVANCE, 12 },
    { GAME_BOY_COLOR, 10 },
    { NINTENDO_GAMECUBE, 13 },
    { NINTENDO_WII, 16 },
    { NINTENDO_WII_U, 18 },
    { NINTENDO_VIRTUAL_BOY, 11 },
    { NINTENDO_GAME_AND_WATCH, 52 },
    { PC, 135 },
    { OPENBOR, 214 },
    { SCUMMVM, 123},
    { SEGA_32X, 19 },
    { SEGA_CD, 20 },
    { SEGA_DREAMCAST, 23 },
    { SEGA_GAME_GEAR, 21 },
    { SEGA_GENESIS, 1 },
    { SEGA_MASTER_SYSTEM, 2 },
    { SEGA_MEGA_DRIVE, 1 },
    { SEGA_SATURN, 22 },
    { SEGA_SG1000, 109 },
    { SHARP_X6800, 79},
    { SOLARUS, 223 },
    { PLAYSTATION, 57 },
    { PLAYSTATION_2, 58 },
    { PLAYSTATION_3, 59 },
    // Missing Sony Playstation 4 ?
    { PLAYSTATION_VITA, 62 },
    { PLAYSTATION_PORTABLE, 61 },
    { SUPER_NINTENDO, 4 },
    { TURBOGRAFX_16, 31 },
    { TURBOGRAFX_CD, 114 },
    { WONDERSWAN, 45 },
    { WONDERSWAN_COLOR, 46 },
    { ZX_SPECTRUM, 76 },
    { ZX81_SINCLAR, 77 },
    { VIDEOPAC_ODYSSEY2, 104 },
    { VECTREX, 102 },
    { TRS80_COLOR_COMPUTER, 144 },
    { TANDY, 144 }
};

// Helper XML parsing method, finding a node-by-name recursively.
pugi::xml_node find_node_by_name_re(const pugi::xml_node& node,
        const std::vector<std::string> node_names) {

    for (const std::string& _val : node_names) {
        pugi::xpath_query query_node_name((static_cast<std::string>("//") + _val).c_str());
        pugi::xpath_node_set results = node.select_nodes(query_node_name);

        if (results.size() > 0)
            return results.first().node();
    }

    return pugi::xml_node();
}

// Help XML parsing method, finding an direct child XML node starting from the parent and
// filtering by an attribute value list.
pugi::xml_node find_child_by_attribute_list(const pugi::xml_node& node_parent,
        const std::string& node_name, const std::string& attribute_name,
        const std::vector<std::string> attribute_values)
{
    for (auto _val : attribute_values) {
        for (pugi::xml_node node : node_parent.children(node_name.c_str())) {
            if (strcmp(node.attribute(attribute_name.c_str()).value(), _val.c_str()) == 0)
                return node;
        }
    }

    return pugi::xml_node(nullptr);
}

void screenscraper_generate_scraper_requests(const ScraperSearchParams& params,
        std::queue< std::unique_ptr<ScraperRequest> >& requests,
        std::vector<ScraperSearchResult>& results)
{
    std::string path;

    ScreenScraperRequest::ScreenScraperConfig ssConfig;

    if (params.nameOverride == "")
        path = ssConfig.getGameSearchUrl(params.game->getCleanName());
    else
        path = ssConfig.getGameSearchUrl(params.nameOverride);

    auto& platforms = params.system->getPlatformIds();
    std::vector<unsigned short> p_ids;

    // Get the IDs of each platform from the ScreenScraper list.
    for (auto platformIt = platforms.cbegin(); platformIt != platforms.cend(); platformIt++) {
        auto mapIt = screenscraper_platformid_map.find(*platformIt);

        if (mapIt != screenscraper_platformid_map.cend()) {
            p_ids.push_back(mapIt->second);
        }
        else {
            LOG(LogWarning) << "Warning - ScreenScraper: no support for platform " <<
                    getPlatformName(*platformIt);
            // Add the scrape request without a platform/system ID.
            requests.push(std::unique_ptr<ScraperRequest>
                    (new ScreenScraperRequest(requests, results, path)));
        }
    }

    // Sort the platform IDs and remove duplicates.
    std::sort(p_ids.begin(), p_ids.end());
    auto last = std::unique(p_ids.begin(), p_ids.end());
    p_ids.erase(last, p_ids.end());

    for (auto platform = p_ids.cbegin(); platform != p_ids.cend(); platform++) {
        path += "&systemeid=";
        path += HttpReq::urlEncode(std::to_string(*platform));
        requests.push(std::unique_ptr<ScraperRequest>
                (new ScreenScraperRequest(requests, results, path)));
    }
}

void ScreenScraperRequest::process(const std::unique_ptr<HttpReq>& req,
        std::vector<ScraperSearchResult>& results)
{
    assert(req->status() == HttpReq::REQ_SUCCESS);

    pugi::xml_document doc;
    pugi::xml_parse_result parseResult = doc.load_string(req->getContent().c_str());

    if (!parseResult) {
        std::stringstream ss;
        ss << "Error - ScreenScraperRequest - Error parsing XML: " << parseResult.description();

        std::string err = ss.str();
        setError(err);
        LOG(LogError) << err;

        return;
    }

    processGame(doc, results);
}

void ScreenScraperRequest::processGame(const pugi::xml_document& xmldoc,
        std::vector<ScraperSearchResult>& out_results)
{
    pugi::xml_node data = xmldoc.child("Data");
    pugi::xml_node game = data.child("jeu");

    if (game) {
        ScraperSearchResult result;
        ScreenScraperRequest::ScreenScraperConfig ssConfig;

        result.gameID = game.attribute("id").as_string();

        // Find how many more requests we can make before the scraper request
        // allowance counter is reset.
        unsigned requestsToday =
                data.child("ssuser").child("requeststoday").text().as_uint();
        unsigned maxRequestsPerDay =
                data.child("ssuser").child("maxrequestsperday").text().as_uint();
        result.scraperRequestAllowance = maxRequestsPerDay - requestsToday;

        std::string region =
                Utils::String::toLower(Settings::getInstance()->getString("ScraperRegion"));
        std::string language =
                Utils::String::toLower(Settings::getInstance()->getString("ScraperLanguage"));

        // Name fallback: US, WOR(LD). ( Xpath: Data/jeu[0]/noms/nom[*] ).
        result.mdl.set("name", find_child_by_attribute_list(game.child("noms"),
                "nom", "region", { region, "wor", "us" , "ss", "eu", "jp" }).text().get());

        // Description fallback language: EN, WOR(LD).
        std::string description = find_child_by_attribute_list(game.child("synopsis"),
                "synopsis", "langue", { language, "en", "wor" }).text().get();

        if (!description.empty())
            result.mdl.set("desc", Utils::String::replace(description, "&nbsp;", " "));

        // Genre fallback language: EN. ( Xpath: Data/jeu[0]/genres/genre[*] ).
        result.mdl.set("genre", find_child_by_attribute_list(game.child("genres"),
                "genre", "langue", { language, "en" }).text().get());

        // Get the date proper. The API returns multiple 'date' children nodes to the 'dates'
        // main child of 'jeu'.
        // Date fallback: WOR(LD), US, SS, JP, EU.
        std::string _date = find_child_by_attribute_list(game.child("dates"), "date", "region",
                { region, "wor", "us", "ss", "jp", "eu" }).text().get();

        // Date can be YYYY-MM-DD or just YYYY.
        if (_date.length() > 4) {
            result.mdl.set("releasedate", Utils::Time::DateTime(
                    Utils::Time::stringToTime(_date, "%Y-%m-%d")));
        }
        else if (_date.length() > 0) {
            result.mdl.set("releasedate", Utils::Time::DateTime(
                    Utils::Time::stringToTime(_date, "%Y")));
        }

        /// Developer for the game( Xpath: Data/jeu[0]/developpeur ).
        std::string developer = game.child("developpeur").text().get();
        if (!developer.empty())
            result.mdl.set("developer", Utils::String::replace(developer, "&nbsp;", " "));

        // Publisher for the game ( Xpath: Data/jeu[0]/editeur ).
        std::string publisher = game.child("editeur").text().get();
        if (!publisher.empty())
            result.mdl.set("publisher", Utils::String::replace(publisher, "&nbsp;", " "));

        // Players.
        result.mdl.set("players", game.child("joueurs").text().get());

        // Validate rating.
        if (Settings::getInstance()->getBool("ScrapeRatings") && game.child("note")) {
            float ratingVal = (game.child("note").text().as_int() / 20.0f);
            // Round up to the closest .1 value, i.e. to the closest half-star.
            ratingVal = Math::ceilf(ratingVal / 0.1) / 10;
            std::stringstream ss;
            ss << ratingVal;
            result.mdl.set("rating", ss.str());
        }

        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Name: " <<
                result.mdl.get("name");
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Release Date (unparsed): " <<
                _date;
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Release Date (parsed): " <<
                result.mdl.get("releasedate");
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Developer: " <<
                result.mdl.get("developer");
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Publisher: " <<
                result.mdl.get("publisher");
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Genre: " <<
                result.mdl.get("genre");
        LOG(LogDebug) << "ScreenScraperRequest::processGame(): Players: " <<
                result.mdl.get("players");

        // Media super-node.
        pugi::xml_node media_list = game.child("medias");

        if (media_list) {
            // 3D box
            processMedia(result, media_list, ssConfig.media_3dbox,
                    result.box3dUrl, result.box3dFormat, region);
            // Cover
            processMedia(result, media_list, ssConfig.media_cover,
                    result.coverUrl, result.coverFormat, region);
            // Marquee (wheel)
            processMedia(result, media_list, ssConfig.media_marquee,
                    result.marqueeUrl, result.marqueeFormat, region);
            // Screenshot
            processMedia(result, media_list, ssConfig.media_screenshot,
                    result.screenshotUrl, result.screenshotFormat, region);
        }
        result.mediaURLFetch = COMPLETED;
        out_results.push_back(result);
    } // Game.
}

void ScreenScraperRequest::processMedia(
        ScraperSearchResult& result,
        const pugi::xml_node& media_list,
        std::string mediaType,
        std::string& fileURL,
        std::string& fileFormat,
        std::string region)
{
    pugi::xml_node art = pugi::xml_node(nullptr);

    // Do an XPath query for media[type='$media_type'], then filter by region.
    // We need to do this because any child of 'medias' has the form
    // <media type="..." region="..." format="...">
    // and we need to find the right media for the region.
    pugi::xpath_node_set results = media_list.select_nodes((static_cast<std::string>
        ("media[@type='") + mediaType + "']").c_str());

        if (results.size()) {
            // Region fallback: WOR(LD), US, CUS(TOM?), JP, EU.
            for (auto _region : std::vector<std::string>{
                    region, "wor", "us", "cus", "jp", "eu" }) {
                if (art)
                    break;

                for (auto node : results) {
                    if (node.node().attribute("region").value() == _region) {
                        art = node.node();
                        break;
                    }
                }
            }
        }

        if (art) {
            // Sending a 'softname' containing space will make the image URLs returned
            // by the API also contain the space. Escape any spaces in the URL here.
            fileURL = Utils::String::replace(art.text().get(), " ", "%20");

            // Get the media type returned by ScreenScraper.
            std::string media_type = art.attribute("format").value();
            if (!media_type.empty())
                fileFormat = "." + media_type;
        }
        else {
            LOG(LogDebug) << "Failed to find media XML node with name=" << mediaType;
        }
}

// Currently not used in this module.
void ScreenScraperRequest::processList(const pugi::xml_document& xmldoc,
        std::vector<ScraperSearchResult>& results)
{
    assert(mRequestQueue != nullptr);

    LOG(LogDebug) << "ScreenScraper: Processing a list of results";

    pugi::xml_node data = xmldoc.child("Data");
    pugi::xml_node game = data.child("jeu");

    if (!game) {
        LOG(LogDebug) << "ScreenScraper: Found nothing";
    }

    ScreenScraperRequest::ScreenScraperConfig ssConfig;

    // Limit the number of results per platform, not in total.
    // Otherwise if the first platform returns >= 7 games
    // but the second platform contains the relevant game,
    // the relevant result would not be shown.
    for (int i = 0; game && i < MAX_SCRAPER_RESULTS; i++) {
        std::string id = game.child("id").text().get();
        std::string name = game.child("nom").text().get();
        std::string platformId = game.child("systemeid").text().get();
        std::string path = ssConfig.getGameSearchUrl(name) + "&systemeid=" +
                platformId + "&gameid=" + id;

        mRequestQueue->push(std::unique_ptr<ScraperRequest>
                (new ScreenScraperRequest(results, path)));

        game = game.next_sibling("jeu");
    }
}

std::string ScreenScraperRequest::ScreenScraperConfig::getGameSearchUrl(
        const std::string gameName) const
{
    return API_URL_BASE
        + "/jeuInfos.php?devid=" + Utils::String::scramble(API_DEV_U, API_DEV_KEY)
        + "&devpassword=" + Utils::String::scramble(API_DEV_P, API_DEV_KEY)
        + "&softname=" + HttpReq::urlEncode(API_SOFT_NAME)
        + "&output=xml"
        + "&romnom=" + HttpReq::urlEncode(gameName);
}

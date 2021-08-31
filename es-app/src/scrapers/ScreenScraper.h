//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ScreenScraper.h
//
//  Functions specifically for scraping from screenscraper.fr
//  Called from Scraper.
//

#ifndef ES_APP_SCRAPERS_SCREEN_SCRAPER_H
#define ES_APP_SCRAPERS_SCREEN_SCRAPER_H

#include "EmulationStation.h"
#include "scrapers/Scraper.h"

namespace pugi
{
    class xml_document;
}

void screenscraper_generate_scraper_requests(const ScraperSearchParams& params,
                                             std::queue<std::unique_ptr<ScraperRequest>>& requests,
                                             std::vector<ScraperSearchResult>& results);

class ScreenScraperRequest : public ScraperHttpRequest
{
public:
    // ctor for a GetGameList request.
    ScreenScraperRequest(std::queue<std::unique_ptr<ScraperRequest>>& requestsWrite,
                         std::vector<ScraperSearchResult>& resultsWrite,
                         const std::string& url)
        : ScraperHttpRequest(resultsWrite, url)
        , mRequestQueue(&requestsWrite)
    {
    }

    // ctor for a GetGame request.
    ScreenScraperRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url)
        : ScraperHttpRequest(resultsWrite, url)
        , mRequestQueue(nullptr)
    {
    }

    // Settings for the scraper.
    static const struct ScreenScraperConfig {
        std::string getGameSearchUrl(const std::string gameName) const;

        // Access to the API.
        const std::string API_DEV_U = {15, 21, 39, 22, 42, 40};
        const std::string API_DEV_P = {32, 70, 46, 54, 12, 5, 13, 120, 50, 66, 25};
        const std::string API_DEV_KEY = {67, 112, 72, 120, 121, 77, 119, 74,  84,  56,
                                         75, 122, 78, 98,  69,  86, 56,  120, 120, 49};
        const std::string API_URL_BASE = "https://www.screenscraper.fr/api2";
        const std::string API_SOFT_NAME =
            "EmulationStation-DE " + static_cast<std::string>(PROGRAM_VERSION_STRING);

        // Which type of image artwork we need. Possible values (not a comprehensive list):
        // - ss: in-game screenshot
        // - box-3D: 3D boxart
        // - box-2D: 2D boxart
        // - screenmarque : marquee
        // - sstitle: in-game start screenshot
        // - steamgrid: Steam artwork
        // - wheel: spine
        // - support-2D: media showing the 2d boxart on the cart
        // - support-3D: media showing the 3d boxart on the cart
        // - video: gameplay videos
        // - video-normalized: gameplay videos in smaller file sizes with lower audio quality
        //
        // Note that not all games contain values for all these, so we default to "ss".
        //

        std::string media_3dbox = "box-3D";
        std::string media_cover = "box-2D";
        std::string media_marquee = "wheel";
        std::string media_screenshot = "ss";
        std::string media_video = "video";

        bool isArcadeSystem;

        // Which Region to use when selecting the artwork.
        // Applies to: artwork, name of the game, date of release.
        // This is read from es_settings.xml, setting "ScraperRegion".

        // Which Language to use when selecting the textual information.
        // Applies to: description, genre.
        // This is read from es_settings.xml, setting "ScraperLanguage".

        ScreenScraperConfig() {}
    } configuration;

protected:
    void process(const std::unique_ptr<HttpReq>& req,
                 std::vector<ScraperSearchResult>& results) override;

    void processList(const pugi::xml_document& xmldoc, std::vector<ScraperSearchResult>& results);
    void processGame(const pugi::xml_document& xmldoc, std::vector<ScraperSearchResult>& results);
    void processMedia(ScraperSearchResult& result,
                      const pugi::xml_node& media_list,
                      std::string mediaType,
                      std::string& fileURL,
                      std::string& fileFormat,
                      std::string region);
    bool isGameRequest() { return !mRequestQueue; }

    std::queue<std::unique_ptr<ScraperRequest>>* mRequestQueue;
};

#endif // ES_APP_SCRAPERS_SCREEN_SCRAPER_H

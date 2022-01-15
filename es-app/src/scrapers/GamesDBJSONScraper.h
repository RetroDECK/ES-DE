//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamesDBJSONScraper.h
//
//  Functions specifically for scraping from thegamesdb.net
//  Called from Scraper.
//

#ifndef ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_H
#define ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_H

#include "scrapers/Scraper.h"

namespace pugi
{
    class xml_document;
}

void thegamesdb_generate_json_scraper_requests(
    const ScraperSearchParams& params,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results);

void thegamesdb_generate_json_scraper_requests(
    const std::string& gameIDs,
    std::queue<std::unique_ptr<ScraperRequest>>& requests,
    std::vector<ScraperSearchResult>& results);

class TheGamesDBJSONRequest : public ScraperHttpRequest
{
public:
    // Constructor for a GetGamelist request.
    TheGamesDBJSONRequest(std::queue<std::unique_ptr<ScraperRequest>>& requestsWrite,
                          std::vector<ScraperSearchResult>& resultsWrite,
                          const std::string& url)
        : ScraperHttpRequest(resultsWrite, url)
        , mRequestQueue(&requestsWrite)
    {
    }
    // Constructior for a GetGame request
    TheGamesDBJSONRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url)
        : ScraperHttpRequest(resultsWrite, url)
        , mRequestQueue(nullptr)
    {
    }

protected:
    void process(const std::unique_ptr<HttpReq>& req,
                 std::vector<ScraperSearchResult>& results) override;
    bool isGameRequest() { return !mRequestQueue; }

    std::queue<std::unique_ptr<ScraperRequest>>* mRequestQueue;
};

#endif // ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_H

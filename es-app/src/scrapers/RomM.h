//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomM.h
//
//  Functions for scraping from a self-hosted RomM (https://github.com/rommapp/romm) instance.
//  Called from Scraper.
//

#ifndef ES_APP_SCRAPERS_ROMM_H
#define ES_APP_SCRAPERS_ROMM_H

#include "scrapers/Scraper.h"

void romm_generate_scraper_requests(const ScraperSearchParams& params,
                                    std::queue<std::unique_ptr<ScraperRequest>>& requests,
                                    std::vector<ScraperSearchResult>& results);

class RomMRequest : public ScraperHttpRequest
{
public:
    RomMRequest(std::vector<ScraperSearchResult>& resultsWrite,
               const std::string& url,
               const std::string& bearerToken)
        : ScraperHttpRequest(resultsWrite, url, bearerToken)
    {
    }

protected:
    void process(const std::unique_ptr<HttpReq>& req,
                std::vector<ScraperSearchResult>& results) override;
};

#endif // ES_APP_SCRAPERS_ROMM_H

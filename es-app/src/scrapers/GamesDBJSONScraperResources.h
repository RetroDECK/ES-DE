//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GamesDBJSONScraperResources.h
//
//  Functions specifically for scraping from thegamesdb.net
//  Called from GamesDBJSONScraper.
//
//  Downloads these resource files to the scrapers folder in the application data directory:
//  gamesdb_developers.json
//  gamesdb_genres.json
//  gamesdb_publishers.json
//

#ifndef ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_RESOURCES_H
#define ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_RESOURCES_H

#include "HttpReq.h"

#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>

struct TheGamesDBJSONRequestResources {
    TheGamesDBJSONRequestResources() = default;

    void prepare();
    void ensureResources();
    std::string getApiKey() const;

    std::unordered_map<int, std::string> gamesdb_new_developers_map;
    std::unordered_map<int, std::string> gamesdb_new_publishers_map;
    std::unordered_map<int, std::string> gamesdb_new_genres_map;

private:
    bool checkLoaded();

    bool saveResource(HttpReq* req,
                      std::unordered_map<int, std::string>& resource,
                      const std::string& resource_name,
                      const std::string& file_name);
    std::unique_ptr<HttpReq> fetchResource(const std::string& endpoint);

    int loadResource(std::unordered_map<int, std::string>& resource,
                     const std::string& resource_name,
                     const std::string& file_name);

    std::unique_ptr<HttpReq> gamesdb_developers_resource_request;
    std::unique_ptr<HttpReq> gamesdb_publishers_resource_request;
    std::unique_ptr<HttpReq> gamesdb_genres_resource_request;
};

std::string getScrapersResouceDir();

#endif // ES_APP_SCRAPERS_GAMES_DB_JSON_SCRAPER_RESOURCES_H

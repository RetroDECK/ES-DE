//
//  Scraper.h
//
//  Main scraper logic.
//  Called from GuiScraperSearch.
//  Calls either GamesDBJSONScraper or ScreenScraper.
//

#pragma once
#ifndef ES_APP_SCRAPERS_SCRAPER_H
#define ES_APP_SCRAPERS_SCRAPER_H

#include "AsyncHandle.h"
#include "HttpReq.h"
#include "MetaData.h"
#include <functional>
#include <memory>
#include <queue>
#include <utility>
#include <assert.h>

#define MAX_SCRAPER_RESULTS 7

class FileData;
class SystemData;

enum eDownloadStatus {
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED
};

struct ScraperSearchParams {
    SystemData* system;
    FileData* game;

    std::string nameOverride;
};

struct ScraperSearchResult {
    ScraperSearchResult() : mdl(GAME_METADATA) {};

    MetaDataList mdl;
    std::string gameID;

    // How many more objects the scraper service allows to be downloaded
    // within a given time period.
    unsigned int scraperRequestAllowance;

    enum eDownloadStatus mediaURLFetch = NOT_STARTED;
    enum eDownloadStatus thumbnailDownloadStatus = NOT_STARTED;
    enum eDownloadStatus mediaFilesDownloadStatus = NOT_STARTED;

    std::string ThumbnailImageData; // Thumbnail cache, will containe entire image.
    std::string ThumbnailImageUrl;

    std::string box3dUrl;
    std::string coverUrl;
    std::string marqueeUrl;
    std::string screenshotUrl;

    // Needed to pre-set the image type.
    std::string box3dFormat;
    std::string coverFormat;
    std::string marqueeFormat;
    std::string screenshotFormat;

    // Indicate whether any new images were downloaded and saved.
    bool savedNewImages;
};

// So let me explain why I've abstracted this so heavily.
// There are two ways I can think of that you'd want to write a scraper.

// 1. Do some HTTP request(s) -> process it -> return the results.
// 2. Do some local filesystem queries (an offline scraper) -> return the results.

// The first way needs to be asynchronous while it's waiting for the HTTP request to return.
// The second doesn't.

// It would be nice if we could write it like this:
// search = generate_http_request(searchparams);
// wait_until_done(search);
// ... process search ...
// return results;

// We could do this if we used threads.  Right now ES doesn't because I'm pretty sure I'll
// fuck it up, and I'm not sure of the performance of threads on the Pi (single-core ARM).
// We could also do this if we used coroutines.
// I can't find a really good cross-platform coroutine library (x86/64/ARM Linux + Windows),
// and I don't want to spend more time chasing libraries than just writing it the long way once.

// So, I did it the "long" way.
// ScraperSearchHandle - one logical search, e.g. "search for mario".
// ScraperRequest - encapsulates some sort of asynchronous request that will ultimately
// return some results.
// ScraperHttpRequest - implementation of ScraperRequest that waits on an HttpReq, then
// processes it with some processing function.

// A scraper search gathers results from (potentially multiple) ScraperRequests.
class ScraperRequest : public AsyncHandle
{
public:
    ScraperRequest(std::vector<ScraperSearchResult>& resultsWrite);

    // Returns "true" once we're done.
    virtual void update() = 0;

protected:
    std::vector<ScraperSearchResult>& mResults;
};

// A single HTTP request that needs to be processed to get the results.
class ScraperHttpRequest : public ScraperRequest
{
public:
    ScraperHttpRequest(std::vector<ScraperSearchResult>& resultsWrite, const std::string& url);
    virtual void update() override;

protected:
    virtual void process(const std::unique_ptr<HttpReq>& req,
            std::vector<ScraperSearchResult>& results) = 0;

private:
    std::unique_ptr<HttpReq> mReq;
};

// A request to get a list of results.
class ScraperSearchHandle : public AsyncHandle
{
public:
    ScraperSearchHandle();

    void update();
    inline const std::vector<ScraperSearchResult>& getResults() const {
                assert(mStatus != ASYNC_IN_PROGRESS); return mResults; }

protected:
    friend std::unique_ptr<ScraperSearchHandle>
            startScraperSearch(const ScraperSearchParams& params);

    friend std::unique_ptr<ScraperSearchHandle>
            startMediaURLsFetch(const std::string& gameIDs);

    std::queue< std::unique_ptr<ScraperRequest> > mRequestQueue;
    std::vector<ScraperSearchResult> mResults;
};

// Will use the current scraper settings to pick the result source.
std::unique_ptr<ScraperSearchHandle> startScraperSearch(const ScraperSearchParams& params);

std::unique_ptr<ScraperSearchHandle> startMediaURLsFetch(const std::string& gameIDs);

// Returns a list of valid scraper names.
std::vector<std::string> getScraperList();

// Returns true if the scraper configured in the settings is still valid.
bool isValidConfiguredScraper();

typedef void (*generate_scraper_requests_func)(const ScraperSearchParams& params,
        std::queue<std::unique_ptr<ScraperRequest>>& requests,
        std::vector<ScraperSearchResult>& results);

// -------------------------------------------------------------------------

// Meta data asset downloading stuff.
class MDResolveHandle : public AsyncHandle
{
public:
    MDResolveHandle(const ScraperSearchResult& result, const ScraperSearchParams& search);

    void update() override;
    inline const ScraperSearchResult& getResult() const
            { assert(mStatus == ASYNC_DONE); return mResult; }

private:
    ScraperSearchResult mResult;

    typedef std::pair<std::unique_ptr<AsyncHandle>, std::function<void()>> ResolvePair;
    std::vector<ResolvePair> mFuncs;
};

class ImageDownloadHandle : public AsyncHandle
{
public:
    ImageDownloadHandle(
            const std::string& url,
            const std::string& path,
            const std::string& existingMediaPath,
            bool& savedNewImage,
            int maxWidth,
            int maxHeight);

    void update() override;

private:
    std::unique_ptr<HttpReq> mReq;
    std::string mSavePath;
    std::string mExistingMediaFile;
    bool *mSavedNewImagePtr;
    int mMaxWidth;
    int mMaxHeight;
};

// Downloads to the home directory, using this subdirectory structure:
// ".emulationstation/downloaded_media/[system_name]/[media_type]/[game_name].[file_extension]".
// The subdirectories are automatically created if they do not exist.
std::string getSaveAsPath(const ScraperSearchParams& params,
        const std::string& filetypeSubdirectory, const std::string& url);

// Will resize according to Settings::getInt("ScraperResizeMaxWidth") and
// Settings::getInt("ScraperResizeMaxHeight").
std::unique_ptr<ImageDownloadHandle> downloadImageAsync(const std::string& url,
        const std::string& saveAs, const std::string& existingMediaPath, bool& savedNewImage);

// Resolves all metadata assets that need to be downloaded.
std::unique_ptr<MDResolveHandle> resolveMetaDataAssets(const ScraperSearchResult& result,
        const ScraperSearchParams& search);

// You can pass 0 for maxWidth or maxHeight to automatically keep the aspect ratio.
// It will overwrite the image at [path] with the new resized one.
// Returns true if successful, false otherwise.
bool resizeImage(const std::string& path, int maxWidth, int maxHeight);

#endif // ES_APP_SCRAPERS_SCRAPER_H

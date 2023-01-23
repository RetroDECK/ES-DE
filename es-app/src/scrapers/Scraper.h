//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Scraper.h
//
//  Main scraper logic.
//  Called from GuiScraperSearch.
//  Calls either GamesDBJSONScraper or ScreenScraper.
//

#ifndef ES_APP_SCRAPERS_SCRAPER_H
#define ES_APP_SCRAPERS_SCRAPER_H

#include "AsyncHandle.h"
#include "HttpReq.h"
#include "MetaData.h"
#include "PlatformId.h"

#include <assert.h>
#include <functional>
#include <memory>
#include <queue>
#include <utility>

#define MAX_SCRAPER_RESULTS 7

class FileData;
class SystemData;

enum downloadStatus {
    NOT_STARTED,
    IN_PROGRESS,
    COMPLETED
};

struct ScraperSearchParams {
    SystemData* system;
    FileData* game;

    std::string nameOverride;
    bool automaticMode;

    ScraperSearchParams()
        : automaticMode {false}
    {
    }
};

struct ScraperSearchResult {
    ScraperSearchResult()
        : mdl(GAME_METADATA)
        , scraperRequestAllowance {0}
        , mediaURLFetch {NOT_STARTED}
        , thumbnailDownloadStatus {NOT_STARTED}
        , mediaFilesDownloadStatus {NOT_STARTED}
        , savedNewMedia {false}
    {
    }

    MetaDataList mdl;
    std::string gameID;
    std::vector<PlatformIds::PlatformId> platformIDs;

    // How many more objects the scraper service allows to be downloaded
    // within a given time period.
    unsigned int scraperRequestAllowance;

    enum downloadStatus mediaURLFetch;
    enum downloadStatus thumbnailDownloadStatus;
    enum downloadStatus mediaFilesDownloadStatus;

    std::string thumbnailImageData; // Thumbnail cache, this will contain the entire image.
    std::string thumbnailImageUrl;

    std::string box3DUrl;
    std::string backcoverUrl;
    std::string coverUrl;
    std::string fanartUrl;
    std::string marqueeUrl;
    std::string physicalmediaUrl;
    std::string screenshotUrl;
    std::string titlescreenUrl;
    std::string videoUrl;

    // Needed to pre-set the image type.
    std::string box3DFormat;
    std::string backcoverFormat;
    std::string coverFormat;
    std::string fanartFormat;
    std::string marqueeFormat;
    std::string physicalmediaFormat;
    std::string screenshotFormat;
    std::string titlescreenFormat;
    std::string videoFormat;

    // Indicates whether any new media files were downloaded and saved.
    bool savedNewMedia;
};

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
    void update() override;

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
    ScraperSearchHandle() { setStatus(ASYNC_IN_PROGRESS); }

    void update();
    const std::vector<ScraperSearchResult>& getResults() const
    {
        assert(mStatus != ASYNC_IN_PROGRESS);
        return mResults;
    }

protected:
    friend std::unique_ptr<ScraperSearchHandle> startScraperSearch(
        const ScraperSearchParams& params);

    friend std::unique_ptr<ScraperSearchHandle> startMediaURLsFetch(const std::string& gameIDs);

    std::queue<std::unique_ptr<ScraperRequest>> mRequestQueue;
    std::vector<ScraperSearchResult> mResults;
};

// Will use the current scraper settings to pick the result source.
std::unique_ptr<ScraperSearchHandle> startScraperSearch(const ScraperSearchParams& params);

std::unique_ptr<ScraperSearchHandle> startMediaURLsFetch(const std::string& gameIDs);

// Returns a list of valid scraper names.
std::vector<std::string> getScraperList();

// Returns true if the scraper configured in the settings is still valid.
bool isValidConfiguredScraper();

using generate_scraper_requests_func =
    void (*)(const ScraperSearchParams& params,
             std::queue<std::unique_ptr<ScraperRequest>>& requests,
             std::vector<ScraperSearchResult>& results);

// -------------------------------------------------------------------------

// Meta data asset downloading stuff.
class MDResolveHandle : public AsyncHandle
{
public:
    MDResolveHandle(const ScraperSearchResult& result, const ScraperSearchParams& search);

    void update() override;
    const ScraperSearchResult& getResult() const
    {
        assert(mStatus == ASYNC_DONE);
        return mResult;
    }
    bool getSavedNewMedia() { return mResult.savedNewMedia; }

private:
    ScraperSearchResult mResult;

    using ResolvePair = std::pair<std::unique_ptr<AsyncHandle>, std::function<void()>>;
    std::vector<ResolvePair> mFuncs;
};

class MediaDownloadHandle : public AsyncHandle
{
public:
    MediaDownloadHandle(const std::string& url,
                        const std::string& path,
                        const std::string& existingMediaPath,
                        const std::string& mediaType,
                        const bool resizeFile,
                        bool& savedNewMedia);

    void update() override;

private:
    std::unique_ptr<HttpReq> mReq;
    std::string mSavePath;
    std::string mExistingMediaFile;
    std::string mMediaType;
    bool mResizeFile;
    bool* mSavedNewMediaPtr;
};

// Downloads to the home directory, using this subdirectory structure:
// ".emulationstation/downloaded_media/[system_name]/[media_type]/[game_name].[file_extension]".
// The subdirectories are automatically created if they do not exist.
std::string getSaveAsPath(const ScraperSearchParams& params,
                          const std::string& filetypeSubdirectory,
                          const std::string& url);

std::unique_ptr<MediaDownloadHandle> downloadMediaAsync(const std::string& url,
                                                        const std::string& saveAs,
                                                        const std::string& existingMediaPath,
                                                        const std::string& mediaType,
                                                        const bool resizeFile,
                                                        bool& savedNewMedia);

// Resolves all metadata assets that need to be downloaded.
std::unique_ptr<MDResolveHandle> resolveMetaDataAssets(const ScraperSearchResult& result,
                                                       const ScraperSearchParams& search);

bool resizeImage(const std::string& path, const std::string& mediaType);

#endif // ES_APP_SCRAPERS_SCRAPER_H

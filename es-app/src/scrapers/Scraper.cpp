//
//  Scraper.cpp
//
//  Main scraper logic.
//  Called from GuiScraperSearch.
//  Calls either GamesDBJSONScraper or ScreenScraper.
//

#include "scrapers/Scraper.h"

#include "FileData.h"
#include "GamesDBJSONScraper.h"
#include "ScreenScraper.h"
#include "Log.h"
#include "Settings.h"
#include "SystemData.h"
#include <FreeImage.h>
#include <fstream>

const std::map<std::string, generate_scraper_requests_func> scraper_request_funcs {
    { "TheGamesDB", &thegamesdb_generate_json_scraper_requests },
    { "ScreenScraper", &screenscraper_generate_scraper_requests }
};

std::unique_ptr<ScraperSearchHandle> startScraperSearch(const ScraperSearchParams& params)
{
    const std::string& name = Settings::getInstance()->getString("Scraper");
    std::unique_ptr<ScraperSearchHandle> handle(new ScraperSearchHandle());

    // Check if the scraper in the settings still exists as a registered scraping source.
    if (scraper_request_funcs.find(name) == scraper_request_funcs.end())
        LOG(LogWarning) << "Configured scraper (" << name << ") unavailable, scraping aborted.";
    else
        scraper_request_funcs.at(name)(params, handle->mRequestQueue, handle->mResults);

    return handle;
}

std::unique_ptr<ScraperSearchHandle> startMediaURLsFetch(const std::string& gameIDs)
{
    const std::string& name = Settings::getInstance()->getString("Scraper");
    std::unique_ptr<ScraperSearchHandle> handle(new ScraperSearchHandle());

    ScraperSearchParams params;
    // Check if the scraper in the settings still exists as a registered scraping source.
    if (scraper_request_funcs.find(name) == scraper_request_funcs.end())
        LOG(LogWarning) << "Configured scraper (" << name << ") unavailable, scraping aborted.";
    else
        // Specifically use the TheGamesDB function as this type of request
        // will never occur for ScreenScraper.
        thegamesdb_generate_json_scraper_requests(gameIDs, handle->mRequestQueue,
                handle->mResults);

    return handle;
}

std::vector<std::string> getScraperList()
{
    std::vector<std::string> list;
    for (auto it = scraper_request_funcs.cbegin(); it != scraper_request_funcs.cend(); it++)
        list.push_back(it->first);

    return list;
}

bool isValidConfiguredScraper()
{
    const std::string& name = Settings::getInstance()->getString("Scraper");
    return scraper_request_funcs.find(name) != scraper_request_funcs.end();
}

// ScraperSearchHandle.
ScraperSearchHandle::ScraperSearchHandle()
{
    setStatus(ASYNC_IN_PROGRESS);
}

void ScraperSearchHandle::update()
{
    if (mStatus == ASYNC_DONE)
        return;

    if (!mRequestQueue.empty()) {
        // A request can add more requests to the queue while running,
        // so be careful with references into the queue.
        auto& req = *(mRequestQueue.front());
        AsyncHandleStatus status = req.status();

        if (status == ASYNC_ERROR) {
            // Propagate error.
            setError(req.getStatusString());

            // Empty our queue.
            while (!mRequestQueue.empty())
                mRequestQueue.pop();

            return;
        }

        // Finished this one, see if we have any more.
        if (status == ASYNC_DONE)
            mRequestQueue.pop();

        // Status == ASYNC_IN_PROGRESS.
    }

    // We finished without any errors!
    if (mRequestQueue.empty()) {
        setStatus(ASYNC_DONE);
        return;
    }
}

// ScraperRequest.
ScraperRequest::ScraperRequest(std::vector<ScraperSearchResult>& resultsWrite)
        : mResults(resultsWrite)
{
}

// ScraperHttpRequest.
ScraperHttpRequest::ScraperHttpRequest(std::vector<ScraperSearchResult>& resultsWrite,
        const std::string& url) : ScraperRequest(resultsWrite)
{
    setStatus(ASYNC_IN_PROGRESS);
    mReq = std::unique_ptr<HttpReq>(new HttpReq(url));
}

void ScraperHttpRequest::update()
{
    HttpReq::Status status = mReq->status();
    if (status == HttpReq::REQ_SUCCESS) {
        // If process() has an error, status will be changed to ASYNC_ERROR.
        setStatus(ASYNC_DONE);
        process(mReq, mResults);
        return;
    }

    // Not ready yet.
    if (status == HttpReq::REQ_IN_PROGRESS)
        return;

    // Everything else is some sort of error.
    LOG(LogError) << "ScraperHttpRequest network error (status: " << status<< ") - "
            << mReq->getErrorMsg();
    setError(mReq->getErrorMsg());
}

// Download and write the media files to disk.
std::unique_ptr<MDResolveHandle> resolveMetaDataAssets(const ScraperSearchResult& result,
        const ScraperSearchParams& search)
{
    return std::unique_ptr<MDResolveHandle>(new MDResolveHandle(result, search));
}

MDResolveHandle::MDResolveHandle(const ScraperSearchResult& result,
        const ScraperSearchParams& search) : mResult(result)
{
    struct mediaFileInfoStruct {
        std::string fileURL;
        std::string fileFormat;
        std::string subDirectory;
        std::string existingMediaFile;
    } mediaFileInfo;

    std::vector<struct mediaFileInfoStruct> scrapeFiles;

    if (Settings::getInstance()->getBool("Scrape3DBoxes") && result.box3dUrl != "") {
        mediaFileInfo.fileURL = result.box3dUrl;
        mediaFileInfo.fileFormat = result.box3dFormat;
        mediaFileInfo.subDirectory = "3dboxes";
        mediaFileInfo.existingMediaFile = search.game->get3DBoxPath();
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeCovers") && result.coverUrl != "") {
        mediaFileInfo.fileURL = result.coverUrl;
        mediaFileInfo.fileFormat = result.coverFormat;
        mediaFileInfo.subDirectory = "covers";
        mediaFileInfo.existingMediaFile = search.game->getCoverPath();
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeMarquees") && result.marqueeUrl != "") {
        mediaFileInfo.fileURL = result.marqueeUrl;
        mediaFileInfo.fileFormat = result.marqueeFormat;
        mediaFileInfo.subDirectory = "marquees";
        mediaFileInfo.existingMediaFile = search.game->getMarqueePath();
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeScreenshots") && result.screenshotUrl != "") {
        mediaFileInfo.fileURL = result.screenshotUrl;
        mediaFileInfo.fileFormat = result.screenshotFormat;
        mediaFileInfo.subDirectory = "screenshots";
        mediaFileInfo.existingMediaFile = search.game->getScreenshotPath();
        scrapeFiles.push_back(mediaFileInfo);
    }

    for (auto it = scrapeFiles.cbegin(); it != scrapeFiles.cend(); it++) {

        std::string ext;

        // If we have a file extension returned by the scraper, then use it.
        // Otherwise, try to guess it by the name of the URL, which point to an image.
        if (!it->fileFormat.empty()) {
            ext = it->fileFormat;
        }
        else {
            size_t dot = it->fileURL.find_last_of('.');

            if (dot != std::string::npos)
                ext = it->fileURL.substr(dot, std::string::npos);
        }

        std::string filePath = getSaveAsPath(search, it->subDirectory, ext);

        // If there is an existing media file on disk and the setting to overwrite data
        // has been set to no, then don't proceed with downloading or saving a new file.
        if (it->existingMediaFile != "" &&
                !Settings::getInstance()->getBool("ScraperOverwriteData"))
            continue;

        // If the image is cached already as the thumbnail, then we don't need
        // to download it again, in this case just save it to disk and resize it.
        if (mResult.ThumbnailImageUrl == it->fileURL &&
                mResult.ThumbnailImageData.size() > 0) {

            // Remove any existing media file before attempting to write a new one.
            // This avoids the problem where there's already a file for this media type
            // with a different format/extension (e.g. game.jpg and we're going to write
            // game.png) which would lead to two media files for this game.
            if(it->existingMediaFile != "")
                Utils::FileSystem::removeFile(it->existingMediaFile);

            std::ofstream stream(filePath, std::ios_base::out | std::ios_base::binary);
            if (stream.bad()) {
                setError("Failed to open image path to write. Permission error? Disk full?");
                return;
            }

            const std::string& content = mResult.ThumbnailImageData;
            stream.write(content.data(), content.length());
            stream.close();
            if (stream.bad()) {
                setError("Failed to save image. Disk full?");
                return;
            }

            // Resize it.
            if (!resizeImage(filePath, Settings::getInstance()->getInt("ScraperResizeWidth"),
                    Settings::getInstance()->getInt("ScraperResizeHeight"))) {
                setError("Error saving resized image. Out of memory? Disk full?");
                return;
            }
        }
        // If it's not cached, then initiate the download.
        else {
            mFuncs.push_back(ResolvePair(downloadImageAsync(it->fileURL, filePath,
                    it->existingMediaFile), [this, filePath] {
            }));
        }
    }
}

void MDResolveHandle::update()
{
    if (mStatus == ASYNC_DONE || mStatus == ASYNC_ERROR)
        return;

    auto it = mFuncs.cbegin();
    while (it != mFuncs.cend()) {

        if (it->first->status() == ASYNC_ERROR) {
            setError(it->first->getStatusString());
            return;
        }
        else if (it->first->status() == ASYNC_DONE) {
            it->second();
            it = mFuncs.erase(it);
            continue;
        }
        it++;
    }

    if (mFuncs.empty())
        setStatus(ASYNC_DONE);
}

std::unique_ptr<ImageDownloadHandle> downloadImageAsync(const std::string& url,
        const std::string& saveAs, const std::string& existingMediaFile)
{
    return std::unique_ptr<ImageDownloadHandle>(new ImageDownloadHandle(
            url,
            saveAs,
            existingMediaFile,
            Settings::getInstance()->getInt("ScraperResizeWidth"),
            Settings::getInstance()->getInt("ScraperResizeHeight")));
}

ImageDownloadHandle::ImageDownloadHandle(
        const std::string& url,
        const std::string& path,
        const std::string& existingMediaPath,
        int maxWidth,
        int maxHeight)
        : mSavePath(path),
        mExistingMediaFile(existingMediaPath),
        mMaxWidth(maxWidth),
        mMaxHeight(maxHeight),
        mReq(new HttpReq(url))
{
}

void ImageDownloadHandle::update()
{
    if (mReq->status() == HttpReq::REQ_IN_PROGRESS)
        return;

    if (mReq->status() != HttpReq::REQ_SUCCESS) {
        std::stringstream ss;
        ss << "Network error: " << mReq->getErrorMsg();
        setError(ss.str());
        return;
    }

    // Download is done, save it to disk.

    // Remove any existing media file before attempting to write a new one.
    // This avoids the problem where there's already a file for this media type
    // with a different format/extension (e.g. game.jpg and we're going to write
    // game.png) which would lead to two media files for this game.
    if(mExistingMediaFile != "")
        Utils::FileSystem::removeFile(mExistingMediaFile);

    std::ofstream stream(mSavePath, std::ios_base::out | std::ios_base::binary);
    if (stream.bad()) {
        setError("Failed to open image path to write. Permission error? Disk full?");
        return;
    }

    const std::string& content = mReq->getContent();
    stream.write(content.data(), content.length());
    stream.close();
    if (stream.bad()) {
        setError("Failed to save image. Disk full?");
        return;
    }

    // Resize it.
    if (!resizeImage(mSavePath, mMaxWidth, mMaxHeight)) {
        setError("Error saving resized image. Out of memory? Disk full?");
        return;
    }

    setStatus(ASYNC_DONE);
}

// You can pass 0 for width or height to keep aspect ratio.
bool resizeImage(const std::string& path, int maxWidth, int maxHeight)
{
    // Nothing to do.
    if (maxWidth == 0 && maxHeight == 0)
        return true;

    FREE_IMAGE_FORMAT format = FIF_UNKNOWN;
    FIBITMAP* image = nullptr;

    // Detect the filetype.
    format = FreeImage_GetFileType(path.c_str(), 0);
    if (format == FIF_UNKNOWN)
        format = FreeImage_GetFIFFromFilename(path.c_str());
    if (format == FIF_UNKNOWN) {
        LOG(LogError) << "Error - could not detect filetype for image \"" << path << "\"!";
        return false;
    }

    // Make sure we can read this filetype first, then load it.
    if (FreeImage_FIFSupportsReading(format)) {
        image = FreeImage_Load(format, path.c_str());
    }
    else {
        LOG(LogError) << "Error - file format reading not supported for image \"" << path << "\"!";
        return false;
    }

    float width = (float)FreeImage_GetWidth(image);
    float height = (float)FreeImage_GetHeight(image);

    // If the image is smaller than maxWidth or maxHeight, then don't do any
    // scaling. It doesn't make sense to upscale the image and waste disk space.
    if (maxWidth > width || maxHeight > height)
        return true;

    if (maxWidth == 0)
        maxWidth = (int)((maxHeight / height) * width);
    else if (maxHeight == 0)
        maxHeight = (int)((maxWidth / width) * height);

    FIBITMAP* imageRescaled = FreeImage_Rescale(image, maxWidth, maxHeight, FILTER_BILINEAR);
    FreeImage_Unload(image);

    if (imageRescaled == nullptr) {
        LOG(LogError) << "Could not resize image! (not enough memory? invalid bitdepth?)";
        return false;
    }

    bool saved = (FreeImage_Save(format, imageRescaled, path.c_str()) != 0);
    FreeImage_Unload(imageRescaled);

    if (!saved)
        LOG(LogError) << "Failed to save resized image!";

    return saved;
}

std::string getSaveAsPath(const ScraperSearchParams& params,
        const std::string& filetypeSubdirectory, const std::string& extension)
{
    const std::string systemsubdirectory = params.system->getName();
    const std::string name = Utils::FileSystem::getStem(params.game->getPath());

    std::string path = FileData::getMediaDirectory();

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += systemsubdirectory + "/" + filetypeSubdirectory + "/";

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += name + extension;
    return path;
}

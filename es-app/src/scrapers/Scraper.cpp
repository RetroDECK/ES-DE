//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Scraper.cpp
//
//  Main scraper logic.
//  Called from GuiScraperSearch.
//  Calls either GamesDBJSONScraper or ScreenScraper.
//

#include "scrapers/Scraper.h"

#include "FileData.h"
#include "GamesDBJSONScraper.h"
#include "Log.h"
#include "ScreenScraper.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/StringUtil.h"

#if defined(_WIN64)
#include "views/ViewController.h"
#endif

#include <FreeImage.h>
#include <cmath>
#include <fstream>

namespace
{
    const std::map<std::string, generate_scraper_requests_func> scraper_request_funcs {
        {"thegamesdb", &thegamesdb_generate_json_scraper_requests},
        {"screenscraper", &screenscraper_generate_scraper_requests}};
}

std::unique_ptr<ScraperSearchHandle> startScraperSearch(const ScraperSearchParams& params)
{
    std::string name = Settings::getInstance()->getString("Scraper");
    // Handle a potentially invalid entry in the configuration file.
    if (name != "screenscraper" && name != "thegamesdb") {
        name = "screenscraper";
        Settings::getInstance()->setString("Scraper", name);
        Settings::getInstance()->saveFile();
    }

    std::unique_ptr<ScraperSearchHandle> handle(new ScraperSearchHandle());

    // Check if the scraper in the settings still exists as a registered scraping source.
    if (scraper_request_funcs.find(name) == scraper_request_funcs.end()) {
        LOG(LogError) << "Configured scraper (" << name << ") unavailable, scraping aborted";
    }
    else {
        LOG(LogDebug) << "Scraper::startScraperSearch(): Scraping system \""
                      << params.system->getName()
                      << (params.game->getType() == FOLDER ? "\", folder \"" : "\", game file \"")
                      << params.game->getFileName() << "\"";
        scraper_request_funcs.at(name)(params, handle->mRequestQueue, handle->mResults);
    }

    return handle;
}

std::unique_ptr<ScraperSearchHandle> startMediaURLsFetch(const std::string& gameIDs)
{
    const std::string& name = Settings::getInstance()->getString("Scraper");
    std::unique_ptr<ScraperSearchHandle> handle(new ScraperSearchHandle());

    ScraperSearchParams params;
    // Check if the scraper in the settings still exists as a registered scraping source.
    if (scraper_request_funcs.find(name) == scraper_request_funcs.end()) {
        LOG(LogWarning) << "Configured scraper (" << name << ") unavailable, scraping aborted";
    }
    else {
        // Specifically use the TheGamesDB function as this type of request
        // will never occur for ScreenScraper.
        thegamesdb_generate_json_scraper_requests(gameIDs, handle->mRequestQueue, handle->mResults);
    }

    return handle;
}

std::vector<std::string> getScraperList()
{
    std::vector<std::string> list;
    for (auto it = scraper_request_funcs.cbegin(); it != scraper_request_funcs.cend(); ++it)
        list.push_back(it->first);

    return list;
}

bool isValidConfiguredScraper()
{
    const std::string& name = Settings::getInstance()->getString("Scraper");
    return scraper_request_funcs.find(name) != scraper_request_funcs.end();
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

    // Check if we finished without any errors and if so set the status flag accordingly.
    if (mRequestQueue.empty() && mStatus != ASYNC_ERROR) {
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
                                       const std::string& url)
    : ScraperRequest(resultsWrite)
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
    LOG(LogError) << "ScraperHttpRequest network error (status: " << status << ") - "
                  << mReq->getErrorMsg();
    setError("Network error: " + mReq->getErrorMsg());
}

// Download and write the media files to disk.
std::unique_ptr<MDResolveHandle> resolveMetaDataAssets(const ScraperSearchResult& result,
                                                       const ScraperSearchParams& search)
{
    return std::unique_ptr<MDResolveHandle>(new MDResolveHandle(result, search));
}

MDResolveHandle::MDResolveHandle(const ScraperSearchResult& result,
                                 const ScraperSearchParams& search)
    : mResult(result)
{
    struct mediaFileInfoStruct {
        std::string fileURL;
        std::string fileFormat;
        std::string subDirectory;
        std::string existingMediaFile;
        bool resizeFile;
    } mediaFileInfo;

    std::vector<struct mediaFileInfoStruct> scrapeFiles;

    mResult.savedNewMedia = false;

    if (Settings::getInstance()->getBool("Scrape3DBoxes") && result.box3DUrl != "") {
        mediaFileInfo.fileURL = result.box3DUrl;
        mediaFileInfo.fileFormat = result.box3DFormat;
        mediaFileInfo.subDirectory = "3dboxes";
        mediaFileInfo.existingMediaFile = search.game->get3DBoxPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeBackCovers") && result.backcoverUrl != "") {
        mediaFileInfo.fileURL = result.backcoverUrl;
        mediaFileInfo.fileFormat = result.backcoverFormat;
        mediaFileInfo.subDirectory = "backcovers";
        mediaFileInfo.existingMediaFile = search.game->getBackCoverPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeCovers") && result.coverUrl != "") {
        mediaFileInfo.fileURL = result.coverUrl;
        mediaFileInfo.fileFormat = result.coverFormat;
        mediaFileInfo.subDirectory = "covers";
        mediaFileInfo.existingMediaFile = search.game->getCoverPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeFanArt") && result.fanartUrl != "") {
        mediaFileInfo.fileURL = result.fanartUrl;
        mediaFileInfo.fileFormat = result.fanartFormat;
        mediaFileInfo.subDirectory = "fanart";
        mediaFileInfo.existingMediaFile = search.game->getFanArtPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapePhysicalMedia") && result.physicalmediaUrl != "") {
        mediaFileInfo.fileURL = result.physicalmediaUrl;
        mediaFileInfo.fileFormat = result.physicalmediaFormat;
        mediaFileInfo.subDirectory = "physicalmedia";
        mediaFileInfo.existingMediaFile = search.game->getPhysicalMediaPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeMarquees") && result.marqueeUrl != "") {
        mediaFileInfo.fileURL = result.marqueeUrl;
        mediaFileInfo.fileFormat = result.marqueeFormat;
        mediaFileInfo.subDirectory = "marquees";
        mediaFileInfo.existingMediaFile = search.game->getMarqueePath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeScreenshots") && result.screenshotUrl != "") {
        mediaFileInfo.fileURL = result.screenshotUrl;
        mediaFileInfo.fileFormat = result.screenshotFormat;
        mediaFileInfo.subDirectory = "screenshots";
        mediaFileInfo.existingMediaFile = search.game->getScreenshotPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeTitleScreens") && result.titlescreenUrl != "") {
        mediaFileInfo.fileURL = result.titlescreenUrl;
        mediaFileInfo.fileFormat = result.titlescreenFormat;
        mediaFileInfo.subDirectory = "titlescreens";
        mediaFileInfo.existingMediaFile = search.game->getTitleScreenPath();
        mediaFileInfo.resizeFile = true;
        scrapeFiles.push_back(mediaFileInfo);
    }
    if (Settings::getInstance()->getBool("ScrapeVideos") && result.videoUrl != "") {
        mediaFileInfo.fileURL = result.videoUrl;
        mediaFileInfo.fileFormat = result.videoFormat;
        mediaFileInfo.subDirectory = "videos";
        mediaFileInfo.existingMediaFile = search.game->getVideoPath();
        mediaFileInfo.resizeFile = false;
        scrapeFiles.push_back(mediaFileInfo);
#if defined(_WIN64)
        // Required due to the idiotic file locking that exists on this operating system.
        ViewController::getInstance()->stopViewVideos();
#endif
    }

    for (auto it = scrapeFiles.cbegin(); it != scrapeFiles.cend(); ++it) {

        std::string ext;

        // If we have a file extension returned by the scraper, then use it.
        // Otherwise, try to guess it by the name of the URL, which points to a media file.
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
        if (mResult.thumbnailImageUrl == it->fileURL && mResult.thumbnailImageData.size() > 0) {

            // This is just a temporary workaround to avoid saving media files to disk that
            // are actually just containing error messages from the scraper service. The
            // proper solution is to implement file checksum checks to determine if the
            // server response contains valid media. As for the current approach, if the
            // file is less than 350 bytes, we check if FreeImage can actually detect a
            // valid format, and if not, we present an error message. Black/empty images
            // are sometimes returned from the scraper service and these can actually be
            // less than 350 bytes in size.
            if (Settings::getInstance()->getBool("ScraperHaltOnInvalidMedia") &&
                mResult.thumbnailImageData.size() < 350) {

                FIMEMORY* memoryStream =
                    FreeImage_OpenMemory(reinterpret_cast<BYTE*>(&mResult.thumbnailImageData.at(0)),
                                         static_cast<DWORD>(mResult.thumbnailImageData.size()));

                FREE_IMAGE_FORMAT imageFormat = FreeImage_GetFileTypeFromMemory(memoryStream, 0);
                FreeImage_CloseMemory(memoryStream);

                if (imageFormat == FIF_UNKNOWN) {
                    setError("The file \"" + Utils::FileSystem::getFileName(filePath) +
                             "\" returned by the scraper seems to be invalid as it's less than " +
                             "350 bytes in size");
                    return;
                }
            }

            // Remove any existing media file before attempting to write a new one.
            // This avoids the problem where there's already a file for this media type
            // with a different format/extension (e.g. game.jpg and we're going to write
            // game.png) which would lead to two media files for this game.
            if (it->existingMediaFile != "")
                Utils::FileSystem::removeFile(it->existingMediaFile);

            // If the media directory does not exist, something is wrong, possibly permission
            // problems or the MediaDirectory setting points to a file instead of a directory.
            if (!Utils::FileSystem::isDirectory(Utils::FileSystem::getParent(filePath))) {
                setError("Media directory does not exist and can't be created. "
                         "Permission problems?");
                LOG(LogError) << "Couldn't create media directory: \""
                              << Utils::FileSystem::getParent(filePath) << "\"";
                return;
            }

#if defined(_WIN64)
            std::ofstream stream(Utils::String::stringToWideString(filePath).c_str(),
                                 std::ios_base::out | std::ios_base::binary);
#else
            std::ofstream stream(filePath, std::ios_base::out | std::ios_base::binary);
#endif
            if (!stream || stream.bad()) {
                setError("Failed to open path for writing media file.\nPermission error?");
                return;
            }

            const std::string& content = mResult.thumbnailImageData;
            stream.write(content.data(), content.length());
            stream.close();
            if (stream.bad()) {
                setError("Failed to save media file.\nDisk full?");
                return;
            }

            // Resize it.
            if (it->resizeFile) {
                if (!resizeImage(filePath, it->subDirectory)) {
                    setError("Error saving resized image.\nOut of memory? Disk full?");
                    return;
                }
            }

            mResult.savedNewMedia = true;
        }
        // If it's not cached, then initiate the download.
        else {
            mFuncs.push_back(ResolvePair(downloadMediaAsync(it->fileURL, filePath,
                                                            it->existingMediaFile, it->subDirectory,
                                                            it->resizeFile, mResult.savedNewMedia),
                                         [filePath] {}));
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
        ++it;
    }

    if (mFuncs.empty())
        setStatus(ASYNC_DONE);
}

std::unique_ptr<MediaDownloadHandle> downloadMediaAsync(const std::string& url,
                                                        const std::string& saveAs,
                                                        const std::string& existingMediaPath,
                                                        const std::string& mediaType,
                                                        const bool resizeFile,
                                                        bool& savedNewMedia)
{
    return std::unique_ptr<MediaDownloadHandle>(new MediaDownloadHandle(
        url, saveAs, existingMediaPath, mediaType, resizeFile, savedNewMedia));
}

MediaDownloadHandle::MediaDownloadHandle(const std::string& url,
                                         const std::string& path,
                                         const std::string& existingMediaPath,
                                         const std::string& mediaType,
                                         const bool resizeFile,
                                         bool& savedNewMedia)
    : mReq(new HttpReq(url))
    , mSavePath(path)
    , mExistingMediaFile(existingMediaPath)
    , mMediaType(mediaType)
    , mResizeFile(resizeFile)
{
    mSavedNewMediaPtr = &savedNewMedia;
}

void MediaDownloadHandle::update()
{
    if (mReq->status() == HttpReq::REQ_IN_PROGRESS)
        return;

    if (mReq->status() != HttpReq::REQ_SUCCESS) {
        std::stringstream ss;
        ss << "Network error: " << mReq->getErrorMsg();
        setError(ss.str());
        return;
    }

    // This seems to take care of a strange race condition where the media saving and
    // resizing would sometimes take place twice.
    if (mStatus == ASYNC_DONE)
        return;

    // Download is done, save it to disk.

    // There are multiple issues with box back covers at ScreenScraper. Some only contain a single
    // color like pure black or more commonly pure green, and some are mostly transparent with just
    // a few black lines at the bottom. The following code attempts to detect such broken images
    // and skip them so they're not saved to disk.
    if (Settings::getInstance()->getString("Scraper") == "screenscraper" &&
        mMediaType == "backcovers") {
        bool emptyImage = false;
        FREE_IMAGE_FORMAT imageFormat = FIF_UNKNOWN;
        std::string imageData = mReq->getContent();
        FIMEMORY* memoryStream = FreeImage_OpenMemory(reinterpret_cast<BYTE*>(&imageData.at(0)),
                                                      static_cast<DWORD>(imageData.size()));
        imageFormat = FreeImage_GetFileTypeFromMemory(memoryStream, 0);

        if (imageFormat != FIF_UNKNOWN) {
            emptyImage = true;

            FIBITMAP* tempImage = FreeImage_LoadFromMemory(imageFormat, memoryStream);
            RGBQUAD firstPixel;
            RGBQUAD currPixel;

            unsigned int width = FreeImage_GetWidth(tempImage);
            unsigned int height = FreeImage_GetHeight(tempImage);

            // Skip really small images as they're obviously not valid.
            if (width < 50) {
                emptyImage = true;
            }
            else if (height < 50) {
                emptyImage = true;
            }
            else {
                // Remove the alpha channel which will convert fully transparent pixels to black.
                if (FreeImage_GetBPP(tempImage) != 24) {
                    FIBITMAP* convertImage = FreeImage_ConvertTo24Bits(tempImage);
                    FreeImage_Unload(tempImage);
                    tempImage = convertImage;
                }

                // Skip the first line as this can apparently lead to false positives.
                FreeImage_GetPixelColor(tempImage, 0, 1, &firstPixel);

                for (unsigned int x = 0; x < width; ++x) {
                    if (!emptyImage)
                        break;
                    // Skip the last line as well.
                    for (unsigned int y = 1; y < height - 1; ++y) {
                        FreeImage_GetPixelColor(tempImage, x, y, &currPixel);
                        if (currPixel.rgbBlue != firstPixel.rgbBlue ||
                            currPixel.rgbGreen != firstPixel.rgbGreen ||
                            currPixel.rgbRed != firstPixel.rgbRed) {
                            emptyImage = false;
                            break;
                        }
                    }
                }
            }
            FreeImage_Unload(tempImage);
        }
        FreeImage_CloseMemory(memoryStream);

        if (emptyImage) {
            LOG(LogWarning) << "ScreenScraper: Image does not seem to contain any data, not saving "
                               "it to disk: \""
#if defined(_WIN64)
                            << Utils::String::replace(mSavePath, "/", "\\") << "\"";
#else
                            << mSavePath << "\"";
#endif
            setStatus(ASYNC_DONE);
            return;
        }
    }

    // This is just a temporary workaround to avoid saving media files to disk that are
    // actually just containing error messages from the scraper service. The proper solution
    // is to implement file checksum checks to determine if the server response contains valid
    // media. As for the current approach, if the file is less than 350 bytes, we check if
    // FreeImage can actually detect a valid format, and if not, we present an error message.
    // Black/empty images are sometimes returned from the scraper service and these can actually
    // be less than 350 bytes in size.
    if (Settings::getInstance()->getBool("ScraperHaltOnInvalidMedia") &&
        mReq->getContent().size() < 350) {

        FREE_IMAGE_FORMAT imageFormat = FIF_UNKNOWN;

        if (mMediaType != "videos") {
            std::string imageData = mReq->getContent();
            FIMEMORY* memoryStream = FreeImage_OpenMemory(reinterpret_cast<BYTE*>(&imageData.at(0)),
                                                          static_cast<DWORD>(imageData.size()));
            imageFormat = FreeImage_GetFileTypeFromMemory(memoryStream, 0);
            FreeImage_CloseMemory(memoryStream);
        }

        if (imageFormat == FIF_UNKNOWN) {
            setError("The file \"" + Utils::FileSystem::getFileName(mSavePath) +
                     "\" returned by the scraper seems to be invalid as it's less than " +
                     "350 bytes in size");
            return;
        }
    }

    // Remove any existing media file before attempting to write a new one.
    // This avoids the problem where there's already a file for this media type
    // with a different format/extension (e.g. game.jpg and we're going to write
    // game.png) which would lead to two media files for this game.
    if (mExistingMediaFile != "")
        Utils::FileSystem::removeFile(mExistingMediaFile);

    // If the media directory does not exist, something is wrong, possibly permission
    // problems or the MediaDirectory setting points to a file instead of a directory.
    if (!Utils::FileSystem::isDirectory(Utils::FileSystem::getParent(mSavePath))) {
        setError("Media directory does not exist and can't be created. Permission problems?");
        LOG(LogError) << "Couldn't create media directory: \""
                      << Utils::FileSystem::getParent(mSavePath) << "\"";
        return;
    }

#if defined(_WIN64)
    std::ofstream stream(Utils::String::stringToWideString(mSavePath).c_str(),
                         std::ios_base::out | std::ios_base::binary);
#else
    std::ofstream stream(mSavePath, std::ios_base::out | std::ios_base::binary);
#endif
    if (!stream || stream.bad()) {
        setError("Failed to open path for writing media file.\nPermission error?");
        return;
    }

    const std::string& content = mReq->getContent();
    stream.write(content.data(), content.length());
    stream.close();
    if (stream.bad()) {
        setError("Failed to save media file.\nDisk full?");
        return;
    }

    // Resize it.
    if (mResizeFile) {
        if (!resizeImage(mSavePath, mMediaType)) {
            setError("Error saving resized image.\nOut of memory? Disk full?");
            return;
        }
    }

    // If this media file was successfully saved, update savedNewMedia in ScraperSearchResult.
    *mSavedNewMediaPtr = true;

    setStatus(ASYNC_DONE);
}

bool resizeImage(const std::string& path, const std::string& mediaType)
{
    float maxWidth {0.0f};
    float maxHeight {0.0f};

    if (mediaType == "marquees") {
        // We don't really need huge marquees.
        maxWidth = 1000.0f;
        maxHeight = 600.0f;
    }
    else {
        maxWidth = 2560.0f;
        maxHeight = 1440.0f;
    }

    FREE_IMAGE_FORMAT format {FIF_UNKNOWN};
    FIBITMAP* image {nullptr};

    // Detect the file format.

#if defined(_WIN64)
    format = FreeImage_GetFileTypeU(Utils::String::stringToWideString(path).c_str(), 0);
    if (format == FIF_UNKNOWN)
        format = FreeImage_GetFIFFromFilenameU(Utils::String::stringToWideString(path).c_str());
#else
    format = FreeImage_GetFileType(path.c_str(), 0);
    if (format == FIF_UNKNOWN)
        format = FreeImage_GetFIFFromFilename(path.c_str());
#endif
    if (format == FIF_UNKNOWN) {
        LOG(LogError) << "Could not detect filetype for image \"" << path << "\"!";
        return false;
    }

    // Make sure we can read this format, and if so, then load it.
    if (FreeImage_FIFSupportsReading(format)) {
#if defined(_WIN64)
        image = FreeImage_LoadU(format, Utils::String::stringToWideString(path).c_str());
#else
        image = FreeImage_Load(format, path.c_str());
#endif
    }
    else {
        LOG(LogError) << "File format not supported for image \"" << path << "\"";
        return false;
    }

    float width {static_cast<float>(FreeImage_GetWidth(image))};
    float height {static_cast<float>(FreeImage_GetHeight(image))};

    // If the image is smaller than (or the same size as) maxWidth and maxHeight, then don't
    // do any scaling. It doesn't make sense to upscale the image and waste disk space.
    if (maxWidth >= width && maxHeight >= height) {
#if defined(_WIN64)
        LOG(LogDebug) << "Scraper::resizeImage(): Saving image \""
                      << Utils::String::replace(path, "/", "\\") << "\" at its original resolution "
                      << width << "x" << height;
#else
        LOG(LogDebug) << "Scraper::resizeImage(): Saving image \"" << path
                      << "\" at its original resolution " << width << "x" << height;
#endif
        FreeImage_Unload(image);
        return true;
    }

    float scaleFactor {0.0f};

    // Calculate how much we should scale.
    if (width > maxWidth) {
        scaleFactor = maxWidth / width;
        if (height * scaleFactor > maxHeight)
            scaleFactor = maxHeight / height;
    }
    else {
        scaleFactor = maxHeight / height;
    }

    maxWidth = floorf(width * scaleFactor);
    maxHeight = floorf(height * scaleFactor);

    // We use Lanczos3 which is the highest quality resampling method available in FreeImage.
    FIBITMAP* imageRescaled {FreeImage_Rescale(image, static_cast<int>(maxWidth),
                                               static_cast<int>(maxHeight), FILTER_LANCZOS3)};
    FreeImage_Unload(image);

    if (imageRescaled == nullptr) {
        LOG(LogError) << "Couldn't resize image, not enough memory or invalid bit depth?";
        return false;
    }

#if defined(_WIN64)
    bool saved {FreeImage_SaveU(format, imageRescaled,
                                Utils::String::stringToWideString(path).c_str()) != 0};
#else
    bool saved {FreeImage_Save(format, imageRescaled, path.c_str()) != 0};
#endif
    FreeImage_Unload(imageRescaled);

    if (!saved) {
        LOG(LogError) << "Failed to save resized image";
    }
    else {
#if defined(_WIN64)
        LOG(LogDebug) << "Scraper::resizeImage(): Downscaled image \""
                      << Utils::String::replace(path, "/", "\\") << "\" from " << width << "x"
                      << height << " to " << maxWidth << "x" << maxHeight;
#else
        LOG(LogDebug) << "Scraper::resizeImage(): Downscaled image \"" << path << "\" from "
                      << width << "x" << height << " to " << maxWidth << "x" << maxHeight;
#endif
    }

    return saved;
}

std::string getSaveAsPath(const ScraperSearchParams& params,
                          const std::string& filetypeSubdirectory,
                          const std::string& extension)
{
    const std::string systemsubdirectory = params.system->getName();
    const std::string name = Utils::FileSystem::getStem(params.game->getPath());
    std::string subFolders;

    // Extract possible subfolders from the path.
    if (params.system->getSystemEnvData()->mStartPath != "")
        subFolders = Utils::String::replace(Utils::FileSystem::getParent(params.game->getPath()),
                                            params.system->getSystemEnvData()->mStartPath, "");

    std::string path = FileData::getMediaDirectory();

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += systemsubdirectory + "/" + filetypeSubdirectory + subFolders + "/";

    if (!Utils::FileSystem::exists(path))
        Utils::FileSystem::createDirectory(path);

    path += name + extension;
    return path;
}

//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  HttpReq.cpp
//
//  HTTP requests using libcurl.
//  Used by the scraper and application updater.
//

#include "HttpReq.h"

#include "ApplicationVersion.h"
#include "Log.h"
#include "Settings.h"
#include "resources/ResourceManager.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

#include <algorithm>
#include <assert.h>

std::string HttpReq::urlEncode(const std::string& s)
{
    const std::string unreserved {
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.~"};

    std::string escaped {""};
    for (size_t i {0}; i < s.length(); ++i) {
        if (unreserved.find_first_of(s[i]) != std::string::npos) {
            escaped.push_back(s[i]);
        }
        else {
            escaped.append("%");
            char buf[3];
            snprintf(buf, 3, "%.2X", static_cast<unsigned char>(s[i]));
            escaped.append(buf);
        }
    }
    return escaped;
}

HttpReq::HttpReq(const std::string& url, bool scraperRequest)
    : mStatus {REQ_IN_PROGRESS}
    , mHandle {nullptr}
    , mTotalBytes {0}
    , mDownloadedBytes {0}
    , mScraperRequest {scraperRequest}
{
    // The multi-handle is cleaned up via an explicit call to cleanupCurlMulti() from any object
    // that uses HttpReq. For example from GuiScraperSearch after scraping has been completed.
    if (!sMultiHandle)
        sMultiHandle = curl_multi_init();

    mHandle = curl_easy_init();

    if (mHandle == nullptr) {
        mStatus = REQ_IO_ERROR;
        onError("curl_easy_init failed");
        return;
    }

    if (!mPollThread) {
        sStopPoll = false;
        mPollThread = std::make_unique<std::thread>(&HttpReq::pollCurl, this);
    }

#if defined(USE_BUNDLED_CERTIFICATES)
    // Use the bundled curl TLS/SSL certificates (which come from the Mozilla project).
    // This is used on Windows and also on Android as there is no way for curl to access
    // the system certificates on this OS.
    curl_easy_setopt(mHandle, CURLOPT_CAINFO,
                     ResourceManager::getInstance()
                         .getResourcePath(":/certificates/curl-ca-bundle.crt")
                         .c_str());
#endif

    // Set the URL.
    CURLcode err {curl_easy_setopt(mHandle, CURLOPT_URL, url.c_str())};
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    if (!mScraperRequest) {
        // Set User-Agent.
        std::string userAgent {"ES-DE Frontend/"};
        userAgent.append(PROGRAM_VERSION_STRING).append(" (");
#if defined(__ANDROID__)
        userAgent.append("Android");
#elif defined(_WIN64)
        userAgent.append("Windows");
#elif defined(__APPLE__)
        userAgent.append("macOS");
#elif defined(__linux__)
        userAgent.append("Linux");
#elif defined(__unix__)
        userAgent.append("Unix");
#else
        userAgent.append("Unknown");
#endif
        userAgent.append(")");
        CURLcode err {curl_easy_setopt(mHandle, CURLOPT_USERAGENT, userAgent.c_str())};
        if (err != CURLE_OK) {
            mStatus = REQ_IO_ERROR;
            onError(curl_easy_strerror(err));
            return;
        }
    }

    long connectionTimeout;

    if (mScraperRequest) {
        connectionTimeout =
            static_cast<long>(Settings::getInstance()->getInt("ScraperConnectionTimeout"));

        if (connectionTimeout < 0 || connectionTimeout > 300)
            connectionTimeout = static_cast<long>(
                Settings::getInstance()->getDefaultInt("ScraperConnectionTimeout"));
    }
    else {
        connectionTimeout = 30;
    }

    // Set connection timeout (default is 30 seconds).
    err = curl_easy_setopt(mHandle, CURLOPT_CONNECTTIMEOUT, connectionTimeout);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    long transferTimeout;

    if (mScraperRequest) {
        transferTimeout =
            static_cast<long>(Settings::getInstance()->getInt("ScraperTransferTimeout"));

        if (transferTimeout < 0 || transferTimeout > 300)
            transferTimeout =
                static_cast<long>(Settings::getInstance()->getDefaultInt("ScraperTransferTimeout"));
    }
    else {
        transferTimeout = 0;
    }

    // Set transfer timeout (default is 120 seconds for the scraper and infinite otherwise).
    err = curl_easy_setopt(mHandle, CURLOPT_TIMEOUT, transferTimeout);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Set curl to handle redirects.
    err = curl_easy_setopt(mHandle, CURLOPT_FOLLOWLOCATION, 1L);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Set curl max redirects.
    err = curl_easy_setopt(mHandle, CURLOPT_MAXREDIRS, 2L);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Set curl restrict redirect protocols.
#if defined(__APPLE__) || LIBCURL_VERSION_MAJOR < 7 ||                                             \
    (LIBCURL_VERSION_MAJOR == 7 && LIBCURL_VERSION_MINOR < 85)
    err = curl_easy_setopt(mHandle, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
#else
    err = curl_easy_setopt(mHandle, CURLOPT_REDIR_PROTOCOLS_STR, "http,https");
#endif

    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Tell curl how to write the data.
    err = curl_easy_setopt(mHandle, CURLOPT_WRITEFUNCTION, &HttpReq::writeContent);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Pass curl a pointer to this HttpReq so we know where to write the data to in our
    // write function.
    err = curl_easy_setopt(mHandle, CURLOPT_WRITEDATA, this);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Enable the curl progress meter.
    err = curl_easy_setopt(mHandle, CURLOPT_NOPROGRESS, mScraperRequest ? 1 : 0);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Pass curl a pointer to HttpReq to provide access to the counter variables.
    err = curl_easy_setopt(mHandle, CURLOPT_XFERINFODATA, this);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Progress meter callback.
    if (!mScraperRequest) {
        err = curl_easy_setopt(mHandle, CURLOPT_XFERINFOFUNCTION, HttpReq::transferProgress);
        if (err != CURLE_OK) {
            mStatus = REQ_IO_ERROR;
            onError(curl_easy_strerror(err));
            return;
        }
    }

    // Fail on HTTP status codes >= 400.
    err = curl_easy_setopt(mHandle, CURLOPT_FAILONERROR, 1L);
    if (err != CURLE_OK) {
        mStatus = REQ_IO_ERROR;
        onError(curl_easy_strerror(err));
        return;
    }

    // Add the handle to the multi. This is done in pollCurl(), running in a separate thread.
    std::unique_lock<std::mutex> handleLock {sHandleMutex};
    sAddHandleQueue.push(mHandle);
    handleLock.unlock();

    curl_multi_wakeup(sMultiHandle);

    std::unique_lock<std::mutex> requestLock {sRequestMutex};
    sRequests[mHandle] = this;
    requestLock.unlock();
}

HttpReq::~HttpReq()
{
    if (mHandle) {
        std::unique_lock<std::mutex> requestLock {sRequestMutex};
        sRequests.erase(mHandle);
        requestLock.unlock();

        std::unique_lock<std::mutex> handleLock {sHandleMutex};
        sRemoveHandleQueue.push(mHandle);
        handleLock.unlock();

        curl_multi_wakeup(sMultiHandle);
    }
}

std::string HttpReq::getContent() const
{
    assert(mStatus == REQ_SUCCESS);
    return mContent.str();
}

int HttpReq::transferProgress(
    void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    if (dltotal == 0 && dlnow == 0)
        return CURLE_OK;

    // We need all the check logic below to make sure we're not attempting to write into
    // a request that has just been removed by the main thread.
    bool validEntry {false};

    std::unique_lock<std::mutex> requestLock {sRequestMutex};
    if (std::find_if(sRequests.cbegin(), sRequests.cend(), [&clientp](auto&& entry) {
            return entry.second == clientp;
        }) != sRequests.cend())
        validEntry = true;

    if (validEntry) {
        // Note that it's not guaranteed that the server will actually provide the total size.
        if (dltotal > 0)
            static_cast<HttpReq*>(clientp)->mTotalBytes = static_cast<long>(dltotal);
        if (dlnow > 0)
            static_cast<HttpReq*>(clientp)->mDownloadedBytes = static_cast<long>(dlnow);
    }

    requestLock.unlock();

    return CURLE_OK;
}

size_t HttpReq::writeContent(void* buff, size_t size, size_t nmemb, void* req_ptr)
{
    // We need all the check logic below to make sure we're not attempting to write into
    // a request that has just been removed by the main thread.
    bool validEntry {false};

    std::unique_lock<std::mutex> requestLock {sRequestMutex};
    if (std::find_if(sRequests.cbegin(), sRequests.cend(), [&req_ptr](auto&& entry) {
            return entry.second == req_ptr;
        }) != sRequests.cend())
        validEntry = true;

    if (validEntry) {
        // size = size of an element, nmemb = number of elements.
        std::stringstream& ss {static_cast<HttpReq*>(req_ptr)->mContent};
        ss.write(static_cast<char*>(buff), size * nmemb);
    }

    requestLock.unlock();

    // Return value is number of elements successfully read.
    return nmemb;
}

void HttpReq::pollCurl()
{
    int numfds {0};

    do {
        if (!sStopPoll)
            curl_multi_poll(sMultiHandle, nullptr, 0, 2000, &numfds);

        // Check if any easy handles should be added or removed.
        std::unique_lock<std::mutex> handleLock {sHandleMutex};

        if (sAddHandleQueue.size() > 0) {
            // Add the handle to our multi.
            CURLMcode merr {curl_multi_add_handle(sMultiHandle, sAddHandleQueue.front())};

            std::unique_lock<std::mutex> requestLock {sRequestMutex};
            HttpReq* req {sRequests[sAddHandleQueue.front()]};
            if (merr != CURLM_OK) {
                if (req != nullptr) {
                    req->mStatus = REQ_IO_ERROR;
                    req->onError(curl_multi_strerror(merr));
                    LOG(LogError) << "onError(): " << curl_multi_strerror(merr);
                }
            }
            else {
                if (req != nullptr)
                    req->mStatus = REQ_IN_PROGRESS;
            }
            sAddHandleQueue.pop();
            requestLock.unlock();
        }

        if (sRemoveHandleQueue.size() > 0) {
            // Remove the handle from our multi.
            CURLMcode merr {curl_multi_remove_handle(sMultiHandle, sRemoveHandleQueue.front())};
            if (merr != CURLM_OK) {
                LOG(LogError) << "Error removing curl easy handle from curl multi: "
                              << curl_multi_strerror(merr);
            }
            curl_easy_cleanup(sRemoveHandleQueue.front());
            sRemoveHandleQueue.pop();
        }

        handleLock.unlock();

        if (sMultiHandle != nullptr && !sStopPoll) {
            int handleCount {0};
            std::unique_lock<std::mutex> handleLock {sHandleMutex};
            CURLMcode merr {curl_multi_perform(sMultiHandle, &handleCount)};
            handleLock.unlock();
            if (merr != CURLM_OK && merr != CURLM_CALL_MULTI_PERFORM) {
                LOG(LogError) << "Error reading data from multi: " << curl_multi_strerror(merr);
            }

            int msgsLeft;
            CURLMsg* msg;
            while (!sStopPoll && (msg = curl_multi_info_read(sMultiHandle, &msgsLeft)) != nullptr) {
                if (msg->msg == CURLMSG_DONE) {
                    std::unique_lock<std::mutex> requestLock {sRequestMutex};
                    HttpReq* req {sRequests[msg->easy_handle]};

                    if (req == nullptr) {
                        LOG(LogError) << "Cannot find easy handle!";
                        requestLock.unlock();
                        continue;
                    }

                    if (msg->data.result == CURLE_OK) {
                        req->mStatus = REQ_SUCCESS;
                    }
                    else if (msg->data.result == CURLE_PEER_FAILED_VERIFICATION) {
                        req->mStatus = REQ_FAILED_VERIFICATION;
                        req->onError(curl_easy_strerror(msg->data.result));
                    }
                    else if (msg->data.result == CURLE_HTTP_RETURNED_ERROR) {
                        long responseCode;
                        curl_easy_getinfo(msg->easy_handle, CURLINFO_RESPONSE_CODE, &responseCode);

                        if (responseCode == 430 &&
                            Settings::getInstance()->getString("Scraper") == "screenscraper") {
                            req->mContent << _("You have exceeded your daily scrape quota");
                            req->mStatus = REQ_SUCCESS;
                        }
                        else if (responseCode == 404 && req->mScraperRequest &&
                                 Settings::getInstance()->getBool("ScraperIgnoreHTTP404Errors")) {
                            req->mStatus = REQ_RESOURCE_NOT_FOUND;
                        }
                        else {
                            req->mStatus = REQ_BAD_STATUS_CODE;
                            req->onError(
                                Utils::String::format(_("Server returned HTTP error code %s"),
                                                      std::to_string(responseCode).c_str()));
                        }
                    }
                    else {
                        req->mStatus = REQ_IO_ERROR;
                        req->onError(curl_easy_strerror(msg->data.result));
                    }
                    requestLock.unlock();
                }
            }
        }
    } while (!sStopPoll || !sAddHandleQueue.empty() || !sRemoveHandleQueue.empty());
}

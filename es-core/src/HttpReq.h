//  SPDX-License-Identifier: MIT
//
//  ES-DE
//  HttpReq.h
//
//  HTTP requests using libcurl.
//  Used by the scraper and application updater.
//

#ifndef ES_CORE_HTTP_REQ_H
#define ES_CORE_HTTP_REQ_H

#include <curl/curl.h>

#include <atomic>
#include <map>
#include <sstream>

class HttpReq
{
public:
    HttpReq(const std::string& url, bool scraperRequest);
    ~HttpReq();

    enum Status {
        // clang-format off
        REQ_IN_PROGRESS,         // Request is in progress.
        REQ_SUCCESS,             // Request completed successfully.
        REQ_IO_ERROR,            // An error occured.
        REQ_FAILED_VERIFICATION, // Peer's certificate or fingerprint wasn't verified correctly.
        REQ_BAD_STATUS_CODE,     // HTTP error response >= 400.
        REQ_RESOURCE_NOT_FOUND,  // HTTP error code 404 specifically.
        REQ_INVALID_RESPONSE,    // The HTTP response was invalid.
        REQ_UNDEFINED_ERROR
        // clang-format on
    };

    // Process any received data and return the status afterwards.
    Status status();

    std::string getErrorMsg() { return mErrorMsg; }
    std::string getContent() const;
    long getTotalBytes() { return mTotalBytes; }
    long getDownloadedBytes() { return mDownloadedBytes; }

    static std::string urlEncode(const std::string& s);
    static void cleanupCurlMulti()
    {
        if (sMultiHandle != nullptr) {
            curl_multi_cleanup(sMultiHandle);
            sMultiHandle = nullptr;
        }
    }

private:
    // Callbacks.
    static int transferProgress(
        void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    static size_t writeContent(void* buff, size_t size, size_t nmemb, void* req_ptr);

    void onError(const std::string& msg) { mErrorMsg = msg; }

    static inline std::map<CURL*, HttpReq*> sRequests;
    static inline CURLM* sMultiHandle;

    Status mStatus;
    CURL* mHandle;

    std::stringstream mContent;
    std::string mErrorMsg;
    std::atomic<long> mTotalBytes;
    std::atomic<long> mDownloadedBytes;
    bool mScraperRequest;
};

#endif // ES_CORE_HTTP_REQ_H

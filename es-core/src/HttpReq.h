//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  HttpReq.h
//
//  HTTP requests using libcurl.
//  Used by the scraper and application updater.
//

#ifndef ES_CORE_HTTP_REQ_H
#define ES_CORE_HTTP_REQ_H

#include <curl/curl.h>

#include <atomic>
#include <cstdint>
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>

class HttpReq
{
public:
    // If bearerToken is non-empty, the request authenticates using an "Authorization: Bearer"
    // header.
    // If downloadFilePath is non-empty, the response body is streamed directly to that file
    // instead of being buffered in memory, which is required for large downloads.
    // If postJsonBody is non-empty, the request is a POST with that body (otherwise GET).
    // If failOnHttpError is false, CURLOPT_FAILONERROR is left unset so the response body is
    // still delivered on an HTTP >= 400 status (status() reports REQ_SUCCESS regardless;
    // getHttpStatusCode() must be checked instead).
    HttpReq(const std::string& url,
            bool scraperRequest,
            const std::string& downloadFilePath = "",
            const std::string& bearerToken = "",
            const std::string& postJsonBody = "",
            bool failOnHttpError = true);
    ~HttpReq();

    enum Status {
        // clang-format off
        REQ_IN_PROGRESS,         // Request is in progress.
        REQ_SUCCESS,             // Request completed successfully.
        REQ_IO_ERROR,            // An error occured.
        REQ_FAILED_VERIFICATION, // Peer's certificate or fingerprint wasn't verified correctly.
        REQ_BAD_STATUS_CODE,     // HTTP error response >= 400.
        REQ_RESOURCE_NOT_FOUND,  // HTTP error code 404 specifically.
        REQ_QUOTA_REACHED,       // API quota reached on scraper service.
        REQ_INVALID_RESPONSE,    // The HTTP response was invalid.
        REQ_UNDEFINED_ERROR
        // clang-format on
    };

    Status status() { return mStatus; }

    std::string getErrorMsg() { return mErrorMsg; }
    // Returns the full response body. Not valid if constructed with a downloadFilePath. Valid
    // for REQ_SUCCESS and REQ_BAD_STATUS_CODE (the latter can still carry a useful body).
    std::string getContent() const;
    int64_t getTotalBytes() { return mTotalBytes; }
    int64_t getDownloadedBytes() { return mDownloadedBytes; }
    // The HTTP response status code, e.g. 200 or 401. Only meaningful once the request has
    // left the REQ_IN_PROGRESS state.
    long getHttpStatusCode() { return mHttpStatusCode; }

    static std::string urlEncode(const std::string& s);

    // Called explicitly from any object that uses HttpReq.
    static void cleanupCurlMulti()
    {
        if (sMultiHandle != nullptr) {
            sStopPoll = true;
            curl_multi_wakeup(sMultiHandle);
            mPollThread->join();
            mPollThread.reset();
            curl_multi_cleanup(sMultiHandle);
            sMultiHandle = nullptr;
        }
    }

#if defined(__FreeBSD__)
    const static inline std::string sPlatformIdentifier {"B"};
#elif defined(__HAIKU__)
    const static inline std::string sPlatformIdentifier {"H"};
#elif defined(STEAM_DECK)
    const static inline std::string sPlatformIdentifier {"S"};
#elif defined(RETRODECK)
    const static inline std::string sPlatformIdentifier {"R"};
#elif defined(__ANDROID__)
    const static inline std::string sPlatformIdentifier {"G"};
#elif defined(APPIMAGE_BUILD)
    const static inline std::string sPlatformIdentifier {"A"};
#elif defined(__linux__)
    const static inline std::string sPlatformIdentifier {"L"};
#elif defined(__IOS__)
    const static inline std::string sPlatformIdentifier {"I"};
#elif defined(__APPLE__)
    const static inline std::string sPlatformIdentifier {"M"};
#elif defined(_WIN64)
    const static inline std::string sPlatformIdentifier {"W"};
#else
    const static inline std::string sPlatformIdentifier {"O"};
#endif

private:
    // Callbacks.
    static int transferProgress(
        void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
    static size_t writeContent(void* buff, size_t size, size_t nmemb, void* req_ptr);

    void onError(const std::string& msg) { mErrorMsg = msg; }

    // Poll constantly to maintain network throughput even during VSyncs and other waiting states.
    void pollCurl();

    static inline CURLM* sMultiHandle;
    static inline std::map<CURL*, HttpReq*> sRequests;
    static inline std::queue<CURL*> sAddHandleQueue;
    static inline std::queue<CURL*> sRemoveHandleQueue;

    std::atomic<Status> mStatus;
    CURL* mHandle;

    static inline std::unique_ptr<std::thread> mPollThread;
    static inline std::mutex sHandleMutex;
    static inline std::mutex sRequestMutex;

    std::stringstream mContent;
    std::ofstream mOutputFile;
    bool mStreamToFile;
    struct curl_slist* mHeaderList;
    std::string mErrorMsg;
    static inline std::atomic<bool> sStopPoll = false;
    std::atomic<int64_t> mTotalBytes;
    std::atomic<int64_t> mDownloadedBytes;
    std::atomic<long> mHttpStatusCode;
    bool mScraperRequest;
};

#endif // ES_CORE_HTTP_REQ_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  HttpReq.h
//
//  HTTP request functions.
//  Used by Scraper, GamesDBJSONScraper, GamesDBJSONScraperResources and
//  ScreenScraper to download game information and media files.
//  Also used by ApplicationUpdater to check for application updates.
//

#ifndef ES_CORE_HTTP_REQ_H
#define ES_CORE_HTTP_REQ_H

#include <curl/curl.h>
#include <map>
#include <sstream>

// Usage:
// HttpReq myRequest("www.duckduckgo.com", "/index.html");
//
// For blocking behavior:
// while (myRequest.status() == HttpReq::REQ_IN_PROGRESS);
//
// For non-blocking behavior:
// Check 'if (myRequest.status() != HttpReq::REQ_IN_PROGRESS)' in some sort of update method.
//
// Once one of those calls complete, the request is ready.
//
// Do something like this to capture errors:
// if (myRequest.status() != REQ_SUCCESS) {
//    // An error occured.
//    LOG(LogError) << "HTTP request error - " << myRequest.getErrorMessage();
//    return;
// }
//
// This is how to read the returned content:
// std::string content = myRequest.getContent();

class HttpReq
{
public:
    HttpReq(const std::string& url);
    ~HttpReq();

    enum Status {
        // clang-format off
        REQ_IN_PROGRESS,         // Request is in progress.
        REQ_SUCCESS,             // Request completed successfully, get it with getContent().
        REQ_IO_ERROR,            // Some error happened, get it with getErrorMsg().
        REQ_FAILED_VERIFICATION, // Peer's certificate or fingerprint wasn't verified correctly.
        REQ_BAD_STATUS_CODE,     // Some invalid HTTP response status code happened (non-200).
        REQ_INVALID_RESPONSE,    // The HTTP response was invalid.
        REQ_UNDEFINED_ERROR
        // clang-format on
    };

    Status status(); // Process any received data and return the status afterwards.
    std::string getErrorMsg() { return mErrorMsg; }
    std::string getContent() const; // mStatus must be REQ_SUCCESS.

    static std::string urlEncode(const std::string& s);
    static bool isUrl(const std::string& s);

    static void cleanupCurlMulti()
    {
        if (sMultiHandle != nullptr) {
            curl_multi_cleanup(sMultiHandle);
            sMultiHandle = nullptr;
        }
    }

private:
    static size_t writeContent(void* buff, size_t size, size_t nmemb, void* req_ptr);
    void onError(const std::string& msg) { mErrorMsg = msg; }

    static inline std::map<CURL*, HttpReq*> sRequests;
    static inline CURLM* sMultiHandle;

    Status mStatus;
    CURL* mHandle;

    std::stringstream mContent;
    std::string mErrorMsg;
};

#endif // ES_CORE_HTTP_REQ_H

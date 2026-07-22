//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMUtils.cpp
//

#include "RomM/RomMUtils.h"

#include "HttpReq.h"
#include "Settings.h"
#include "utils/StringUtil.h"

#include <chrono>
#include <ctime>
#include <thread>

namespace
{
    // Parses the fixed "YYYY-MM-DDTHH:MM:SS" prefix of an ISO-8601 timestamp as returned by
    // RomM's "expires_at" field into Unix seconds, ignoring any fractional-seconds/offset
    // suffix. Returns 0 on any parse failure.
    int64_t parseIso8601ToUnixSeconds(const std::string& value)
    {
        tm parsedTime {};
        if (sscanf(value.c_str(), "%d-%d-%dT%d:%d:%d", &parsedTime.tm_year, &parsedTime.tm_mon,
                   &parsedTime.tm_mday, &parsedTime.tm_hour, &parsedTime.tm_min,
                   &parsedTime.tm_sec) != 6)
            return 0;

        parsedTime.tm_year -= 1900;
        parsedTime.tm_mon -= 1;

#if defined(_WIN64)
        return static_cast<int64_t>(_mkgmtime(&parsedTime));
#else
        return static_cast<int64_t>(timegm(&parsedTime));
#endif
    }
} // namespace

namespace RomMUtils
{
    bool isLoggedIn()
    {
        if (Settings::getInstance()->getString("RomMToken").empty())
            return false;

        const std::string expiresAt {Settings::getInstance()->getString("RomMTokenExpiresAt")};
        if (expiresAt.empty())
            return true; // No expiry set - RomM's own "expires_at": null.

        const int64_t expiresAtUnix {parseIso8601ToUnixSeconds(expiresAt)};
        if (expiresAtUnix == 0)
            return true; // Unparseable - fail open rather than lock the user out over this.

        return std::time(nullptr) < expiresAtUnix;
    }

    std::string joinUrl(const std::string& serverURL, const std::string& path)
    {
        std::string trimmedServerURL {serverURL};
        while (!trimmedServerURL.empty() && trimmedServerURL.back() == '/')
            trimmedServerURL.pop_back();
        return trimmedServerURL + path;
    }

    bool checkHeartbeat(const std::string& serverURL)
    {
        HttpReq req {joinUrl(serverURL, "/api/heartbeat"), false};
        for (int i {0}; i < 30000 / 50; ++i) {
            const HttpReq::Status status {req.status()};
            if (status == HttpReq::REQ_SUCCESS)
                return true;
            if (status != HttpReq::REQ_IN_PROGRESS)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return false;
    }

    bool resolveServerUrl(const std::string& rawInput, std::string& outResolvedUrl)
    {
        std::string trimmed {Utils::String::trim(rawInput)};
        while (!trimmed.empty() && trimmed.back() == '/')
            trimmed.pop_back();

        if (trimmed.empty())
            return false;

        const std::string lowerTrimmed {Utils::String::toLower(trimmed)};
        if (lowerTrimmed.rfind("http://", 0) == 0 || lowerTrimmed.rfind("https://", 0) == 0) {
            if (!checkHeartbeat(trimmed))
                return false;
            outResolvedUrl = trimmed;
            return true;
        }

        const std::string httpsUrl {"https://" + trimmed};
        if (checkHeartbeat(httpsUrl)) {
            outResolvedUrl = httpsUrl;
            return true;
        }

        const std::string httpUrl {"http://" + trimmed};
        if (checkHeartbeat(httpUrl)) {
            outResolvedUrl = httpUrl;
            return true;
        }

        return false;
    }
} // namespace RomMUtils

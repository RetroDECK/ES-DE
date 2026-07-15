//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMApiClient.h
//
//  Thin client for the subset of the RomM (https://github.com/rommapp/romm) REST API that
//  ES-DE needs: listing platforms/roms and looking up a rom by hash. Used both by the RomM
//  scraper backend (matching an already-local game to its RomM record) and by the library
//  sync that lists not-yet-downloaded games inline in the gamelists.
//
//  Every method performs one or more blocking network calls and must therefore only be
//  invoked from a background thread, never from the render/main thread.
//

#ifndef ES_APP_ROMM_ROMM_API_CLIENT_H
#define ES_APP_ROMM_ROMM_API_CLIENT_H

#include <cstdint>
#include <string>
#include <vector>

class HttpReq;

class RomMApiClient
{
public:
    struct Platform {
        int id {0};
        std::string slug;
        std::string fsSlug;
        std::string name;
        int romCount {0};
    };

    struct Rom {
        int id {0};
        std::string name;
        std::string summary;
        std::string fsName;
        int64_t fsSizeBytes {0};
        std::string urlCover;
        std::vector<std::string> genres;
        std::vector<std::string> companies;
        // Unix timestamp (seconds since epoch), or 0 if unset. RomM's API itself reports this
        // in milliseconds - already divided down to seconds by the time it lands here.
        int64_t firstReleaseDate {0};
        float averageRating {0.0f};
        // Free-form string, e.g. "1-4" - not a pure integer in RomM's API.
        std::string playerCount;
    };

    RomMApiClient(const std::string& serverURL, const std::string& token);

    // Performs a lightweight request (fetching the platform list) purely to validate that
    // the server URL and credentials are correct. Returns false and sets lastError() on
    // any failure (unreachable server, wrong credentials, invalid response, etc.).
    bool testConnection();

    // Returns all platforms configured on the RomM server. Returns an empty vector and sets
    // lastError() on failure.
    std::vector<Platform> fetchPlatforms();

    // Pages through and returns every rom belonging to the given RomM platform id. Returns
    // an empty vector and sets lastError() on failure (an empty vector is also returned, with
    // no error, if the platform genuinely has no roms).
    std::vector<Rom> fetchRoms(int platformId);

    // Looks up a single rom by MD5 hash. Returns true and populates outRom on a match, false
    // otherwise (not found or a request error - check lastError() to disambiguate).
    bool fetchRomByHash(const std::string& md5Hash, Rom& outRom);

    const std::string& lastError() const { return mLastError; }

    // Best-effort match between an ES-DE system's platform name (PlatformIds::getPlatformName(),
    // e.g. "snes", as declared via the <platform> tag in es_systems.xml) and a RomM platform's
    // slug/fs_slug. RomM's own slug conventions are closely aligned with those used by
    // ES-DE/RetroPie/Batocera, so most systems match via a direct case-insensitive compare;
    // the small alias table below covers the handful of known mismatches. Extend the table as
    // more mismatches are discovered against real RomM instances.
    static bool platformNameMatches(const std::string& esdePlatformName,
                                    const std::string& rommSlug,
                                    const std::string& rommFsSlug);

private:
    std::string buildUrl(const std::string& path) const;

    // Blocks the calling thread until the request leaves REQ_IN_PROGRESS (success, error, or
    // a generous internal timeout). Returns true if the request succeeded.
    bool waitForRequest(HttpReq& req);

    std::string mServerURL;
    std::string mToken;
    std::string mLastError;
};

#endif // ES_APP_ROMM_ROMM_API_CLIENT_H

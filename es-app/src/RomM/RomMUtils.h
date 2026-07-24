//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMUtils.h
//
//  Stateless RomM helpers shared across the API client, the device-pairing flow and the GUIs -
//  none of these need (or, in the case of resolveServerUrl()/checkHeartbeat(), even have yet)
//  a fixed server to be instance methods of a client.
//

#ifndef ES_APP_ROMM_ROMM_UTILS_H
#define ES_APP_ROMM_ROMM_UTILS_H

#include <string>

namespace RomMUtils
{
    // Non-empty RomMToken with an unexpired (or absent) RomMTokenExpiresAt. Local check only.
    bool isLoggedIn();

    std::string joinUrl(const std::string& serverURL, const std::string& path);

    bool checkHeartbeat(const std::string& serverURL);

    // Tries https:// then http:// if rawInput has no scheme. Returns true and sets
    // outResolvedUrl on success.
    bool resolveServerUrl(const std::string& rawInput, std::string& outResolvedUrl);

    // Best-effort match between an ES-DE system's platform name (PlatformIds::getPlatformName(),
    // e.g. "snes", as declared via the <platform> tag in es_systems.xml) and a RomM platform's
    // slug/fs_slug. RomM's own slug conventions are closely aligned with those used by
    // ES-DE/RetroPie/Batocera, so most systems match via a direct case-insensitive compare;
    // the small alias table below covers the handful of known mismatches. Extend the table as
    // more mismatches are discovered against real RomM instances.
    bool platformNameMatches(const std::string& esdePlatformName,
                             const std::string& rommSlug,
                             const std::string& rommFsSlug);

    // Formats a UTC time_t as "YYYY-MM-DDTHH:MM:SSZ", the format RomM's updated_after query
    // parameter accepts (empirically verified against a live RomM instance). Public and
    // static so callers (e.g. RomMLibrarySync/RomMCache) can format a persisted sync cursor
    // without duplicating the gmtime_r/gmtime_s platform split.
    std::string formatTimestampUtc(time_t time);

    // Formats a RomM rom's metadatum.first_release_date (Unix seconds, UTC - the caller divides
    // RomM's own millisecond value down first) as ES-DE's MD_DATE storage format
    // ("YYYYMMDDT000000"). Returns "" if unset (<= 0) or implausible (a parsed year outside
    // [1950, this-year + 2], logged via gameName so a genuine parsing/unit bug can be told
    // apart from bad upstream data) - shared by the RomM scraper backend and the library sync,
    // which both need to turn the same field into the same ES-DE representation.
    std::string formatReleaseDate(int64_t firstReleaseDateUnixSeconds, const std::string& gameName);

    // Converts a RomM metadatum.average_rating (0-100, community/IGDB scale) to ES-DE's 0.0-1.0
    // MD_RATING string, rounded to the closest half-star like the ScreenScraper backend does for
    // its own rating scale. Returns "" if the result would be 0.
    std::string formatCommunityRating(float averageRating0to100);
} // namespace RomMUtils

#endif // ES_APP_ROMM_ROMM_UTILS_H

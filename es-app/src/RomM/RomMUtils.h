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

#include "RomM/RomMApiClient.h"

#include <string>

class FileData;

namespace RomMUtils
{
    // Non-empty RomMToken with an unexpired (or absent) RomMTokenExpiresAt. Local check only.
    bool isLoggedIn();

    std::string joinUrl(const std::string& serverURL, const std::string& path);

    // Strips trailing slashes so callers that need a bare normalized server URL (e.g. to pass
    // into another API expecting to append its own leading-slash path) match RomMApiClient's own
    // normalization without duplicating this loop at each call site.
    std::string stripTrailingSlashes(const std::string& url);

    // Reads the configured RomMServerURL setting and trims it via stripTrailingSlashes(). For
    // callers (e.g. RomMLibrarySync) that need the normalized server URL itself rather than
    // constructing a RomMApiClient (which already normalizes internally).
    std::string getServerUrl();

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

    // Applies everything ES-DE tracks about a rom from RomM onto file's metadata: server-sourced
    // descriptive fields (releasedate/rating/genre/developer/publisher/players, from RomM's rom
    // metadata), per-user state (hidden/completed, from RomM's rom_user resource), local favorite
    // intent (RomMLocalFavorites - favorite is never mirrored from RomM itself), and lastplayed
    // (merged forward only, since a game can be played through ES-DE or RomM's own web player and
    // neither source is authoritative alone). Each field is only set/merged when rom actually
    // carries a value for it, so passing a rom reconstructed from a cache that's missing some of
    // these (see RomMCache::toApiRom()) never regresses an existing value back to blank/default.
    // Called for still-remote entries during a sync (RomMLibrarySync, every run) and by
    // GuiGamelistOptions' "delete downloaded file" path (reverting an entry back to remote).
    void applyRomMData(FileData* file, const RomMApiClient::Rom& rom);

    // Only ever moves lastplayed forward - a game can be played through ES-DE (once downloaded) or
    // through RomM's own web player, so neither source can be trusted as authoritative on its own.
    // Exposed on its own (rather than only via applyRomMData()) for RomMLibrarySync's "already
    // downloaded" entries, which want just this merge without the rest of applyRomMData() touching
    // fields the downloaded copy now owns.
    void mergeLastPlayed(FileData* file, int64_t rommLastPlayedUnix);
} // namespace RomMUtils

#endif // ES_APP_ROMM_ROMM_UTILS_H

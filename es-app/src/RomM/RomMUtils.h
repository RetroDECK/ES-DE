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
} // namespace RomMUtils

#endif // ES_APP_ROMM_ROMM_UTILS_H

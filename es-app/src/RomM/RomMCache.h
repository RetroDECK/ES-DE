//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMCache.h
//
//  Local, on-disk cache of the (narrow) rom field subset RomMLibrarySync actually needs,
//  keyed by RomM platform id, plus a per-platform incremental-sync cursor. Lets a sync run
//  fetch only what's changed since last time (via RomMApiClient::fetchRoms's updated_after
//  parameter) instead of re-fetching every rom's full (and largely unused-by-ES-DE) payload
//  on every app launch.
//

#ifndef ES_APP_ROMM_ROMM_CACHE_H
#define ES_APP_ROMM_ROMM_CACHE_H

#include "RomM/RomMApiClient.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Not thread-safe by design: exactly one RomMLibrarySync background thread ever exists at a
// time (the startup background sync and the manual "FORCE FULL RESYNC" action, run via
// ViewController::runRomMSyncWithSplashScreen(), can't overlap - see
// ViewController::isRomMSyncing()), and it's the only caller of this class, so no locking is
// needed. Must never be touched from the main/render thread.
class RomMCache
{
public:
    // Only the fields RomMLibrarySync::applyResults()/buildDisplayNames() actually read -
    // deliberately excludes urlCover/genres/companies/firstReleaseDate/averageRating/
    // playerCount/files/updatedAt, which together account for the vast majority of a rom's
    // payload size and are never used by the sync path.
    struct CachedRom {
        int id {0};
        std::string name;
        std::string summary;
        std::string fsName;
        int64_t fsSizeBytes {0};
        std::string revision;
        std::vector<std::string> regions;
        std::vector<std::string> languages;
        bool hasMultipleFiles {false};
    };

    static RomMCache& getInstance();

    // Empty string if this platform has never been successfully synced.
    std::string getCursor(int rommPlatformId) const;
    // Empty vector if this platform has never been successfully synced.
    std::vector<CachedRom> getRoms(int rommPlatformId) const;
    // In-memory only - call flush() to persist. Overwrites any prior entry for this platform.
    void setPlatform(int rommPlatformId,
                     const std::string& cursor,
                     const std::vector<CachedRom>& roms);
    // Clears everything in memory (does not touch disk until flush() is called).
    void clearAll();
    // Writes the full cache to disk. Intended to be called once per sync run (after the loop
    // over every opted-in platform), not once per setPlatform() call.
    void flush();

    static CachedRom fromApiRom(const RomMApiClient::Rom& rom);
    static RomMApiClient::Rom toApiRom(const CachedRom& cached);

private:
    struct PlatformCache {
        std::string cursor;
        std::vector<CachedRom> roms;
    };

    RomMCache();
    void loadFile();

    std::unordered_map<int, PlatformCache> mPlatforms;
};

#endif // ES_APP_ROMM_ROMM_CACHE_H

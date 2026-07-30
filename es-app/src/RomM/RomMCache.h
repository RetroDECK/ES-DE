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

// Not thread-safe by design: every RomM sync runs via
// ViewController::runRomMSyncWithSplashScreen(), which blocks the main thread until done, so only
// one RomMLibrarySync background thread ever exists at a time, and it's the only caller of this
// class - no locking is needed. Writes (setPlatform()/flush()) must never be touched from the
// main/render thread. The exceptions are findCachedSize() and findCachedRom(): read-only lookups
// called from the main thread (ViewController::update(), to show a not-yet-downloaded game's size
// without a network round-trip; and GuiGamelistOptions' "delete downloaded file" path, to refill
// an entry's descriptive metadata when reverting it back to remote). Both are safe without
// locking because runRomMSyncWithSplashScreen() blocks the main thread for a sync's entire
// duration - the main thread (and thus these lookups) can never run while the sync thread is
// writing.
class RomMCache
{
public:
    // Deliberately excludes urlCover/files/updatedAt, which RomM's sync/revert paths never read.
    // Everything else is kept: a still-remote FileData is rebuilt from scratch every app run, and
    // an incremental sync only re-fetches changed roms, so an unchanged (or reverted-to-remote)
    // rom is reconstructed via toApiRom() from exactly what's cached here - without these fields
    // it would regress to blank/default values instead of keeping what RomM last reported.
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
        int64_t lastPlayed {0};
        bool userHidden {false};
        int userRating {0};
        std::string userStatus;
        std::vector<std::string> genres;
        std::vector<std::string> companies;
        int64_t firstReleaseDate {0};
        float averageRating {0.0f};
        std::string playerCount;
    };

    static RomMCache& getInstance();

    // Empty string if this platform has never been successfully synced.
    std::string getCursor(int rommPlatformId) const;
    // Empty vector if this platform has never been successfully synced.
    std::vector<CachedRom> getRoms(int rommPlatformId) const;
    // Scans every cached platform for a rom with this id. Returns false (leaving sizeBytesOut
    // untouched) if not found in any cached platform.
    bool findCachedSize(int rommId, int64_t& sizeBytesOut) const;
    // Scans every cached platform for a rom with this id. Returns false (leaving cachedOut
    // untouched) if not found in any cached platform. Read-only, so it's safe to call from the
    // main thread under the same invariant as findCachedSize() above.
    bool findCachedRom(int rommId, CachedRom& cachedOut) const;
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

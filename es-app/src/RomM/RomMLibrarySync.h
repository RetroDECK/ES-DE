//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMLibrarySync.h
//
//  Synchronizes ES-DE's gamelists with a RomM server. This is the reusable sync engine shared
//  by the splash-screen-driven syncs (ViewController::runRomMSyncWithSplashScreen(), used for
//  both the post-pairing sync and the manual "FORCE FULL RESYNC" action) and the silent
//  background sync triggered automatically on startup (ViewController::startRomMBackgroundSync()).
//

#ifndef ES_APP_ROMM_ROMM_LIBRARY_SYNC_H
#define ES_APP_ROMM_ROMM_LIBRARY_SYNC_H

#include "RomM/RomMApiClient.h"

#include <atomic>
#include <memory>
#include <thread>
#include <vector>

class SystemData;

// For each system opted into RomM sync (see RomMPlatformMapping), lists the roms present on
// the server and adds/updates/removes synthetic FileData entries (flagged "rommremote") for
// games not yet present locally. A local cache (RomMCache) of the rom fields this class needs
// lets normal syncs fetch only what's changed since last time (RomMApiClient::fetchRoms's
// updated_after parameter) instead of every rom's full payload - but that incremental path can
// only ever add/update, never detect a rom removed from RomM (a cheap, delta-only fetch has no
// way to know something's now missing). Removals are only reconciled by a forced full resync
// (see forceFullResync below), which re-fetches each platform's complete list with an empty
// cache to merge against, so anything no longer returned is naturally dropped.
//
// Network I/O runs on a background thread; the resulting FileData tree mutations are applied
// on the main thread via applyResults(), since SystemData/FileData/ViewController are not safe
// to touch from a background thread. Callers must poll isDone() from the main thread (e.g. from
// a GuiComponent::update() or ViewController::update()) and call applyResults() exactly once
// after it becomes true.
class RomMLibrarySync
{
public:
    // forceFullResync: when true, ignores any cached rom list/cursor for every opted-in
    // platform this run (treated the same as that platform never having been synced before)
    // and, once a platform's fetch succeeds, wholesale-replaces its cache entry with the fresh
    // result - the "FULL ROMM RESYNC" menu action's escape hatch, and the only way a rom
    // deleted from RomM gets reconciled locally (see the class comment above). Defaults to
    // false: the fast incremental+cache, additions/updates-only path the automatic background
    // sync always uses.
    explicit RomMLibrarySync(bool forceFullResync = false);
    ~RomMLibrarySync();

    // Runs synchronously on the main thread: for any RomM platform the user has enabled sync
    // for but which has no local ES-DE system yet, creates its ROM directory and rescans, so
    // the fetch below picks it up in the same continuous sync run - no restart, no separate
    // activation step. Then spawns the background fetch thread.
    void start();

    bool isDone() const { return mDoneSyncing; }

    // Runs on the main thread once isDone() is true: creates, updates and removes the synthetic
    // remote FileData entries based on the fetched results. Must only be called once per run.
    void applyResults();

    int getAddedCount() const { return mSystemsAdded; }
    int getRemovedCount() const { return mSystemsRemoved; }

    // Safe to poll from the main thread while the background thread runs. 0 until start() has
    // resolved the list of systems to sync.
    int getTotalSystems() const { return mTotalSystems; }
    int getCompletedSystems() const { return mCompletedSystems; }

    // Removes every synthetic "rommremote" placeholder FileData across all systems (used on
    // logout). Runs synchronously on the main thread.
    static void removeAllRemoteEntries();

private:
    struct SystemSyncResult {
        SystemData* system;
        std::vector<RomMApiClient::Rom> roms;
    };

    void activatePendingSystems();
    // Runs on the background thread: only network I/O, no FileData/ViewController access.
    void fetchInBackground();

    std::unique_ptr<std::thread> mSyncThread;
    std::atomic<bool> mDoneSyncing;
    std::vector<SystemSyncResult> mResults;
    int mSystemsAdded;
    int mSystemsRemoved;
    bool mForceFullResync;
    std::atomic<int> mTotalSystems;
    std::atomic<int> mCompletedSystems;
};

#endif // ES_APP_ROMM_ROMM_LIBRARY_SYNC_H

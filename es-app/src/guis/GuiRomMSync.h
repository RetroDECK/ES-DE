//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMSync.h
//
//  Synchronizes ES-DE's gamelists with a RomM server.
//

#ifndef ES_APP_GUIS_GUI_ROMM_SYNC_H
#define ES_APP_GUIS_GUI_ROMM_SYNC_H

#include "GuiComponent.h"
#include "RomM/RomMApiClient.h"
#include "components/BusyComponent.h"

#include <atomic>
#include <thread>

class SystemData;

// For each system opted into RomM sync (see RomMPlatformMapping), lists the roms present on
// the server and adds/updates/removes synthetic FileData entries (flagged "rommremote") for
// games not yet present locally.
//
// Network I/O runs on a background thread; the resulting FileData tree mutations are applied
// on the main thread from update(), since SystemData/FileData/ViewController are not safe to
// touch from a background thread. Input is blocked while syncing, matching the existing
// GuiGameImporter/GuiApplicationUpdater pattern for background-thread-driven dialogs.
class GuiRomMSync : public GuiComponent
{
public:
    GuiRomMSync();
    ~GuiRomMSync();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    struct SystemSyncResult {
        SystemData* system;
        std::vector<RomMApiClient::Rom> roms;
    };

    // Runs synchronously on the main thread from the constructor, before the background fetch
    // thread is spawned: for any RomM platform the user has enabled sync for but which has no
    // local ES-DE system yet, creates its ROM directory and rescans, so the fetch below picks
    // it up in the same continuous sync run - no restart, no separate activation step.
    void activatePendingSystems();
    // Runs on the background thread: only network I/O, no FileData/ViewController access.
    void fetchInBackground();
    // Runs on the main thread once fetchInBackground() has completed: creates, updates and
    // removes the synthetic remote FileData entries based on the fetched results.
    void applyResults();

    Renderer* mRenderer;
    BusyComponent mBusyAnim;
    std::unique_ptr<std::thread> mSyncThread;
    std::atomic<bool> mSyncing;
    std::atomic<bool> mDoneSyncing;
    std::vector<SystemSyncResult> mResults;
    int mSystemsAdded;
    int mSystemsRemoved;
};

#endif // ES_APP_GUIS_GUI_ROMM_SYNC_H

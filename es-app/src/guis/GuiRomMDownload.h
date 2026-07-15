//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMDownload.h
//
//  Downloads a RomM library entry's file content to disk.
//

#ifndef ES_APP_GUIS_GUI_ROMM_DOWNLOAD_H
#define ES_APP_GUIS_GUI_ROMM_DOWNLOAD_H

#include "GuiComponent.h"
#include "RomM/RomMApiClient.h"
#include "components/BusyComponent.h"

#include <atomic>
#include <memory>
#include <thread>

class FileData;

// Streams a RomM library entry's file content to disk (via HttpReq's downloadFilePath
// support), replacing the placeholder "rommremote" FileData with a real one once the file
// lands at its final path. Modeled on GuiRomMSync's background-thread + BusyComponent pattern,
// plus a live download percentage and a B-button cancel (matching
// GuiApplicationUpdater::downloadPackage()'s cancel behavior).
//
// Network I/O (including the streaming file write, which HttpReq itself performs) runs on a
// background thread; the resulting FileData/metadata mutation and ViewController notification
// run on the main thread from update(), since those aren't safe to touch from a background
// thread.
class GuiRomMDownload : public GuiComponent
{
public:
    explicit GuiRomMDownload(FileData* game);
    ~GuiRomMDownload();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    // Runs on the background thread: fetches the rom's current detail by id (rather than
    // trusting anything cached from an earlier sync) to decide whether it's multi-disc, then
    // dispatches to one of the two methods below.
    void downloadInBackground();
    // Single-file rom: one HttpReq straight to mTmpPath, as a plain file.
    void downloadSingleFileInBackground();
    // Multi-disc rom: mTmpPath is created as a directory, each disc/part file downloaded into it
    // individually via RomM's per-file endpoint (its rom-level endpoint bundles these as a zip,
    // which ES-DE has no way to unpack). Uses RomM's own .m3u among the files if present,
    // otherwise synthesizes one - either way it's saved named exactly like the wrapper
    // directory, since that's what FileData::launchGame() looks for (see
    // SystemData::populateFolder() for the ES-DE convention this reuses).
    void downloadMultiDiscInBackground(const RomMApiClient::Rom& rom);
    // Runs on the main thread once downloadInBackground() has completed: renames the
    // downloaded file/directory into place and updates the FileData, or cleans up on
    // failure/cancel.
    void finishOnMainThread();

    Renderer* mRenderer;
    BusyComponent mBusyAnim;
    FileData* mGame;
    bool mMultiDisc;
    std::string mTmpPath;
    std::unique_ptr<std::thread> mDownloadThread;
    std::atomic<bool> mDownloading;
    std::atomic<bool> mDone;
    std::atomic<bool> mAbort;
    std::atomic<int> mPercentage;
    bool mSuccess;
    std::string mErrorMessage;
};

#endif // ES_APP_GUIS_GUI_ROMM_DOWNLOAD_H

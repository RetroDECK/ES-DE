//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMDownload.cpp
//

#include "guis/GuiRomMDownload.h"

#include "FileData.h"
#include "HttpReq.h"
#include "Log.h"
#include "RomM/RomMApiClient.h"
#include "RomM/RomMLocalFavorites.h"
#include "Settings.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#include <SDL2/SDL_timer.h>

#include <cstdlib>
#include <fstream>

namespace
{
    // FileData::getMediafilePath() names media after getDisplayName() (getStem(mPath)), which
    // only strips the extension when the path isn't currently a directory - so a multi-disc
    // entry's media gets cached under the extension-stripped name while still remote (path
    // doesn't exist yet) but looked up under the extension-included name once downloaded (path
    // is now a real directory). Migrate anything caught by that flip, or it disappears.
    void migrateMultiDiscMedia(const std::string& oldName,
                               const std::string& newName,
                               const std::string& systemName)
    {
        const std::string mediaRoot {FileData::getMediaDirectory() + systemName};
        if (!Utils::FileSystem::isDirectory(mediaRoot))
            return;

        for (const std::string& path : Utils::FileSystem::getDirContent(mediaRoot, true)) {
            if (Utils::FileSystem::isDirectory(path) || Utils::FileSystem::getStem(path) != oldName)
                continue;
            const std::string newPath {Utils::FileSystem::getParent(path) + "/" + newName +
                                       Utils::FileSystem::getExtension(path)};
            if (Utils::FileSystem::renameFile(path, newPath, true)) {
                LOG(LogWarning) << "RomM download: Failed to migrate media file \"" << path
                                << "\" to \"" << newPath << "\"";
            }
        }
    }
} // namespace

GuiRomMDownload::GuiRomMDownload(FileData* game)
    : mRenderer {Renderer::getInstance()}
    , mGame {game}
    , mMultiDisc {false}
    , mTmpPath {game->getPath() + ".tmp"}
    , mDownloading {true}
    , mDone {false}
    , mAbort {false}
    , mPercentage {0}
    , mSuccess {false}
{
    setSize(mRenderer->getScreenWidth() * 0.4f, mRenderer->getScreenHeight() * 0.1f);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText(_("DOWNLOADING") + " 0%");
    mBusyAnim.onSizeChanged();

    mDownloadThread = std::make_unique<std::thread>(&GuiRomMDownload::downloadInBackground, this);
}

GuiRomMDownload::~GuiRomMDownload()
{
    if (mDownloadThread) {
        mDownloadThread->join();
        mDownloadThread.reset();
    }
}

void GuiRomMDownload::downloadInBackground()
{
    const std::string serverURL {Settings::getInstance()->getString("RomMServerURL")};
    const std::string token {Settings::getInstance()->getString("RomMToken")};
    const int romId {atoi(mGame->metadata.get("rommid").c_str())};

    // Always re-fetch the rom's current detail rather than trusting anything cached from an
    // earlier sync - has_multiple_files/files could have changed on the server since, and this
    // is the only place that needs them (RomMLibrarySync only ever needed has_multiple_files
    // transiently, to pick the synthetic entry's extension).
    RomMApiClient client {serverURL, token};
    RomMApiClient::Rom rom;
    if (client.fetchRomById(romId, rom) && rom.hasMultipleFiles && !rom.files.empty()) {
        mMultiDisc = true;
        downloadMultiDiscInBackground(rom);
    }
    else {
        downloadSingleFileInBackground();
    }

    // Any HttpReq instances go out of scope inside the methods above, closing the partially- or
    // fully-written file(s) - this must happen before finishOnMainThread() renames or deletes
    // them below.
    mDone = true;
    mDownloading = false;
}

void GuiRomMDownload::downloadSingleFileInBackground()
{
    const std::string serverURL {Settings::getInstance()->getString("RomMServerURL")};
    const std::string token {Settings::getInstance()->getString("RomMToken")};

    RomMApiClient client {serverURL, token};
    const std::string url {
        client.getDownloadUrl(atoi(mGame->metadata.get("rommid").c_str()), mGame->getFileName())};

    HttpReq request {url, false, mTmpPath, token};

    while (!mAbort) {
        SDL_Delay(16);
        const HttpReq::Status status {request.status()};
        if (status == HttpReq::REQ_SUCCESS) {
            mSuccess = true;
            break;
        }
        else if (status != HttpReq::REQ_IN_PROGRESS) {
            mSuccess = false;
            if (request.getHttpStatusCode() == 401) {
                mErrorMessage = _("AUTHENTICATION FAILED, CHECK THE ROMM CREDENTIALS");
            }
            else if (status == HttpReq::REQ_IO_ERROR || status == HttpReq::REQ_UNDEFINED_ERROR) {
                mErrorMessage = _("NETWORK ERROR, COULD NOT REACH THE ROMM SERVER");
            }
            else {
                mErrorMessage =
                    Utils::String::format(_("DOWNLOAD FAILED: %s"), request.getErrorMsg().c_str());
            }
            LOG(LogWarning) << "RomM download: Failed to download \"" << mGame->getFileName()
                            << "\": " << request.getErrorMsg();
            break;
        }
        else {
            const long downloaded {request.getDownloadedBytes()};
            const long total {request.getTotalBytes()};
            if (total > 0)
                mPercentage = static_cast<int>((downloaded * 100) / total);
        }
    }
}

void GuiRomMDownload::downloadMultiDiscInBackground(const RomMApiClient::Rom& rom)
{
    const std::string serverURL {Settings::getInstance()->getString("RomMServerURL")};
    const std::string token {Settings::getInstance()->getString("RomMToken")};
    RomMApiClient client {serverURL, token};

    if (!Utils::FileSystem::createDirectory(mTmpPath)) {
        mSuccess = false;
        mErrorMessage = _("DOWNLOAD FAILED: COULDN'T CREATE THE DESTINATION DIRECTORY");
        LOG(LogWarning) << "RomM download: Failed to create directory \"" << mTmpPath << "\"";
        return;
    }

    // The wrapper directory's own name (e.g. "Chrono Cross (USA).m3u") is also what the
    // playlist file inside it must be named - FileData::launchGame() resolves a directory-typed
    // GAME entry by looking inside it for a file with that exact name (see
    // SystemData::populateFolder() for the ES-DE convention this reuses).
    const std::string wrapperName {Utils::FileSystem::getFileName(mGame->getPath())};

    // If RomM already lists a .m3u among the rom's files, use it (renamed to wrapperName) -
    // it's RomM's own authoritative disc order/content. Otherwise one is synthesized below from
    // whichever disc files actually get downloaded.
    const RomMApiClient::RomFile* rommM3u {nullptr};
    int64_t totalBytes {0};
    for (const auto& file : rom.files) {
        totalBytes += file.sizeBytes;
        if (Utils::String::toLower(Utils::FileSystem::getExtension(file.fileName)) == ".m3u")
            rommM3u = &file;
    }
    if (totalBytes <= 0)
        totalBytes = rom.fsSizeBytes;

    int64_t completedBytes {0};
    std::vector<std::string> discFileNames;

    for (const auto& file : rom.files) {
        if (mAbort)
            break;

        const bool isM3u {&file == rommM3u};
        const std::string destPath {mTmpPath + "/" + (isM3u ? wrapperName : file.fileName)};
        const std::string url {client.getFileDownloadUrl(file.id, file.fileName)};
        HttpReq request {url, false, destPath, token};

        bool fileDone {false};
        while (!mAbort && !fileDone) {
            SDL_Delay(16);
            const HttpReq::Status status {request.status()};
            if (status == HttpReq::REQ_SUCCESS) {
                fileDone = true;
            }
            else if (status != HttpReq::REQ_IN_PROGRESS) {
                mSuccess = false;
                if (request.getHttpStatusCode() == 401) {
                    mErrorMessage = _("AUTHENTICATION FAILED, CHECK THE ROMM CREDENTIALS");
                }
                else if (status == HttpReq::REQ_IO_ERROR ||
                         status == HttpReq::REQ_UNDEFINED_ERROR) {
                    mErrorMessage = _("NETWORK ERROR, COULD NOT REACH THE ROMM SERVER");
                }
                else {
                    mErrorMessage = Utils::String::format(_("DOWNLOAD FAILED: %s"),
                                                          request.getErrorMsg().c_str());
                }
                LOG(LogWarning) << "RomM download: Failed to download \"" << file.fileName
                                << "\": " << request.getErrorMsg();
                return;
            }
            else {
                const long downloaded {request.getDownloadedBytes()};
                if (totalBytes > 0) {
                    mPercentage =
                        static_cast<int>(((completedBytes + downloaded) * 100) / totalBytes);
                }
            }
        }

        if (mAbort)
            return;

        completedBytes += file.sizeBytes;
        if (!isM3u && file.category != "manual" && file.category != "soundtrack" &&
            file.category != "screenshot")
            discFileNames.emplace_back(file.fileName);
    }

    if (rommM3u == nullptr) {
        // RomM didn't include a playlist of its own - synthesize one from the disc-image files
        // that were actually downloaded, in RomM's own file order.
        std::ofstream m3u {mTmpPath + "/" + wrapperName, std::ios::binary};
        for (const std::string& fileName : discFileNames)
            m3u << fileName << "\n";
    }

    mSuccess = true;
}

void GuiRomMDownload::finishOnMainThread()
{
    // A multi-disc download's mTmpPath is a directory (one entry per disc plus the .m3u
    // playlist); a single-file download's is a plain file.
    const auto removeTmpPath = [this]() {
        if (mMultiDisc)
            Utils::FileSystem::removeDirectory(mTmpPath, true);
        else
            Utils::FileSystem::removeFile(mTmpPath);
    };

    if (mAbort) {
        removeTmpPath();
        return;
    }

    if (!mSuccess) {
        removeTmpPath();
        mWindow->pushGui(new GuiMsgBox(mErrorMessage));
        return;
    }

    // Utils::FileSystem::renameFile() returns std::rename()'s raw C return value converted to
    // bool - i.e. truthy means FAILURE (matches every other call site in the codebase, e.g.
    // GuiApplicationUpdater.cpp's AppImage replacement). It works equally for renaming a
    // directory into place (the multi-disc case) since its only guard is against overwriting an
    // already-existing directory at the destination, which mGame->getPath() never is yet here.
    if (Utils::FileSystem::renameFile(mTmpPath, mGame->getPath(), true)) {
        LOG(LogError) << "RomM download: Downloaded \"" << mTmpPath
                      << "\" but couldn't rename it to \"" << mGame->getPath() << "\"";
        removeTmpPath();
        mWindow->pushGui(new GuiMsgBox(_("DOWNLOAD SUCCEEDED BUT THE FILE COULDN'T BE SAVED, CHECK "
                                         "PERMISSIONS ON THE ROM DIRECTORY")));
        return;
    }

    if (mMultiDisc) {
        const std::string newName {Utils::FileSystem::getFileName(mGame->getPath())};
        const std::string extension {Utils::FileSystem::getExtension(newName)};
        const std::string oldName {newName.substr(0, newName.size() - extension.size())};
        migrateMultiDiscMedia(oldName, newName, mGame->getSystemName());
    }

    // Favorite now persists as ordinary metadata - the local-only intent entry is no longer needed.
    RomMLocalFavorites::getInstance().setFavorite(atoi(mGame->metadata.get("rommid").c_str()),
                                                  false);

    // Clears the badge on the next onFileChanged() and lets GamelistFileParser's write-side
    // guard stop excluding this entry, so it gets persisted to gamelist.xml normally.
    mGame->metadata.set("rommremote", "false");
    mGame->getSystem()->onMetaDataSavePoint();
    ViewController::getInstance()->onFileChanged(mGame, true);

    // Per design: no auto-launch, a second explicit select/launch is required.
    mWindow->pushGui(new GuiMsgBox(Utils::String::format(
        _("%s HAS BEEN DOWNLOADED"), Utils::String::toUpper(mGame->metadata.get("name")).c_str())));
}

void GuiRomMDownload::update(int deltaTime)
{
    if (mDownloading)
        mBusyAnim.update(deltaTime);

    if (mDone) {
        mDone = false;
        finishOnMainThread();
        delete this;
        return;
    }

    mBusyAnim.setText(mAbort ? _("CANCELLING...") :
                               _("DOWNLOADING") + " " + std::to_string(mPercentage.load()) + "%");

    GuiComponent::update(deltaTime);
}

void GuiRomMDownload::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);

    if (mDownloading)
        mBusyAnim.render(trans);
}

bool GuiRomMDownload::input(InputConfig* config, Input input)
{
    if (input.value != 0 && !mAbort &&
        (config->isMappedTo("b", input) || config->isMappedTo("back", input))) {
        // Require an explicit confirmation before actually cancelling, so a stray back-button
        // press doesn't throw away an in-progress download of a potentially large file. While
        // this confirm is showing it becomes the top of the GUI stack, so it receives input
        // (and update()) instead of this component - the background download thread keeps
        // running independently of that either way.
        mWindow->pushGui(new GuiMsgBox(
            _("CANCEL THE DOWNLOAD?"), _("YES"), [this] { mAbort = true; }, _("NO"), nullptr));
    }

    // Block all other input while downloading, matching GuiGameImporter's behavior for its own
    // background-thread-driven step.
    return true;
}

std::vector<HelpPrompt> GuiRomMDownload::getHelpPrompts() { return std::vector<HelpPrompt>(); }

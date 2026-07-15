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
#include "Settings.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

#include <SDL2/SDL_timer.h>

#include <cstdlib>

GuiRomMDownload::GuiRomMDownload(FileData* game)
    : mRenderer {Renderer::getInstance()}
    , mGame {game}
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

    RomMApiClient client {serverURL, token};
    const std::string url {
        client.getDownloadUrl(atoi(mGame->metadata.get("rommid").c_str()), mGame->getFileName())};

    HttpReq request {url, false, "", "", mTmpPath, token};

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
                mErrorMessage = Utils::String::format(
                    _("DOWNLOAD FAILED: %s"), request.getErrorMsg().c_str());
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

    // request goes out of scope here, closing the partially- or fully-written .tmp file -
    // this must happen before finishOnMainThread() renames or deletes it below.
    mDone = true;
    mDownloading = false;
}

void GuiRomMDownload::finishOnMainThread()
{
    if (mAbort) {
        Utils::FileSystem::removeFile(mTmpPath);
        return;
    }

    if (!mSuccess) {
        Utils::FileSystem::removeFile(mTmpPath);
        mWindow->pushGui(new GuiMsgBox(mErrorMessage));
        return;
    }

    // Utils::FileSystem::renameFile() returns std::rename()'s raw C return value converted to
    // bool - i.e. truthy means FAILURE (matches every other call site in the codebase, e.g.
    // GuiApplicationUpdater.cpp's AppImage replacement).
    if (Utils::FileSystem::renameFile(mTmpPath, mGame->getPath(), true)) {
        LOG(LogError) << "RomM download: Downloaded \"" << mTmpPath
                      << "\" but couldn't rename it to \"" << mGame->getPath() << "\"";
        Utils::FileSystem::removeFile(mTmpPath);
        mWindow->pushGui(
            new GuiMsgBox(_("DOWNLOAD SUCCEEDED BUT THE FILE COULDN'T BE SAVED, CHECK "
                            "PERMISSIONS ON THE ROM DIRECTORY")));
        return;
    }

    // Clears the badge on the next onFileChanged() and lets GamelistFileParser's write-side
    // guard stop excluding this entry, so it gets persisted to gamelist.xml normally.
    mGame->metadata.set("rommremote", "false");
    ViewController::getInstance()->onFileChanged(mGame, true);

    // Per design: no auto-launch, a second explicit select/launch is required.
    mWindow->pushGui(new GuiMsgBox(
        Utils::String::format(_("%s HAS BEEN DOWNLOADED"),
                              Utils::String::toUpper(mGame->metadata.get("name")).c_str())));
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

    // Block all other input while downloading, matching GuiRomMSync's behavior for its own
    // background-thread-driven step.
    return true;
}

std::vector<HelpPrompt> GuiRomMDownload::getHelpPrompts() { return std::vector<HelpPrompt>(); }

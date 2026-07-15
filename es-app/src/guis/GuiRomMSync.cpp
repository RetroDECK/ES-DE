//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMSync.cpp
//

#include "guis/GuiRomMSync.h"

#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "RomM/RomMPlatformMapping.h"
#include "Settings.h"
#include "SystemData.h"
#include "guis/GuiMsgBox.h"
#include "utils/FileSystemUtil.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"
#include "views/GamelistView.h"
#include "views/ViewController.h"

#include <unordered_set>

GuiRomMSync::GuiRomMSync()
    : mRenderer {Renderer::getInstance()}
    , mSyncing {true}
    , mDoneSyncing {false}
    , mSystemsAdded {0}
    , mSystemsRemoved {0}
{
    setSize(mRenderer->getScreenWidth() * 0.4f, mRenderer->getScreenHeight() * 0.1f);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText(_("SYNCING ROMM LIBRARY..."));
    mBusyAnim.onSizeChanged();

    activatePendingSystems();

    mSyncThread = std::make_unique<std::thread>(&GuiRomMSync::fetchInBackground, this);
}

void GuiRomMSync::activatePendingSystems()
{
    bool anyActivated {false};

    for (const auto& mapping : RomMPlatformMapping::getInstance().getAllMappings()) {
        // Not enabled, or already tied to a real (or previously activated) local system.
        if (!mapping.syncEnabled || !mapping.systemName.empty())
            continue;

        for (const auto& tmpl : SystemData::sInactiveSystemTemplates) {
            bool matched {false};
            for (const std::string& token :
                Utils::String::delimitedStringToVector(tmpl.platform, ",")) {
                const std::string platformName {Utils::String::trim(token)};
                if (!platformName.empty() &&
                    RomMApiClient::platformNameMatches(platformName, mapping.platformSlug,
                                                       mapping.platformFsSlug)) {
                    matched = true;
                    break;
                }
            }
            if (!matched)
                continue;

            if (Utils::FileSystem::createDirectory(tmpl.path)) {
                RomMPlatformMapping::getInstance().setSystemNameForPlatform(mapping.rommPlatformId,
                                                                             tmpl.name);
                anyActivated = true;
            }
            else {
                LOG(LogWarning) << "RomM sync: Failed to create ROM directory for pending "
                                   "platform \""
                                << mapping.platformSlug << "\": " << tmpl.path;
            }
            break;
        }
    }

    if (anyActivated)
        ViewController::getInstance()->rescanROMDirectory();
}

GuiRomMSync::~GuiRomMSync()
{
    if (mSyncThread) {
        mSyncThread->join();
        mSyncThread.reset();
    }
}

void GuiRomMSync::fetchInBackground()
{
    const std::string serverURL {Settings::getInstance()->getString("RomMServerURL")};
    const std::string token {Settings::getInstance()->getString("RomMToken")};

    for (auto system : SystemData::sSystemVector) {
        const RomMSystemMapping* mapping {
            RomMPlatformMapping::getInstance().getMapping(system->getName())};
        if (mapping == nullptr || !mapping->syncEnabled || mapping->rommPlatformId < 0)
            continue;

        RomMApiClient client {serverURL, token};
        SystemSyncResult result;
        result.system = system;
        result.roms = client.fetchRoms(mapping->rommPlatformId);
        if (result.roms.empty() && !client.lastError().empty()) {
            LOG(LogWarning) << "RomM sync: Failed to fetch roms for system \"" << system->getName()
                            << "\": " << client.lastError();
        }
        mResults.emplace_back(std::move(result));
    }

    mSyncing = false;
    mDoneSyncing = true;
}

void GuiRomMSync::applyResults()
{
    for (auto& result : mResults) {
        SystemData* system {result.system};
        FileData* rootFolder {system->getRootFolder()};
        FileFilterIndex* fileIndex {system->getIndex()};

        std::unordered_set<std::string> seenFileNames;
        bool addedAny {false};

        for (const auto& rom : result.roms) {
            if (rom.fsName.empty())
                continue;
            seenFileNames.insert(rom.fsName);

            const std::unordered_map<std::string, FileData*>& children {
                rootFolder->getChildrenByFilename()};
            auto it {children.find(rom.fsName)};
            if (it != children.cend()) {
                FileData* existing {it->second};
                if (existing->metadata.get("rommremote") == "true") {
                    // Refresh metadata for an already-synthesized remote entry.
                    existing->metadata.set("name", rom.name);
                    if (!rom.summary.empty())
                        existing->metadata.set("desc", rom.summary);
                    existing->metadata.set("rommid", std::to_string(rom.id));
                }
                // If it's a real local file already, leave it untouched.
                continue;
            }

            // New remote-only entry. The synthetic path is exactly where the file will land
            // once downloaded, so no other code needs to change once that happens.
            const std::string syntheticPath {system->getStartPath() + "/" + rom.fsName};
            FileData* newGame {
                new FileData(GAME, syntheticPath, system->getSystemEnvData(), system)};
            newGame->metadata.set("name", rom.name);
            if (!rom.summary.empty())
                newGame->metadata.set("desc", rom.summary);
            newGame->metadata.set("rommremote", "true");
            newGame->metadata.set("rommid", std::to_string(rom.id));
            rootFolder->addChild(newGame);
            fileIndex->addToIndex(newGame);
            addedAny = true;
            ++mSystemsAdded;
        }

        // Remove remote entries that no longer exist on the server.
        std::vector<FileData*> toRemove;
        for (FileData* file : rootFolder->getFilesRecursive(GAME)) {
            if (file->metadata.get("rommremote") == "true" &&
                seenFileNames.find(Utils::FileSystem::getFileName(file->getPath())) ==
                    seenFileNames.cend())
                toRemove.push_back(file);
        }
        for (FileData* file : toRemove) {
            ViewController::getInstance()->getGamelistView(system)->remove(file, false);
            ++mSystemsRemoved;
        }

        if (addedAny || !toRemove.empty()) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                             Settings::getInstance()->getBool("FavoritesFirst"));
            ViewController::getInstance()->onFileChanged(rootFolder, true);
        }
    }
}

void GuiRomMSync::update(int deltaTime)
{
    if (mSyncing)
        mBusyAnim.update(deltaTime);

    if (mDoneSyncing) {
        mDoneSyncing = false;
        applyResults();

        const std::string message {
            Utils::String::format(_("ROMM SYNC COMPLETE\n%d GAME(S) ADDED\n%d GAME(S) REMOVED"),
                                  mSystemsAdded, mSystemsRemoved)};
        mWindow->pushGui(new GuiMsgBox(message));
        delete this;
        return;
    }

    GuiComponent::update(deltaTime);
}

void GuiRomMSync::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);

    if (mSyncing)
        mBusyAnim.render(trans);
}

bool GuiRomMSync::input(InputConfig* config, Input input)
{
    // Block all input while syncing, matching GuiGameImporter's behavior for its own
    // background-thread-driven inventory step.
    return true;
}

std::vector<HelpPrompt> GuiRomMSync::getHelpPrompts() { return std::vector<HelpPrompt>(); }

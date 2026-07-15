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

#include <cstdlib>
#include <unordered_map>
#include <unordered_set>

namespace
{
    // Filesystem-illegal characters across the platforms ES-DE supports (Windows is the most
    // restrictive of them), replaced with underscores so RomM's freeform game titles are always
    // safe to use verbatim as local file names.
    std::string sanitizeForFileName(const std::string& input)
    {
        static const std::string illegal {"\\/:*?\"<>|"};
        std::string result;
        result.reserve(input.size());
        for (char c : input)
            result += (illegal.find(c) != std::string::npos) ? '_' : c;
        result = Utils::String::trim(result);
        // Trailing dots/spaces are also illegal in a Windows file name.
        while (!result.empty() && (result.back() == '.' || result.back() == ' '))
            result.pop_back();
        return result;
    }

    // Builds a region/revision/language tag from RomM's own parsed rom metadata (never guessed
    // from the filename), e.g. "(USA) (Rev 1)". Returns an empty string if none of these fields
    // are populated for this rom.
    std::string buildVariantTag(const RomMApiClient::Rom& rom)
    {
        std::string tag;
        if (!rom.regions.empty()) {
            std::string joined;
            for (const auto& region : rom.regions)
                joined += (joined.empty() ? "" : ", ") + region;
            tag += "(" + joined + ")";
        }
        if (!rom.revision.empty())
            tag += (tag.empty() ? "" : " ") + std::string("(Rev ") + rom.revision + ")";
        if (!rom.languages.empty()) {
            std::string joined;
            for (const auto& language : rom.languages)
                joined += (joined.empty() ? "" : ",") + language;
            tag += (tag.empty() ? "" : " ") + std::string("(") + joined + ")";
        }
        return tag;
    }

    // Roms sharing the same title within a single sync batch would otherwise show up as
    // identically-named gamelist entries and collide once downloaded to the same directory. For
    // any such collision, appends a tag built from RomM's own region/revision/language metadata;
    // if that's empty for a colliding rom, falls back to a running counter so the name is still
    // guaranteed unique.
    std::unordered_map<int, std::string>
    buildDisplayNames(const std::vector<RomMApiClient::Rom>& roms)
    {
        std::unordered_map<std::string, int> nameCounts;
        for (const auto& rom : roms)
            ++nameCounts[Utils::String::toLower(rom.name)];

        std::unordered_map<int, std::string> displayNames;
        std::unordered_map<std::string, int> disambiguatorSeen;
        for (const auto& rom : roms) {
            if (nameCounts[Utils::String::toLower(rom.name)] <= 1) {
                displayNames[rom.id] = rom.name;
                continue;
            }
            const std::string tag {buildVariantTag(rom)};
            std::string candidate {tag.empty() ? rom.name : rom.name + " " + tag};
            const std::string candidateKey {Utils::String::toLower(candidate)};
            int& seen {disambiguatorSeen[candidateKey]};
            if (seen > 0)
                candidate += " (" + std::to_string(seen + 1) + ")";
            ++seen;
            displayNames[rom.id] = candidate;
        }
        return displayNames;
    }
} // namespace

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

        const std::unordered_map<int, std::string> displayNames {buildDisplayNames(result.roms)};

        // Index already-known RomM entries (remote or downloaded) by RomM rom id rather than by
        // file name - the local file name a still-remote entry uses can change between syncs as
        // the batch's title collisions change (a newly-added sibling rom can introduce a
        // collision that wasn't there before), so an id-based lookup is the only one that stays
        // correct regardless.
        std::unordered_map<int, FileData*> byRommId;
        for (FileData* file : rootFolder->getFilesRecursive(GAME)) {
            const std::string& rommId {file->metadata.get("rommid")};
            if (!rommId.empty())
                byRommId[atoi(rommId.c_str())] = file;
        }

        std::unordered_set<int> seenRomIds;
        bool addedAny {false};

        for (const auto& rom : result.roms) {
            if (rom.fsName.empty())
                continue;
            seenRomIds.insert(rom.id);

            // The name shown in the list is also the name the file will be saved under once
            // downloaded (GuiRomMDownload just saves to the synthetic FileData's own path), so
            // it needs to be filesystem-safe and keep the original extension.
            const std::string& displayName {displayNames.at(rom.id)};
            // RomM's list endpoint only reliably reports has_multiple_files, not the files
            // array itself (only hydrated on the single-rom detail endpoint GuiRomMDownload
            // re-fetches from at download time), so detection here relies on the flag alone.
            const bool isMultiDisc {rom.hasMultipleFiles};
            // Represented as a directory named "<title>.m3u" - ES-DE's existing convention for
            // multi-disc games (SystemData::populateFolder()): a directory whose name matches a
            // configured extension is treated as a single GAME entry, and
            // FileData::launchGame() looks inside it for a file with that exact name (the
            // downloaded disc files plus a synthesized .m3u playlist).
            std::string extension {isMultiDisc ? ".m3u" :
                                                 Utils::FileSystem::getExtension(rom.fsName)};
            // getExtension() returns "." for "no extension" - never emit a bare trailing dot.
            if (extension == ".")
                extension = ".m3u";
            const std::string localFileName {sanitizeForFileName(displayName) + extension};
            const std::string desiredPath {system->getStartPath() + "/" + localFileName};

            auto it {byRommId.find(rom.id)};
            if (it != byRommId.cend()) {
                FileData* existing {it->second};
                if (existing->metadata.get("rommremote") != "true") {
                    // Already downloaded - leave the real local file untouched.
                    continue;
                }
                if (existing->getPath() == desiredPath) {
                    // Still remote and already at the right synthetic path - just refresh.
                    existing->metadata.set("name", displayName);
                    if (!rom.summary.empty())
                        existing->metadata.set("desc", rom.summary);
                    existing->metadata.set("rommsize", std::to_string(rom.fsSizeBytes));
                    continue;
                }
                // The computed name changed since the last sync (e.g. a newly-added sibling rom
                // introduced a title collision) - a FileData's path can't be mutated in place,
                // so drop the stale synthetic entry and recreate it below with the new path.
                ViewController::getInstance()->getGamelistView(system)->remove(existing, false);
            }

            // New (or renamed) remote-only entry. The synthetic path is exactly where the file
            // will land once downloaded, using the same name the user sees in the list - so no
            // other code needs to change once that happens.
            FileData* newGame {
                new FileData(GAME, desiredPath, system->getSystemEnvData(), system)};
            newGame->metadata.set("name", displayName);
            if (!rom.summary.empty())
                newGame->metadata.set("desc", rom.summary);
            newGame->metadata.set("rommremote", "true");
            newGame->metadata.set("rommid", std::to_string(rom.id));
            newGame->metadata.set("rommsize", std::to_string(rom.fsSizeBytes));
            rootFolder->addChild(newGame);
            fileIndex->addToIndex(newGame);
            addedAny = true;
            ++mSystemsAdded;
        }

        // Remove remote entries that no longer exist on the server.
        std::vector<FileData*> toRemove;
        for (FileData* file : rootFolder->getFilesRecursive(GAME)) {
            if (file->metadata.get("rommremote") == "true" &&
                seenRomIds.find(atoi(file->metadata.get("rommid").c_str())) == seenRomIds.cend())
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

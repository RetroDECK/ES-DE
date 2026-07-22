//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMLibrarySync.cpp
//

#include "RomM/RomMLibrarySync.h"

#include "FileData.h"
#include "FileFilterIndex.h"
#include "Log.h"
#include "RomM/RomMCache.h"
#include "RomM/RomMPlatformMapping.h"
#include "Settings.h"
#include "SystemData.h"
#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
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
    std::unordered_map<int, std::string> buildDisplayNames(
        const std::vector<RomMApiClient::Rom>& roms)
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

RomMLibrarySync::RomMLibrarySync(bool forceFullResync)
    : mDoneSyncing {false}
    , mSystemsAdded {0}
    , mSystemsRemoved {0}
    , mForceFullResync {forceFullResync}
    , mTotalSystems {0}
    , mCompletedSystems {0}
{
}

RomMLibrarySync::~RomMLibrarySync()
{
    if (mSyncThread) {
        mSyncThread->join();
        mSyncThread.reset();
    }
}

void RomMLibrarySync::start()
{
    activatePendingSystems();
    mSyncThread = std::make_unique<std::thread>(&RomMLibrarySync::fetchInBackground, this);
}

void RomMLibrarySync::activatePendingSystems()
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

namespace
{
    // Resolved sequentially before any worker thread is spawned, so the RomMPlatformMapping/
    // RomMCache reads below need no synchronization.
    struct SyncTask {
        SystemData* system;
        int platformId;
        std::string storedCursor;
        std::vector<RomMCache::CachedRom> previousRoms;
        std::vector<RomMCache::CachedRom> mergeBaseRoms;
    };

    struct TaskResult {
        SystemData* system {nullptr};
        int platformId {-1};
        std::vector<RomMApiClient::Rom> finalRoms;
        std::vector<RomMCache::CachedRom> newCachedRoms;
        std::string cursorToStore;
        int addedCount {0};
        int removedCount {0};
    };

    // Bounded so as not to open more simultaneous connections than a modest, self-hosted RomM
    // server can comfortably handle (HttpReq requests all share one curl multi-handle, so several
    // in flight at once is otherwise free - see HttpReq::pollCurl()).
    constexpr size_t kMaxConcurrentPlatformFetches {4};
} // namespace

void RomMLibrarySync::fetchInBackground()
{
    const std::string serverURL {Settings::getInstance()->getString("RomMServerURL")};
    const std::string token {Settings::getInstance()->getString("RomMToken")};

    std::vector<SyncTask> tasks;
    for (auto system : SystemData::sSystemVector) {
        const RomMSystemMapping* mapping {
            RomMPlatformMapping::getInstance().getMapping(system->getName())};
        if (mapping == nullptr || !mapping->syncEnabled || mapping->rommPlatformId < 0)
            continue;

        SyncTask task;
        task.system = system;
        task.platformId = mapping->rommPlatformId;
        // The actual last-known cache, regardless of forceFullResync - used below both as the
        // failure-fallback (so a failed forced resync can't wipe a platform's remote list) and
        // to compute genuinely-new/genuinely-removed counts against the server's real state,
        // rather than against the in-memory FileData tree (which is always empty at this point
        // in every run, forced or not, since remote entries are never persisted - diffing
        // against that would misreport every rom as "added" on every single sync).
        task.previousRoms = RomMCache::getInstance().getRoms(task.platformId);
        task.storedCursor =
            mForceFullResync ? std::string() : RomMCache::getInstance().getCursor(task.platformId);
        // What to merge a successful fetch against - empty for a forced full resync, so stale
        // entries not present in the fresh authoritative fetch are naturally dropped.
        task.mergeBaseRoms =
            mForceFullResync ? std::vector<RomMCache::CachedRom>() : task.previousRoms;
        tasks.emplace_back(std::move(task));
    }

    mTotalSystems = static_cast<int>(tasks.size());

    std::vector<TaskResult> taskResults(tasks.size());

    // Worker thread: network I/O and Rom-vector bookkeeping only, no RomMCache/FileData access.
    auto fetchOne = [&serverURL, &token](const SyncTask& task, TaskResult& result) {
        result.system = task.system;
        result.platformId = task.platformId;

        // Captured before fetchRoms() (which may page across multiple requests) begins, and
        // used as the NEXT sync's updated_after cursor rather than the max updated_at seen in
        // this run's results - so a rom updated mid-fetch still gets picked up next time
        // instead of being permanently skipped.
        const std::string newCursor {RomMApiClient::formatTimestampUtc(Utils::Time::now())};

        RomMApiClient client {serverURL, token};
        std::vector<RomMApiClient::Rom> fetched {
            client.fetchRoms(task.platformId, task.storedCursor)};
        const bool fetchFailed {fetched.empty() && !client.lastError().empty()};

        result.cursorToStore = task.storedCursor;

        if (fetchFailed) {
            if (task.previousRoms.empty()) {
                // No prior cache at all (first-ever sync for this platform, forced or not) and
                // it failed outright - nothing to fall back to, matches the pre-caching
                // behavior (empty result).
                LOG(LogWarning) << "RomM sync: Initial full fetch failed for system \""
                                << task.system->getName() << "\": " << client.lastError();
            }
            else {
                // A delta (or forced full-resync) fetch failed but a prior cache exists -
                // reuse it as-is rather than telling applyResults() every previously known
                // remote rom just vanished from the server. The cursor is NOT advanced, so
                // nothing is silently skipped by a future updated_after filter.
                LOG(LogWarning) << "RomM sync: Fetch failed for system \"" << task.system->getName()
                                << "\", reusing cached rom list (" << task.previousRoms.size()
                                << " roms) as-is: " << client.lastError();
                for (const auto& cachedRom : task.previousRoms)
                    result.finalRoms.push_back(RomMCache::toApiRom(cachedRom));
                result.newCachedRoms = task.previousRoms;
            }
        }
        else {
            // Merge: mergeBaseRoms overlaid by every rom this fetch returned (whether that's
            // the platform's full list - first-ever sync/forced full resync/empty stored
            // cursor - or a delta of only changed roms since storedCursor). Delta entries
            // always win on id collision. Note this only ever ADDS/UPDATES - a rom deleted from
            // RomM stays in the cache (and keeps showing as available locally) until a forced
            // full resync, where mergeBaseRoms is empty and this merge collapses to exactly
            // "fetched", the server's authoritative current list, which is what actually
            // reconciles deletions.
            std::unordered_map<int, RomMApiClient::Rom> merged;
            for (const auto& cachedRom : task.mergeBaseRoms)
                merged[cachedRom.id] = RomMCache::toApiRom(cachedRom);
            for (auto& rom : fetched)
                merged[rom.id] = rom;

            for (auto& [romId, rom] : merged)
                result.finalRoms.push_back(std::move(rom));
            for (const auto& rom : result.finalRoms)
                result.newCachedRoms.push_back(RomMCache::fromApiRom(rom));
            result.cursorToStore = newCursor; // This fetch fully succeeded - safe to advance.
        }

        // Genuinely-new/genuinely-removed counts, against the real previous cache rather than
        // the (always momentarily empty) in-memory FileData tree - see the comment on
        // task.previousRoms above.
        std::unordered_set<int> previousIds;
        for (const auto& rom : task.previousRoms)
            previousIds.insert(rom.id);
        std::unordered_set<int> finalIds;
        for (const auto& rom : result.finalRoms)
            finalIds.insert(rom.id);
        for (int romId : finalIds) {
            if (previousIds.find(romId) == previousIds.cend())
                ++result.addedCount;
        }
        for (int romId : previousIds) {
            if (finalIds.find(romId) == finalIds.cend())
                ++result.removedCount;
        }
    };

    for (size_t batchStart {0}; batchStart < tasks.size();
         batchStart += kMaxConcurrentPlatformFetches) {
        const size_t batchEnd {std::min(tasks.size(), batchStart + kMaxConcurrentPlatformFetches)};
        std::vector<std::thread> workers;
        for (size_t i {batchStart}; i < batchEnd; ++i)
            workers.emplace_back(fetchOne, std::cref(tasks[i]), std::ref(taskResults[i]));
        for (auto& worker : workers)
            worker.join();
        mCompletedSystems = static_cast<int>(batchEnd);
    }

    // Every worker has joined, so touching RomMCache (not thread-safe) here is safe.
    for (auto& result : taskResults) {
        RomMCache::getInstance().setPlatform(result.platformId, result.cursorToStore,
                                             result.newCachedRoms);
        mSystemsAdded += result.addedCount;
        mSystemsRemoved += result.removedCount;

        SystemSyncResult syncResult;
        syncResult.system = result.system;
        syncResult.roms = std::move(result.finalRoms);
        mResults.emplace_back(std::move(syncResult));
    }

    RomMCache::getInstance().flush();

    mDoneSyncing = true;
}

void RomMLibrarySync::applyResults()
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
            //
            // A rom the user already owns as a plain local file (not tracked via rommid, so
            // byRommId above missed it) would collide here on filename - mChildrenByFilename is
            // keyed by FileData::getKey(), just the filename for plain FileData (see FileData.h).
            // Skip rather than risk mislinking by name alone - addChild() below would otherwise
            // silently no-op on the collision and leak the newly constructed object.
            const std::string desiredFileName {Utils::FileSystem::getFileName(desiredPath)};
            const auto& childrenByFilename = rootFolder->getChildrenByFilename();
            if (childrenByFilename.find(desiredFileName) != childrenByFilename.cend()) {
                LOG(LogWarning) << "RomM sync: Skipping rom \"" << displayName << "\" for system \""
                                << system->getName() << "\" as the filename \"" << desiredFileName
                                << "\" is already in use";
                continue;
            }

            FileData* newGame {new FileData(GAME, desiredPath, system->getSystemEnvData(), system)};
            newGame->metadata.set("name", displayName);
            if (!rom.summary.empty())
                newGame->metadata.set("desc", rom.summary);
            newGame->metadata.set("rommremote", "true");
            newGame->metadata.set("rommid", std::to_string(rom.id));
            newGame->metadata.set("rommsize", std::to_string(rom.fsSizeBytes));
            rootFolder->addChild(newGame);
            fileIndex->addToIndex(newGame);
            addedAny = true;
        }

        // Remove remote entries that no longer exist in result.roms. Note this is purely about
        // reconstructing the in-memory FileData tree (which starts empty every run, since
        // remote entries are never persisted) to match result.roms - it does NOT feed
        // mSystemsAdded/mSystemsRemoved, which are computed once in fetchInBackground() against
        // the actual previous cache instead, since every rom here would otherwise misreport as
        // "added"/"removed" on every single run regardless of whether anything really changed.
        std::vector<FileData*> toRemove;
        for (FileData* file : rootFolder->getFilesRecursive(GAME)) {
            if (file->metadata.get("rommremote") == "true" &&
                seenRomIds.find(atoi(file->metadata.get("rommid").c_str())) == seenRomIds.cend())
                toRemove.push_back(file);
        }
        for (FileData* file : toRemove)
            ViewController::getInstance()->getGamelistView(system)->remove(file, false);

        if (addedAny || !toRemove.empty()) {
            rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                             Settings::getInstance()->getBool("FavoritesFirst"));
            ViewController::getInstance()->onFileChanged(rootFolder, true);
        }
    }
}

void RomMLibrarySync::removeAllRemoteEntries()
{
    for (auto system : SystemData::sSystemVector) {
        // Collections don't own FileData of their own - they reference the owning system's
        // entries, which are handled when this loop reaches that system.
        if (system->isCollection())
            continue;

        FileData* rootFolder {system->getRootFolder()};
        std::vector<FileData*> toRemove;
        for (FileData* file : rootFolder->getFilesRecursive(GAME)) {
            if (file->metadata.get("rommremote") == "true")
                toRemove.push_back(file);
        }
        if (toRemove.empty())
            continue;

        for (FileData* file : toRemove)
            ViewController::getInstance()->getGamelistView(system)->remove(file, false);

        rootFolder->sort(rootFolder->getSortTypeFromString(rootFolder->getSortTypeString()),
                         Settings::getInstance()->getBool("FavoritesFirst"));
        ViewController::getInstance()->onFileChanged(rootFolder, true);
    }
}

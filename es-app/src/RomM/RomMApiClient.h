//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMApiClient.h
//
//  Thin client for the subset of the RomM (https://github.com/rommapp/romm) REST API that
//  ES-DE needs: listing platforms/roms and downloading rom files. Used by the library sync
//  that lists not-yet-downloaded games inline in the gamelists.
//
//  Every method performs one or more blocking network calls and must therefore only be
//  invoked from a background thread, never from the render/main thread.
//

#ifndef ES_APP_ROMM_ROMM_API_CLIENT_H
#define ES_APP_ROMM_ROMM_API_CLIENT_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

class HttpReq;

class RomMApiClient
{
public:
    struct Platform {
        int id {0};
        std::string slug;
        std::string fsSlug;
        std::string name;
    };

    // One disc/part of a multi-file rom (RomM's RomFileSchema). "id" here identifies the file
    // itself, not the rom - required by getFileDownloadUrl() below.
    struct RomFile {
        int id {0};
        std::string fileName;
        int64_t sizeBytes {0};
        // Nullable in RomM's API (game/dlc/manual/patch/update/mod/demo/translation/prototype/
        // cheat/soundtrack/screenshot) - empty string if unset.
        std::string category;
    };

    struct Rom {
        int id {0};
        std::string name;
        std::string summary;
        std::string fsName;
        int64_t fsSizeBytes {0};
        std::string urlCover;
        // RomM's own re-hosted cover, relative to the server root - preferred over urlCover
        // (see resolveCoverUrl()), which links directly to the upstream provider and gets
        // rejected for direct client requests.
        std::string pathCoverLarge;
        std::string pathCoverSmall;
        std::vector<std::string> genres;
        std::vector<std::string> companies;
        // Unix timestamp (seconds since epoch), or 0 if unset. RomM's API itself reports this
        // in milliseconds - already divided down to seconds by the time it lands here.
        int64_t firstReleaseDate {0};
        float averageRating {0.0f};
        // Free-form string, e.g. "1-4" - not a pure integer in RomM's API.
        std::string playerCount;
        // RomM's own parsed identification of the rom file's variant, e.g. "1" for a rom named
        // "Some Game (USA) (Rev 1).sfc" - used to tell apart roms that otherwise share the same
        // "name" (different regional/revision releases of one game).
        std::string revision;
        std::vector<std::string> regions;
        std::vector<std::string> languages;
        // True for roms spanning multiple discs/parts. RomM's own content endpoint bundles
        // these as a zip, which ES-DE never unpacks - see getFileDownloadUrl() for how these
        // are actually downloaded instead (one file at a time, no zip involved).
        bool hasMultipleFiles {false};
        std::vector<RomFile> files;
        // Unix timestamp (seconds since epoch, UTC) parsed from RomM's "updated_at", or 0 if
        // unset/unparsed. Diagnostic only - NOT used as the incremental sync cursor (see
        // RomMLibrarySync, which uses the wall-clock fetch-start time instead, to avoid a race
        // where a rom updated mid-fetch would otherwise be permanently skipped by a future
        // updated_after filter).
        int64_t updatedAt {0};
        // The following come from "rom_user" - per-user state not driven by rom.updated_at, so
        // an incremental (updated_after-filtered) sync can miss a fresh change to any of these.
        int64_t lastPlayed {0};
        bool userHidden {false};
        // 0-10 (RomM's own scale), distinct from metadatum.average_rating (0-100, community).
        int userRating {0};
        std::string userStatus;
    };

    RomMApiClient(const std::string& serverURL, const std::string& token);

    // Returns all platforms configured on the RomM server. Returns an empty vector and sets
    // lastError() on failure.
    std::vector<Platform> fetchPlatforms();

    // Pages through and returns every rom belonging to the given RomM platform id. If
    // updatedAfterUtc is non-empty (see RomMUtils::formatTimestampUtc()), restricts the result
    // to roms whose updated_at is after that timestamp instead of the platform's full list -
    // pass "" for the original full-fetch behavior. Returns an empty vector and sets lastError() on
    // failure (an empty vector is also returned, with no error, if the platform/delta
    // genuinely has no roms). If set, onRomsFetched(1, total) is called once per rom parsed -
    // total is scoped to this exact query, so an incremental fetch's total is just what changed,
    // not the platform's full library.
    std::vector<Rom> fetchRoms(int platformId,
                               const std::string& updatedAfterUtc = "",
                               const std::function<void(int, int)>& onRomsFetched = nullptr);

    // Fetches the full, current detail for a single rom by its RomM id (e.g. the "rommid"
    // metadata already stashed on a synced FileData). Used at download time to get an
    // up-to-date has_multiple_files/files list rather than trusting anything cached from an
    // earlier sync. Returns true and populates outRom on success, false otherwise.
    bool fetchRomById(int romId, Rom& outRom);

    // Builds the URL for downloading a rom's file content. Unlike the fetch*() methods above,
    // this doesn't perform a network call itself - the caller is expected to stream the
    // download via its own HttpReq (see GuiRomMDownload), since that needs to run on a
    // dedicated background thread with live progress tracking.
    std::string getDownloadUrl(int romId, const std::string& fsName) const;

    // Builds the URL for downloading a single disc/part of a multi-file rom (Rom::files[i]).
    // Unlike getDownloadUrl(), fileId identifies the individual file record, not the rom itself.
    std::string getFileDownloadUrl(int fileId, const std::string& fileName) const;

    const std::string& lastError() const { return mLastError; }

    // Resolves the best available cover URL for a rom, preferring RomM's own re-hosted
    // path_cover_large/path_cover_small (prefixed with serverURL, with the cache-busting
    // query's raw space encoded) over the upstream url_cover, which is only trusted when its
    // path ends in a plausible image extension. Returns "" (and leaves outFormat untouched) if
    // no usable cover is available. Mirrors the equivalent inline logic in
    // scrapers/RomM.cpp's processGame().
    static std::string resolveCoverUrl(const std::string& serverURL,
                                       const Rom& rom,
                                       std::string& outFormat);

private:
    std::string buildUrl(const std::string& path) const;

    // Blocks the calling thread until the request leaves REQ_IN_PROGRESS (success, error, or
    // maxWaitMs elapses). Returns true if the request succeeded.
    bool waitForRequest(HttpReq& req, int maxWaitMs = 30000);

    std::string mServerURL;
    std::string mToken;
    std::string mLastError;
};

#endif // ES_APP_ROMM_ROMM_API_CLIENT_H

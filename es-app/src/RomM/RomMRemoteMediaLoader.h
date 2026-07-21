//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMRemoteMediaLoader.h
//
//  On-demand, in-memory fetching of cover image bytes for not-yet-downloaded ("rommremote")
//  games, triggered when the gamelist view lands on such an entry rather than during sync.
//  Modeled directly on GuiScraperSearch's thumbnail-fetch pattern (a keyed map of non-blocking
//  HttpReq instances, polled once per frame) - see GuiScraperSearch::updateInfoPane()/
//  updateThumbnail(). Main-thread only; never touches disk or persists anything itself.
//
//  Sync (RomMLibrarySync::applyResults(), main thread) already has every rom's full record in
//  hand and calls setCoverSource() there to record the cover URL already resolved by
//  RomMApiClient::resolveCoverUrl() - no additional per-rom network call is needed to learn that
//  URL. Only the actual image bytes are genuinely lazy: the view layer calls requestCover() (no
//  URL needed - just the rom id) once it wants to display a given entry, which is what triggers
//  the real, non-blocking, HttpReq-backed fetch.
//

#ifndef ES_APP_ROMM_ROMM_REMOTE_MEDIA_LOADER_H
#define ES_APP_ROMM_ROMM_REMOTE_MEDIA_LOADER_H

#include <memory>
#include <string>
#include <unordered_map>

class HttpReq;

class RomMRemoteMediaLoader
{
public:
    enum class State {
        NOT_REQUESTED, // Source known (or not yet set) but no fetch started this session.
        IN_PROGRESS, // HttpReq in flight.
        READY, // Bytes available via getCoverBytes().
        FAILED, // Request error (network, non-2xx, etc.) - not retried automatically.
        NO_MEDIA // No usable URL was ever available for this rom - not retried.
    };

    static RomMRemoteMediaLoader& getInstance();

    // Records the cover source URL for a rom, as resolved by RomMApiClient::resolveCoverUrl().
    // Called once per rom on every sync (RomMLibrarySync::applyResults()). Does not itself start
    // any network I/O. If the url is unchanged from what's already recorded, any existing fetch
    // state/bytes/in-flight request is left untouched (so re-syncing doesn't throw away
    // already-fetched art). If the url genuinely changed since last recorded (e.g. the server
    // regenerated the cover) and no fetch is currently in flight, any previously fetched
    // bytes/FAILED/NO_MEDIA state is reset so a future requestCover() call fetches the new
    // source instead of staying stuck on stale data. An empty url records a NO_MEDIA source.
    void setCoverSource(int rommId, const std::string& url, const std::string& format);

    // Starts a non-blocking fetch for rommId's cover using the source last recorded via
    // setCoverSource(), if one isn't already in flight/resolved/failed/known-absent. Safe to
    // call repeatedly (e.g. once per frame while the cursor sits on the same entry) - a no-op
    // once the entry has left NOT_REQUESTED. Returns NO_MEDIA immediately if no source was ever
    // recorded for this rom.
    State requestCover(int rommId);

    State getCoverState(int rommId) const;

    // Valid only once the corresponding state is READY.
    const std::string* getCoverBytes(int rommId) const;
    const std::string& getCoverFormat(int rommId) const;

    // Polls every in-flight HttpReq. Call once per frame (e.g. from GamelistView::update()).
    void update();

    // Drops all cached state (source, bytes, in-flight request) for a rom, e.g. once it's been
    // downloaded and its cover has been saved to disk, or once removed from RomM entirely.
    void forget(int rommId);

private:
    RomMRemoteMediaLoader() = default;

    struct Entry {
        State state {State::NOT_REQUESTED};
        std::string sourceUrl;
        std::string format;
        std::unique_ptr<HttpReq> req;
        std::string bytes;
    };

    std::unordered_map<int, Entry> mCovers;
};

#endif // ES_APP_ROMM_ROMM_REMOTE_MEDIA_LOADER_H

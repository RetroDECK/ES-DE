//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  RomMLocalFavorites.h
//
//  Local-only favorite marking for not-yet-downloaded RomM games. Deliberately never synced
//  to or from the RomM server - a still-remote FileData is rebuilt from scratch on every sync
//  and never written to gamelist.xml, so a favorite toggled via the UI has to be persisted
//  separately here, keyed by rommid, and reapplied by RomMLibrarySync each time that FileData
//  is rebuilt. Once a game is downloaded, its favorite value becomes ordinary local metadata
//  (persisted to gamelist.xml as usual) and its entry here is removed.
//

#ifndef ES_APP_ROMM_ROMM_LOCAL_FAVORITES_H
#define ES_APP_ROMM_ROMM_LOCAL_FAVORITES_H

#include <unordered_set>

class RomMLocalFavorites
{
public:
    static RomMLocalFavorites& getInstance();

    bool isFavorite(int rommId) const;
    // Persists to disk immediately - toggles are rare, user-initiated events, not part of a
    // sync's bulk write path.
    void setFavorite(int rommId, bool favorite);
    // Called on logout - rommids are meaningless once paired with a (possibly different) server.
    void clearAll();

private:
    RomMLocalFavorites();

    void loadFile();
    void flush();

    std::unordered_set<int> mFavoriteIds;
};

#endif // ES_APP_ROMM_ROMM_LOCAL_FAVORITES_H

//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistBase.h
//
//  Gamelist base class with utility functions and other low-level logic.
//

#ifndef ES_APP_VIEWS_GAMELIST_BASE_H
#define ES_APP_VIEWS_GAMELIST_BASE_H

#include "FileData.h"
#include "GuiComponent.h"
#include "SystemData.h"
#include "ThemeData.h"
#include "Window.h"
#include "components/BadgeComponent.h"
#include "components/DateTimeComponent.h"
#include "components/GIFAnimComponent.h"
#include "components/LottieAnimComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "components/TextComponent.h"
#include "components/TextListComponent.h"
#include "components/VideoFFmpegComponent.h"

#include <stack>

class GamelistBase : public GuiComponent
{
public:
    FileData* getCursor() { return mList.getSelected(); }
    void setCursor(FileData*);

    bool input(InputConfig* config, Input input) override;

    FileData* getNextEntry() { return mList.getNext(); }
    FileData* getPreviousEntry() { return mList.getPrevious(); }
    FileData* getFirstEntry() { return mList.getFirst(); }
    FileData* getLastEntry() { return mList.getLast(); }
    FileData* getFirstGameEntry() { return mFirstGameEntry; }

    // These functions are used to retain the folder cursor history, for instance
    // during a view reload. The calling function stores the history temporarily.
    void copyCursorHistory(std::vector<FileData*>& cursorHistory)
    {
        cursorHistory = mCursorStackHistory;
    }
    void populateCursorHistory(std::vector<FileData*>& cursorHistory)
    {
        mCursorStackHistory = cursorHistory;
    }

    void addPlaceholder(FileData*);

    void remove(FileData* game, bool deleteFile);
    void removeMedia(FileData* game);

    const std::vector<std::string>& getFirstLetterIndex() { return mFirstLetterIndex; }

protected:
    GamelistBase(FileData* root);
    ~GamelistBase();

    // Called when a FileData* is added, has its metadata changed, or is removed.
    virtual void onFileChanged(FileData* file, bool reloadGamelist) = 0;

    void populateList(const std::vector<FileData*>& files, FileData* firstEntry);

    void generateFirstLetterIndex(const std::vector<FileData*>& files);
    void generateGamelistInfo(FileData* cursor, FileData* firstEntry);

    virtual void launch(FileData* game) = 0;

    bool isListScrolling() override { return mList.isScrolling(); }
    void stopListScrolling() override { mList.stopScrolling(); }

    std::string getQuickSystemSelectRightButton() { return "right"; }
    std::string getQuickSystemSelectLeftButton() { return "left"; }

    FileData* mRoot;
    TextListComponent<FileData*> mList;

    // Points to the first game in the list, i.e. the first entry which is of the type "GAME".
    FileData* mFirstGameEntry;

    // This game is randomly selected in the grouped custom collections view.
    FileData* mRandomGame;
    FileData* mLastUpdated;

    std::stack<FileData*> mCursorStack;
    std::vector<FileData*> mCursorStackHistory;

    std::vector<std::string> mFirstLetterIndex;

    unsigned int mGameCount;
    unsigned int mFavoritesGameCount;
    unsigned int mFilteredGameCount;
    unsigned int mFilteredGameCountAll;
    bool mIsFiltered;
    bool mIsFolder;
    bool mVideoPlaying;

private:
};

#endif // ES_APP_VIEWS_GAMELIST_BASE_H

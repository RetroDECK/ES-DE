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
#include "components/VideoFFmpegComponent.h"
#include "components/primary/CarouselComponent.h"
#include "components/primary/GridComponent.h"
#include "components/primary/TextListComponent.h"

#include <stack>

class GamelistBase : public GuiComponent
{
public:
    FileData* getCursor() { return mPrimary->getSelected(); }
    void setCursor(FileData*);

    bool input(InputConfig* config, Input input) override;
    void enterDirectory(FileData* cursor);

    FileData* getNextEntry() { return mPrimary->getNext(); }
    FileData* getPreviousEntry() { return mPrimary->getPrevious(); }
    FileData* getFirstEntry() { return mPrimary->getFirst(); }
    FileData* getLastEntry() { return mPrimary->getLast(); }
    FileData* getFirstGameEntry() { return mFirstGameEntry; }

    void onDemandTextureLoad()
    {
        if (mPrimary != nullptr)
            mPrimary->onDemandTextureLoad();
    }

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

    void stopListScrolling() override { mPrimary->stopScrolling(); }

protected:
    GamelistBase(FileData* root);

    PrimaryComponent<FileData*>* getPrimary() { return mPrimary; }

    // Called when a FileData* is added, has its metadata changed, or is removed.
    virtual void onFileChanged(FileData* file, bool reloadGamelist) = 0;

    void populateList(const std::vector<FileData*>& files, FileData* firstEntry);

    void generateFirstLetterIndex(const std::vector<FileData*>& files);
    void generateGamelistInfo(FileData* cursor, FileData* firstEntry);

    virtual void launch(FileData* game) = 0;

    bool isListScrolling() override { return mPrimary->isScrolling(); }

    std::string getQuickSystemSelectLeftButton();
    std::string getQuickSystemSelectRightButton();

    FileData* mRoot;
    std::unique_ptr<CarouselComponent<FileData*>> mCarousel;
    std::unique_ptr<GridComponent<FileData*>> mGrid;
    std::unique_ptr<TextListComponent<FileData*>> mTextList;
    PrimaryComponent<FileData*>* mPrimary;

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
    bool mLeftRightAvailable;
    bool mSystemNameSuffix;

private:
};

#endif // ES_APP_VIEWS_GAMELIST_BASE_H

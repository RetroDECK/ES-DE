//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GridGamelistView.h
//
//  Interface that defines a GamelistView of the type 'grid'.
//

#ifndef ES_APP_VIEWS_GAMELIST_GRID_GAMELIST_VIEW_H
#define ES_APP_VIEWS_GAMELIST_GRID_GAMELIST_VIEW_H

#include "components/BadgeComponent.h"
#include "components/DateTimeComponent.h"
#include "components/ImageGridComponent.h"
#include "components/RatingComponent.h"
#include "components/ScrollableContainer.h"
#include "components/VideoComponent.h"
#include "views/gamelist/ISimpleGamelistView.h"

class GridGamelistView : public ISimpleGamelistView
{
public:
    GridGamelistView(Window* window, FileData* root);
    virtual ~GridGamelistView() {}

    // Called when a FileData* is added, has its metadata changed, or is removed.
    virtual void onFileChanged(FileData* file, bool reloadGamelist) override;

    virtual void onShow() override;
    virtual void onThemeChanged(const std::shared_ptr<ThemeData>& theme) override;
    virtual void setCursor(FileData* cursor) override;

    virtual FileData* getCursor() override { return mGrid.getSelected(); }
    virtual FileData* getNextEntry() override { return mGrid.getNext(); }
    virtual FileData* getPreviousEntry() override { return mGrid.getPrevious(); }
    virtual FileData* getFirstEntry() override { return mGrid.getFirst(); }
    virtual FileData* getLastEntry() override { return mGrid.getLast(); }
    virtual FileData* getFirstGameEntry() override { return firstGameEntry; }

    virtual std::string getName() const override { return "grid"; }

    virtual bool input(InputConfig* config, Input input) override;

    virtual std::vector<HelpPrompt> getHelpPrompts() override;
    virtual void launch(FileData* game) override;

    virtual bool isListScrolling() override { return mGrid.isScrolling(); }
    virtual void stopListScrolling() override
    {
        mGrid.stopAllAnimations();
        mGrid.stopScrolling();
    }

    virtual const std::vector<std::string>& getFirstLetterIndex() override
    {
        return mFirstLetterIndex;
    }

    virtual void addPlaceholder(FileData* firstEntry = nullptr) override;

protected:
    virtual std::string getQuickSystemSelectRightButton() override { return "rightshoulder"; }
    virtual std::string getQuickSystemSelectLeftButton() override { return "leftshoulder"; }
    virtual void populateList(const std::vector<FileData*>& files, FileData* firstEntry) override;
    virtual void remove(FileData* game, bool deleteFile) override;
    virtual void removeMedia(FileData* game) override;
    virtual void update(int deltaTime) override;

    ImageGridComponent<FileData*> mGrid;
    // Points to the first game in the list, i.e. the first entry which is of the type 'GAME'.
    FileData* firstGameEntry;

private:
    void updateInfoPanel();
    const std::string getImagePath(FileData* file);

    void initMDLabels();
    void initMDValues();

    ImageComponent mMarquee;
    ImageComponent mImage;

    TextComponent mLblRating;
    TextComponent mLblReleaseDate;
    TextComponent mLblDeveloper;
    TextComponent mLblPublisher;
    TextComponent mLblGenre;
    TextComponent mLblPlayers;
    TextComponent mLblLastPlayed;
    TextComponent mLblPlayCount;

    BadgeComponent mBadges;
    RatingComponent mRating;
    DateTimeComponent mReleaseDate;
    TextComponent mDeveloper;
    TextComponent mPublisher;
    TextComponent mGenre;
    TextComponent mPlayers;
    DateTimeComponent mLastPlayed;
    TextComponent mPlayCount;
    TextComponent mName;

    std::vector<TextComponent*> getMDLabels();
    std::vector<GuiComponent*> getMDValues();

    ScrollableContainer mDescContainer;
    TextComponent mDescription;
    TextComponent mGamelistInfo;
};

#endif // ES_APP_VIEWS_GAMELIST_GRID_GAMELIST_VIEW_H
